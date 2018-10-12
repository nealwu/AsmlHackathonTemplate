//
//! \file
//
// Created by Sander van Woensel / Umut Uyumaz
// Copyright (c) 2018 ASML Netherlands B.V. All rights reserved.
//
//! Example Task to output something to the LED Matrix.
#include "Tasks_ExampleDisplayTask.hpp"

#include "Debug.hpp"
#include "Facilities_MeshNetwork.hpp"

#include <LEDMatrixDriver.hpp>

#include <functional>
#include <string>
#include <vector>

namespace Tasks {


const int ExampleDisplayTask::LEDMATRIX_WIDTH = 32;
const int ExampleDisplayTask::LEDMATRIX_HEIGHT = 8;
const int ExampleDisplayTask::LEDMATRIX_SEGMENTS = 4;
const int ExampleDisplayTask::LEDMATRIX_INTENSITY = 5;
const int ExampleDisplayTask::LEDMATRIX_CS_PIN = 16;
const unsigned long ExampleDisplayTask::POLL_DELAY_MS = 100;

//! Initializes the LED Matrix display.
ExampleDisplayTask::ExampleDisplayTask(Facilities::MeshNetwork& mesh) :
    Task(POLL_DELAY_MS , TASK_FOREVER, std::bind(&ExampleDisplayTask::execute, this)),
    m_mesh(mesh), m_lmd(LEDMATRIX_SEGMENTS, LEDMATRIX_CS_PIN), m_x(0) {
    m_lmd.setEnabled(true);
    m_lmd.setIntensity(LEDMATRIX_INTENSITY);

    m_mesh.onReceive(std::bind(&ExampleDisplayTask::receivedCb, this, std::placeholders::_1, std::placeholders::_2));

    m_grid = {
        "        ",
        "        ",
        "        ",
        "********",
        "*      *",
        "*      *",
        "*      *",
        "********",
        "        ",
        "       *",
        "       *",
        "       *",
        "********",
        "        ",
        "       *",
        "       *",
        "       *",
        "********",
        "        ",
        "*      *",
        "*  **  *",
        "*  **  *",
        "********",
        "        ",
        "********",
        "   **   ",
        "   **   ",
        "   **   ",
        "********",
        "        ",
        "        ",
        "        "
    };
}

//! Update display
void ExampleDisplayTask::execute() {
    m_lmd.clear();

    for (int row = 0; row < (int) m_grid.size(); row++) {
        int display_row = row ^ 7;

        for (int col = 0; col < (int) m_grid[row].size(); col++)
            m_lmd.setPixel(display_row, col, m_grid[row][col] != ' ');
    }
    
    m_lmd.setPixel(m_x ^ 7, 0, !m_lmd.getPixel(m_x ^ 7, 0));

    // for (int i = 0; i < 32; i++) {
    //     int length = i / 4;

    //     for (int j = 0; j < length; j++)
    //         m_lmd.setPixel(i, j, true);
    // }
    m_lmd.display();
}

void ExampleDisplayTask::receivedCb(Facilities::MeshNetwork::NodeId nodeId, String& msg) {
    if (!msg.startsWith("XYZ"))
        return;
    
    MY_DEBUG_PRINTF("Received message: %s\n", msg.c_str());
    m_x = (m_x + 1) % LEDMATRIX_WIDTH;
}

} // namespace Tasks
