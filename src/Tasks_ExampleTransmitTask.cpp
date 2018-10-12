//
//! \file
//
// Created by Sander van Woensel / Umut Uyumaz
// Copyright (c) 2018 ASML Netherlands B.V. All rights reserved.
//
//! Example Task
#include "Tasks_ExampleTransmitTask.hpp"

#include "Debug.hpp"
#include "Facilities_MeshNetwork.hpp"

#include <chrono>
#include <functional>

namespace Tasks {


ExampleTransmitTask::ExampleTransmitTask(Facilities::MeshNetwork& mesh) :
    Task(TASK_SECOND * 2 , TASK_FOREVER, std::bind(&ExampleTransmitTask::execute, this)),
    m_mesh(mesh), next_time(-1) {

}

void ExampleTransmitTask::execute() {
    int64_t current_time = std::chrono::steady_clock::now().time_since_epoch().count();

    if (next_time == -1)
        next_time = current_time + 4e9;

    String msg = "XYZ ";
    msg += m_mesh.getMyNodeId();

    if (current_time >= next_time) {
        msg += " 4";
        next_time = current_time + 4e9;
    } else {
        msg += " -1";
    }

    m_mesh.sendBroadcast(msg);
}

} // namespace Tasks
