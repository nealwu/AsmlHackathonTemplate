#include <Arduino.h>

#include "Debug.hpp"
#include "painlessMesh.h"
#include "Facilities_MeshNetwork.hpp"
#include "Tasks_ExampleTransmitTask.hpp"
#include "Tasks_ExampleDisplayTask.hpp"
#include "WebServer.hpp"

// Translation unit local variables
namespace {

Scheduler                  taskScheduler;

Facilities::MeshNetwork    meshNetwork;
WebServer                  webServer(meshNetwork);
Tasks::ExampleTransmitTask exampleTransmitTask(meshNetwork);
Tasks::ExampleDisplayTask  exampleDisplayTask(meshNetwork);
}



//! Called once at board startup.
void setup()
{
   MY_DEBUG_BEGIN(115200);

   // Create MeshNetwork
   meshNetwork.initialize(F("Mesh-gg"), F("Mesh-Secret-Password"), taskScheduler);

   // Create and add tasks.
   taskScheduler.addTask( exampleTransmitTask );
   taskScheduler.addTask( exampleDisplayTask );
   exampleDisplayTask.setTransmit(&exampleTransmitTask);
   exampleTransmitTask.enable();
   exampleDisplayTask.enable();

   webServer.initialize();

   MY_DEBUG_PRINTLN(F("Setup completed"));
}

//! Called repeatedly after setup().
void loop()
{
   taskScheduler.execute();
   meshNetwork.update();
   webServer.loop();
}
