#include "WebServer.h"
#include "secrets.h" // tutaj definiujesz WIFI_SSID i WIFI_PASS

WebInterface::WebInterface() {}

void WebInterface::begin() {
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected: " + WiFi.localIP().toString());

    if (!LittleFS.begin()) {
        Serial.println("LittleFS mount failed");
        return;
    }

    setupRoutes();
    server.begin();
}

void WebInterface::setupRoutes() {
    server.on("/", HTTP_GET, [this]() {
        File f = LittleFS.open("/index.html", "r");
        if (!f) {
            server.send(404, "text/plain", "File not found");
            return;
        }
        server.streamFile(f, "text/html");
        f.close();
    });

    server.on("/style.css", HTTP_GET, [this]() {
        File f = LittleFS.open("/style.css", "r");
        if (!f) {
            server.send(404, "text/plain", "File not found");
            return;
        }
        server.streamFile(f, "text/css");
        f.close();
    });

    server.on("/script.js", HTTP_GET, [this]() {
        File f = LittleFS.open("/script.js", "r");
        if (!f) {
            server.send(404, "text/plain", "File not found");
            return;
        }
        server.streamFile(f, "application/javascript");
        f.close();
    });

    server.on("/start", HTTP_GET, [this]() {
        startFlag = true;
        server.send(200, "text/plain", "START");
    });

    server.on("/stop", HTTP_GET, [this]() {
        stopFlag = true;
        server.send(200, "text/plain", "STOP");
    });
}

void WebInterface::update() {
    server.handleClient();
}

bool WebInterface::startRequested() {
    bool v = startFlag;
    startFlag = false;
    return v;
}

bool WebInterface::stopRequested() {
    bool v = stopFlag;
    stopFlag = false;
    return v;
}