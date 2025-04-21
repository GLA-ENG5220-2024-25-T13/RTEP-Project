#include "ApiServer.h"
#include <iostream>
#include <nlohmann/json.hpp> // Using nlohmann/json for convenience

// For convenience
using json = nlohmann::json;


ApiServer::ApiServer(AlarmController& controller, const std::string& host, int port)
    : alarmController(controller), listenHost(host), listenPort(port), isRunning(false) {}

ApiServer::~ApiServer() {
    stop(); // Ensure server is stopped cleanly
}

bool ApiServer::start() {
    if (isRunning.load()) {
        std::cout << "API server already running." << std::endl;
        return true;
    }

    // --- Define API Endpoints ---

    // GET /status
    svr.Get("/status", [&](const httplib::Request& req, httplib::Response& res) {
        json response;
        response["state"] = alarmController.getStateString();
        response["last_trigger"] = alarmController.getLastTriggerSource(); // First trigger source

        // --- sensor states ---
        json sensor_states;
        sensor_states["pir_active"] = alarmController.isPirActive();
        sensor_states["proximity_active"] = alarmController.isProximityActive();
        response["sensors"] = sensor_states;
        // --- End sensor states ---

        res.set_content(response.dump(), "application/json");
    });

    // POST /arm
    svr.Post("/arm", [&](const httplib::Request& req, httplib::Response& res) {
        alarmController.arm();
        json response;
        response["status"] = "success";
        response["message"] = "System armed.";
        response["current_state"] = alarmController.getStateString();
        res.set_content(response.dump(), "application/json");
    });

    // POST /disarm
    svr.Post("/disarm", [&](const httplib::Request& req, httplib::Response& res) {
        alarmController.disarm();
        json response;
        response["status"] = "success";
        response["message"] = "System disarmed.";
        response["current_state"] = alarmController.getStateString();
        res.set_content(response.dump(), "application/json");
    });

     // Optional: POST /reset (to reset from TRIGGERED state)
     svr.Post("/reset", [&](const httplib::Request& req, httplib::Response& res) {
        alarmController.resetTrigger();
        json response;
        response["status"] = "success";
        response["message"] = "Alarm trigger reset.";
        response["current_state"] = alarmController.getStateString();
        res.set_content(response.dump(), "application/json");
    });


    // --- Start Server Thread ---
    try {
        serverThread = std::thread(&ApiServer::run, this);
        isRunning.store(true);
        std::cout << "API server starting on " << listenHost << ":" << listenPort << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "ERROR starting API server thread: " << e.what() << std::endl;
        return false;
    }
    return true;
}

void ApiServer::stop() {
    if (isRunning.load()) {
        std::cout << "Stopping API server..." << std::endl;
        svr.stop(); // Tell httplib to stop listening
        if (serverThread.joinable()) {
            serverThread.join(); // Wait for the server thread to finish
        }
        isRunning.store(false);
        std::cout << "API server stopped." << std::endl;
    }
}

void ApiServer::run() {
    // svr.listen blocks until svr.stop() is called
    if (!svr.listen(listenHost.c_str(), listenPort)) {
         std::cerr << "ERROR: httplib failed to listen on " << listenHost << ":" << listenPort << std::endl;
         // Handle error, maybe signal main thread
         isRunning.store(false); // Mark as not running if listen failed immediately
    }
    std::cout << "API server listener finished." << std::endl; // Should print after stop() is called
}