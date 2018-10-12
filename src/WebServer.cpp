#include "WebServer.hpp"

const byte DNS_PORT = 53;

WebServer::WebServer(Facilities::MeshNetwork& mesh): m_mesh(mesh), m_server(80) {
}

const char* PARAM_STATE = "state";

const char * FORM = R"V0G0N(
    <form action="/" method="post">
    New state:<br>
    <input type="text" name="state"><br>
    <input type="submit" value="Submit">
    </form>
)V0G0N";

void WebServer::initialize() {
    m_dnsServer.start(DNS_PORT, "*", m_mesh.getIP());

    m_server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request){
        String resp = "<html> Hello: ";
        resp += m_mesh.getIP().toString() + " " + m_mesh.getMyNodeId() + "\n";
        resp += "current state: ";
        resp += m_state;
        resp += FORM;
        resp += "</html>";
        request->send(200, "text/html", resp.c_str());
    });

    // Send a POST request to <IP>/post with a form field message set to <message>
    m_server.on("/", HTTP_POST, [this](AsyncWebServerRequest *request){
        String message;
        if (request->hasParam(PARAM_STATE, true)) {
            m_state = request->getParam(PARAM_STATE, true)->value();
            request->send(200, "text/plain", "OK, updated state to " + m_state);
        } else {
            request->send(400, "text/plain", "Missing param");
        }
    });

    m_server.begin();
}

void WebServer::loop() {
    m_dnsServer.processNextRequest();
}