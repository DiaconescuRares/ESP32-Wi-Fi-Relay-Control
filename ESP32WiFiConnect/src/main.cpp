#include <Arduino.h>
#include <WiFi.h>
#include "webserver_utils.h"   // custom header with helper functions

#define RELAY_PIN   14
#define BUTTON_PIN  32

const char* ssid     = "TP-Link_5840";
const char* password = "bog239811";
const char* BASIC_AUTH_TOKEN = "Authorization: Basic YWRtaW46MTIzNA=="; // admin:1234 in base64

WiFiServer server(80);
String header;
String output14State = "off";

// timers for connection timeout
unsigned long currentTime  = millis();
unsigned long previousTime = 0;
const long timeoutTime     = 10000;

void setup() {
    Serial.begin(115200);
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    // connect to WiFi and start server
    connectToWiFi(ssid, password, server);
}

void loop() {
    WiFiClient client = server.available(); // check for new clients

    if (client) {
        currentTime  = millis();
        previousTime = currentTime;
        Serial.println("New Client.");

        while (client.connected() && currentTime - previousTime <= timeoutTime) {
            currentTime = millis();

            // when request is complete, process it
            if (readClientRequest(client, header)) {
                processClient(client, header, RELAY_PIN, output14State, BASIC_AUTH_TOKEN);
                break;
            }
        }

        header = "";        // clear request buffer
        client.stop();      // close connection
        Serial.println("Client disconnected.\n");
    }
}
