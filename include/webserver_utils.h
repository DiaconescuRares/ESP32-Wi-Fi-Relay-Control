#ifndef WEBSERVER_UTILS_H
#define WEBSERVER_UTILS_H

#include <WiFi.h>

// === DECLARATIONS ===
void connectToWiFi(const char* ssid, const char* password, WiFiServer &server);
bool readClientRequest(WiFiClient &client, String &header);
bool checkAuthentication(WiFiClient &client, String &header, const char* BASIC_AUTH_TOKEN);
void handleRelayControl(String &header, int relayPin, String &output14State);
void updateRelayState(int relayPin, String &output14State, bool state);
void sendWebPage(WiFiClient &client, String &output14State);
void processClient(WiFiClient &client, String &header, int relayPin, String &output14State, const char* BASIC_AUTH_TOKEN);

// === IMPLEMENTATION ===
void connectToWiFi(const char* ssid, const char* password, WiFiServer &server) {
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nWiFi connected.");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    server.begin();
}

bool readClientRequest(WiFiClient &client, String &header) {
    // read the HTTP request line by line
    String currentLine = "";
    while (client.available()) {
        char c = client.read();
        Serial.write(c);
        header += c;

        // check for end of request (empty line)
        if (c == '\n') {
            if (currentLine.length() == 0) {
                return true;  // request is complete
            } else {
                currentLine = "";
            }
        } else if (c != '\r') {
            currentLine += c;
        }
    }
    return false;
}

bool checkAuthentication(WiFiClient &client, String &header, const char* BASIC_AUTH_TOKEN) {
    // look for Basic Auth header
    if (header.indexOf(BASIC_AUTH_TOKEN) < 0) {
        client.println("HTTP/1.1 401 Unauthorized");
        client.println("WWW-Authenticate: Basic realm=\"ESP32\"");
        client.println("Content-Type: text/plain");
        client.println("Connection: close");
        client.println();
        client.println("Authentication required.");
        client.stop();
        header = "";
        Serial.println("Auth failed / missing. Prompted for credentials.");
        return false;
    }
    return true;
}

void updateRelayState(int relayPin, String &output14State, bool state) {
    // write HIGH or LOW to relay pin and update state variable
    if (state == true) {
        digitalWrite(relayPin, HIGH);
        output14State = "on";
    } else {
        digitalWrite(relayPin, LOW);
        output14State = "off";
    }
}

void handleRelayControl(String &header, int relayPin, String &output14State) {
    // detect commands in the request and update relay
    if (header.indexOf("GET /14/on") >= 0) {
        Serial.println("GPIO 14 on");
        updateRelayState(relayPin, output14State, true);
    } else if (header.indexOf("GET /14/off") >= 0) {
        Serial.println("GPIO 14 off");
        updateRelayState(relayPin, output14State, false);
    }
}

void sendWebPage(WiFiClient &client, String &output14State) {
    // send HTTP response with HTML content
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/html");
    client.println("Connection: close");
    client.println();

    // build webpage
    client.println("<!DOCTYPE html><html lang='en'>");
    client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
    client.println("<link rel=\"icon\" href=\"data:,\">");
    client.println("<style>");
    client.println("body { font-family: Arial, sans-serif; margin:0; padding:0; background:#f0f2f5; text-align:center; }");
    client.println("h1 { background:#4CAF50; color:white; padding:20px; margin:0; }");
    client.println(".card { background:white; margin:40px auto; padding:30px; border-radius:15px; "
                   "box-shadow:0 4px 10px rgba(0,0,0,0.1); max-width:400px; }");
    client.println(".state { font-size:22px; margin-bottom:20px; }");
    client.println(".button { display:inline-block; padding:15px 35px; font-size:20px; border:none; "
                   "border-radius:10px; cursor:pointer; transition:0.3s; }");
    client.println(".on { background:#4CAF50; color:white; }");
    client.println(".on:hover { background:#45a049; }");
    client.println(".off { background:#f44336; color:white; }");
    client.println(".off:hover { background:#da190b; }");
    client.println("footer { font-size:14px; color:#888; margin-top:30px; }");
    client.println("</style></head>");
    client.println("<body>");
    client.println("<h1>ESP32 Remote Relay Control</h1>");
    client.println("<div class='card'>");
    client.println("<p class='state'>GPIO 14 - State: <b>" + output14State + "</b></p>");

    // show ON / OFF button depending on state
    if (output14State == "off") {
        client.println("<a href=\"/14/on\"><button class='button on'>Turn ON</button></a>");
    } else {
        client.println("<a href=\"/14/off\"><button class='button off'>Turn OFF</button></a>");
    }

    client.println("</div>");
    client.println("<footer>Prototype Remote Relay Control - Diaconescu Rares Theodor &copy; 2025</footer>");
    client.println("</body></html>");
    client.println();
}

void processClient(WiFiClient &client, String &header, int relayPin, String &output14State, const char* BASIC_AUTH_TOKEN) {
    // 1. authentication
    if (!checkAuthentication(client, header, BASIC_AUTH_TOKEN)) return;

    // 2. relay control
    handleRelayControl(header, relayPin, output14State);

    // 3. send webpage
    sendWebPage(client, output14State);
}

#endif
