#ifndef __WebServer__
#define __WebServer__

#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include "Facilities_MeshNetwork.hpp"
#include "Tasks_ExampleDisplayTask.hpp"

class WebServer {
public:
    WebServer(Facilities::MeshNetwork& mesh, Tasks::ExampleDisplayTask &display);
    void initialize();
    void loop();
private:
    String m_state;
    Facilities::MeshNetwork& m_mesh;
    Tasks::ExampleDisplayTask &m_display;
    AsyncWebServer m_server;
    DNSServer m_dnsServer;
    int m_broadcaseCounter = 0;
};

#endif