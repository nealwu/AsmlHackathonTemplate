#include "WebServer.hpp"

const byte DNS_PORT = 53;

#include <vector>
#include <string>

using namespace std;

WebServer::WebServer(Facilities::MeshNetwork& mesh, Tasks::ExampleDisplayTask &display): m_mesh(mesh), m_display(display), m_server(80) {
}

const char* PARAM_STATE = "state";

const char * FORM = R"V0G0N(
    <!DOCTYPE html>
<html>
<head>
    <title>Board admin</title>
</head>
<body>

<h3>State</h3>
<form action="/" method="post">
    <textarea rows="32" cols="32" name="state">$STATE</textarea>
    <input type="submit" value="Update">
</form>

</body>
</html>

)V0G0N";

void WebServer::initialize() {
    m_dnsServer.start(DNS_PORT, "*", m_mesh.getIP());

    m_server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request){
        String resp(FORM);
        resp.replace("$STATE", m_state);
        request->send(200, "text/html", resp.c_str());
    });

    // Send a POST request to <IP>/post with a form field message set to <message>
    m_server.on("/", HTTP_POST, [this](AsyncWebServerRequest *request){
        String message;
        if (request->hasParam(PARAM_STATE, true)) {
            m_state = request->getParam(PARAM_STATE, true)->value();
            request->send(200, "text/plain", "OK, updated state to \n" + m_state);
            m_display.update(m_state);
            m_broadcaseCounter = 20;
        } else {
            request->send(400, "text/plain", "Missing param");
        }
    });

    m_server.begin();
}

void WebServer::loop() {
    m_dnsServer.processNextRequest();
    if (m_broadcaseCounter > 0) {
        m_mesh.sendBroadcast("IMG" + m_state);
        m_broadcaseCounter--;
    }
}