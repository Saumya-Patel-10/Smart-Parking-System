#include "httplib.h"
#include "ParkingSystem.h"
#include <sstream>
#include <iostream>

// Builds a small JSON string by hand (no external JSON library needed).
static std::string statusToJson(const ParkingSystem::Status& s) {
    std::ostringstream o;
    o << "{";
    o << "\"avail\":" << s.avail << ",";
    o << "\"full\":" << (s.full ? "true" : "false") << ",";
    o << "\"lastAssignedSlot\":" << s.lastAssignedSlot << ",";
    o << "\"slots\":[";
    for (int i = 0; i < ParkingSystem::N; ++i) {
        o << (s.sensor[i] ? "true" : "false");
        if (i != ParkingSystem::N - 1) o << ",";
    }
    o << "]}";
    return o.str();
}

int main() {
    ParkingSystem parking;
    httplib::Server svr;

    // Serve the frontend (index.html, style.css, script.js) from ./public
    svr.set_mount_point("/", "./public");

    // GET /api/status -> full current state as JSON
    svr.Get("/api/status", [&](const httplib::Request&, httplib::Response& res) {
        res.set_content(statusToJson(parking.getStatus()), "application/json");
    });

    // POST /api/enter -> a new vehicle requests a slot; system auto-assigns
    // the first free slot (priority encoder) if the lot isn't full
    // (comparator check).
    svr.Post("/api/enter", [&](const httplib::Request&, httplib::Response& res) {
        auto slot = parking.enter();
        std::ostringstream o;
        if (slot.has_value()) {
            o << "{\"success\":true,\"slot\":" << *slot << "}";
        } else {
            o << "{\"success\":false,\"message\":\"Parking lot full\"}";
        }
        res.set_content(o.str(), "application/json");
    });

    // POST /api/exit/:id -> vehicle leaves a specific slot
    svr.Post(R"(/api/exit/(\d+))", [&](const httplib::Request& req, httplib::Response& res) {
        int slot = std::stoi(req.matches[1]);
        bool ok = parking.exitSlot(slot);
        res.set_content(std::string("{\"success\":") + (ok ? "true" : "false") + "}", "application/json");
    });

    // POST /api/toggle/:id -> manually flip a slot's sensor (demo/testing)
    svr.Post(R"(/api/toggle/(\d+))", [&](const httplib::Request& req, httplib::Response& res) {
        int slot = std::stoi(req.matches[1]);
        auto status = parking.getStatus();
        bool newVal = !status.sensor[slot];
        bool ok = parking.setSensor(slot, newVal);
        res.set_content(std::string("{\"success\":") + (ok ? "true" : "false") + "}", "application/json");
    });

    std::cout << "Smart Parking server running at http://localhost:8080\n";
    svr.listen("0.0.0.0", 8080);
    return 0;
}
