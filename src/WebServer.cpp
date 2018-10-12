#include "WebServer.hpp"

const byte DNS_PORT = 53;

WebServer::WebServer(Facilities::MeshNetwork& mesh): mesh(mesh), server(80) {
}

void WebServer::initialize() {
    dnsServer.start(DNS_PORT, "*", mesh.getIP());

    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request){
        String resp = "Hello: ";
        resp += mesh.getIP().toString() + " " + mesh.getMyNodeId();
        request->send(200, "text/plain", resp.c_str());
    });

    server.begin();
}

void WebServer::loop() {
    dnsServer.processNextRequest();
}