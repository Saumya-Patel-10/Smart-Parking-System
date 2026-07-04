# Smart Parking System — Software (CS) Version

This is a C++ conversion of the original **Verilog/FPGA smart parking project**
(74147 priority encoder, 7442 decoder, LM339 comparator, IR sensors, LEDs) into
a pure software project you can build, run, and demo without any hardware.

It swaps the FPGA for a small C++ backend and the physical LEDs for a live
web page, using **cpp-httplib** (a single-header C++ HTTP library — the C++
equivalent of a micro web framework like Flask) to serve a REST API and an
HTML/CSS/JS frontend.

## How the hardware logic maps to code

| Hardware (original project)      | Software (this project)                          | File |
|-----------------------------------|---------------------------------------------------|------|
| IR sensors (per-slot occupancy)   | `bool sensor_[N]` array                            | `ParkingSystem.h` |
| 74147 priority encoder            | `findFirstFreeSlotLocked()` — scans for the lowest-index free slot | `ParkingSystem.h` |
| LM339 comparator                  | `isFullLocked()` — compares available count to 0 to gate allocation | `ParkingSystem.h` |
| 7442 BCD decoder → LEDs           | `decodeToBitmaskLocked()` + the web page's colored slot indicators | `ParkingSystem.h`, `public/script.js` |
| Verilog `always @(*)` combinational blocks | `recomputeStatusLocked()` — recalculates `avail`, `full`, `entry`, `no_entry` every time a sensor changes | `ParkingSystem.h` |
| Physical wiring between ICs       | REST API calls between the browser and the C++ server | `main.cpp` |

The **priority-based first-fit allocation** from the original Verilog module
(`if sensor[0]==0 assign slot 0, else if sensor[1]==0 assign slot 1, ...`) is
preserved exactly — `findFirstFreeSlotLocked()` is a straight loop doing the
same thing, just in software instead of gates.

## Project structure

```
smart_parking_web/
├── ParkingSystem.h     # Core logic (the "digital design" layer)
├── main.cpp            # HTTP server + REST API (the "control/IO" layer)
├── httplib.h            # Single-header C++ HTTP library (third-party)
└── public/
    ├── index.html       # Frontend page
    ├── style.css         # Styling (LED-style slot indicators)
    └── script.js          # Polls the API and updates the page
```

## Building and running

Requires g++ (C++17) and pthreads — no external package manager needed since
`httplib.h` is a single header already included in this folder.

```bash
g++ -std=c++17 -O2 -pthread main.cpp -o smart_parking_server
./smart_parking_server
```

Then open **http://localhost:8080** in a browser.

## API reference

| Method | Endpoint            | Description |
|--------|----------------------|--------------|
| GET    | `/api/status`         | Returns current state as JSON: `avail`, `full`, `lastAssignedSlot`, `slots[]` |
| POST   | `/api/enter`           | Simulates a new vehicle arriving; auto-assigns the first free slot (or returns `full`) |
| POST   | `/api/exit/{id}`        | Simulates a vehicle leaving slot `id` |
| POST   | `/api/toggle/{id}`      | Manually flips a slot's sensor state (for demoing) |

## Extending it (matches the "Future Scope" section of the original report)

- Swap the in-memory `sensor_` array for a database table to persist state.
- Add a queue (`std::queue`) so vehicles wait in line when `full == true`,
  matching the report's "no additional vehicles allowed until a slot frees up"
  behavior but with FIFO fairness.
- Add timestamps per slot to compute parking duration and billing.
- Replace manual JSON building with a library (e.g. nlohmann/json) as the
  project grows.
- Port the same `ParkingSystem` class logic to a class assignment on
  priority queues / arrays for a Data Structures course — the "priority
  encoder" behavior here is literally a linear-scan priority selection,
  which maps directly to a min-heap or priority queue exercise.
