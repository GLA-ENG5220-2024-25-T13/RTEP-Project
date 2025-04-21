#ifndef APISERVER_H
#define APISERVER_H

#include "AlarmController.h"
#include "../third_party/cpp-httplib/httplib.h" // Include httplib.h
#include <thread>
#include <atomic>
#include <string>

class ApiServer
{
public:
    ApiServer(AlarmController &controller, const std::string &host = "0.0.0.0", int port = 8080);
    ~ApiServer();

    bool start();
    void stop();

private:
    void run(); // Server loop runs in a separate thread

    AlarmController &alarmController;
    httplib::Server svr;
    std::thread serverThread;
    std::string listenHost;
    int listenPort;
    std::atomic<bool> isRunning; // Use atomic for status check if needed, though stop() handles shutdown
};

#endif