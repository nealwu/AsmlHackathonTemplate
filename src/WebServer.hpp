#ifndef __WebServer__
#define __WebServer__

#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include "Facilities_MeshNetwork.hpp"

class WebServer {
public:
    WebServer(Facilities::MeshNetwork& mesh);
    void initialize();
    void loop();
private:
    Facilities::MeshNetwork& mesh;
    AsyncWebServer server;
    DNSServer dnsServer;
};

#endif