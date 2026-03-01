#pragma once
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>

class WebInterface {
public:
    WebInterface();
    void begin();
    void update();
    bool startRequested();
    bool stopRequested();

private:
    void setupRoutes();

    WebServer server{80};
    bool startFlag = false;
    bool stopFlag = false;
};