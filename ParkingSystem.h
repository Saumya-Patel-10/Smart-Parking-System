#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <mutex>

// =====================================================================
// ParkingSystem
// ---------------------------------------------------------------------
// This class is a direct software translation of the original Verilog
// `smart_parking` module and the encoder/decoder/comparator hardware
// design described in the report. Mapping between hardware and code:
//
//   IR Sensors (per-slot occupancy input)
//       -> std::array<bool, N> sensor_   (true = occupied, false = free)
//
//   74147 Priority Encoder
//       (picks the highest-priority *free* slot and encodes its index)
//       -> findFirstFreeSlot()  -- returns the lowest-index free slot,
//          exactly like the priority encoder resolving multiple free
//          IR sensor lines down to a single binary slot number.
//
//   7442 BCD-to-Decimal Decoder
//       (turns the encoded slot number back into one active output line
//        that drives a single LED)
//       -> decodeToBitmask(int slot) -- turns a slot index back into a
//          one-hot bitmask, exactly like the decoder driving exactly one
//          LED line high for the chosen slot.
//
//   LM339 Comparator
//       (compares available slots against zero / against a request to
//        gate whether allocation may proceed)
//       -> isFull() / hasAvailable() -- the software comparison that
//          gates whether enter() is allowed to proceed, mirroring the
//          comparator's role in the original circuit.
//
// The recomputeStatus() function reproduces the combinational always@(*)
// blocks from the Verilog module (avail, full, entry, no_entry).
// =====================================================================

class ParkingSystem {
public:
    static constexpr int N = 8; // matches `parameter N = 8` in the Verilog module

    ParkingSystem() {
        sensor_.fill(false);
        recomputeStatus();
    }

    // ---- Public state snapshot (mirrors the Verilog output ports) -----
    struct Status {
        std::array<bool, N> sensor;     // raw sensor line per slot
        std::array<bool, N> entry;      // == sensor (occupied slots)
        std::array<bool, N> no_entry;   // == ~sensor (free slots)
        int avail;                      // count of free slots
        bool full;                      // avail == 0
        int lastAssignedSlot;           // -1 if none assigned yet
    };

    // A new vehicle arrives at the gate. Mirrors the encoder->comparator->
    // decoder pipeline: find first free slot (encoder), check capacity
    // (comparator), and drive the corresponding output line (decoder).
    std::optional<int> enter() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (isFullLocked()) {
            return std::nullopt; // comparator blocks allocation: lot is full
        }
        int slot = findFirstFreeSlotLocked(); // priority encoder step
        if (slot < 0) return std::nullopt;
        sensor_[slot] = true;                 // decoder drives this slot's LED/occupied line
        lastAssigned_ = slot;
        recomputeStatusLocked();
        return slot;
    }

    // A vehicle leaves a given slot (IR sensor goes back to "free").
    bool exitSlot(int slot) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (slot < 0 || slot >= N) return false;
        sensor_[slot] = false;
        recomputeStatusLocked();
        return true;
    }

    // Directly set a sensor line (useful for manual testing/demo toggles).
    bool setSensor(int slot, bool occupied) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (slot < 0 || slot >= N) return false;
        sensor_[slot] = occupied;
        recomputeStatusLocked();
        return true;
    }

    Status getStatus() {
        std::lock_guard<std::mutex> lock(mutex_);
        return status_;
    }

private:
    std::array<bool, N> sensor_{};
    Status status_{};
    int lastAssigned_ = -1;
    std::mutex mutex_;

    // ---- 74147 priority encoder equivalent ----
    int findFirstFreeSlotLocked() const {
        for (int i = 0; i < N; ++i) {
            if (!sensor_[i]) return i;
        }
        return -1;
    }

    // ---- LM339 comparator equivalent ----
    bool isFullLocked() const {
        for (bool occ : sensor_) if (!occ) return false;
        return true;
    }

    // ---- 7442 decoder equivalent: one-hot bitmask for a slot index ----
    static std::array<bool, N> decodeToBitmaskLocked(int slot) {
        std::array<bool, N> out{};
        if (slot >= 0 && slot < N) out[slot] = true;
        return out;
    }

    // Reproduces the two `always @(*)` blocks from the Verilog module.
    void recomputeStatusLocked() {
        status_.sensor = sensor_;
        int freeCount = 0;
        for (int i = 0; i < N; ++i) {
            status_.entry[i] = sensor_[i];       // entry mirrors sensor
            status_.no_entry[i] = !sensor_[i];   // no_entry = ~sensor
            if (!sensor_[i]) ++freeCount;
        }
        status_.avail = freeCount;
        status_.full = (freeCount == 0);
        status_.lastAssignedSlot = lastAssigned_;
    }

    void recomputeStatus() { recomputeStatusLocked(); }
};
