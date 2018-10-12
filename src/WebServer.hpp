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
    String m_state;
    Facilities::MeshNetwork& m_mesh;
    AsyncWebServer m_server;
    DNSServer m_dnsServer;
};

#endif