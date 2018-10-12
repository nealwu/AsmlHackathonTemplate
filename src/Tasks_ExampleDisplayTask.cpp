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

#include <algorithm>
#include <cassert>
#include <chrono>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace Tasks {


const int ExampleDisplayTask::LEDMATRIX_WIDTH = 32;
const int ExampleDisplayTask::LEDMATRIX_HEIGHT = 8;
const int ExampleDisplayTask::LEDMATRIX_SEGMENTS = 4;
const int ExampleDisplayTask::LEDMATRIX_INTENSITY = 5;
const int ExampleDisplayTask::LEDMATRIX_CS_PIN = 16;
const unsigned long ExampleDisplayTask::POLL_DELAY_MS = 100;

string to_string(long long n) {
    bool negative = false;

    if (n < 0) {
        negative = true;
        n = -n;
    }

    string str = "";

    do {
        str += n % 10 + '0';
        n /= 10;
    } while (n != 0);

    reverse(str.begin(), str.end());

    if (negative)
        str = "-" + str;

    return str;
}

//! Initializes the LED Matrix display.
ExampleDisplayTask::ExampleDisplayTask(Facilities::MeshNetwork& mesh) :
    Task(POLL_DELAY_MS , TASK_FOREVER, std::bind(&ExampleDisplayTask::execute, this)),
    m_mesh(mesh), m_lmd(LEDMATRIX_SEGMENTS, LEDMATRIX_CS_PIN), m_x(0) {
    m_lmd.setEnabled(true);
    m_lmd.setIntensity(LEDMATRIX_INTENSITY);

    m_mesh.onReceive(std::bind(&ExampleDisplayTask::receivedCb, this, std::placeholders::_1, std::placeholders::_2));

    m_index = 0;
    m_grid = {};
    int N = 32;

    for (int i = 0; i < N; i++)
        m_grid.push_back(string(N, ' '));

    assert((int) m_grid.size() == N);

    for (int row = 0; row < N; row++) {
        assert((int) m_grid[row].size() == N);

        for (int col = 0; col < N; col++) {
            int cell_index = max(abs(2 * row - (N - 1)), abs(2 * col - (N - 1)));
            m_grid[row][col] = cell_index % 8 >= 6 ? '*' : ' ';
        }
    }

    // m_grid = {
    //     "        ",
    //     "        ",
    //     "        ",
    //     "********",
    //     "*      *",
    //     "*      *",
    //     "*      *",
    //     "********",
    //     "        ",
    //     "       *",
    //     "       *",
    //     "       *",
    //     "********",
    //     "        ",
    //     "       *",
    //     "       *",
    //     "       *",
    //     "********",
    //     "        ",
    //     "*      *",
    //     "*  **  *",
    //     "*  **  *",
    //     "********",
    //     "        ",
    //     "********",
    //     "   **   ",
    //     "   **   ",
    //     "   **   ",
    //     "********",
    //     "        ",
    //     "        ",
    //     "        "
    // };
}

int display_row(int row) {
    // Fix the row numbering, which is in a very strange order: 7 -> 0, 15 -> 8, 23 -> 16, 31 -> 24
    return row ^ 7;
}

//! Update display
void ExampleDisplayTask::execute() {
    m_lmd.clear();
    assert((int) m_grid.size() == LEDMATRIX_WIDTH);

    for (int row = 0; row < (int) m_grid.size(); row++)
        for (int col = m_index * LEDMATRIX_HEIGHT; col < (m_index + 1) * LEDMATRIX_HEIGHT; col++)
            m_lmd.setPixel(display_row(row), col % LEDMATRIX_HEIGHT, m_grid[row][col] != ' ');

    // Flip the pixel at m_x, 0
    // m_lmd.setPixel(display_row(m_x), 0, !m_lmd.getPixel(display_row(m_x), 0));
    m_lmd.display();
}

void ExampleDisplayTask::receivedCb(Facilities::MeshNetwork::NodeId nodeId, String& msg) {
    if (!msg.startsWith("XYZ"))
        return;

    static std::map<Facilities::MeshNetwork::NodeId, int64_t> id_last_seen;

    MY_DEBUG_PRINTF("Received message: %s\n", msg.c_str());
    char str[100];
    Facilities::MeshNetwork::NodeId their_id, my_id;
    sscanf(msg.c_str(), "%s %ud", str, &their_id);
    assert(string(str) == "XYZ");

    int64_t current_time = std::chrono::steady_clock::now().time_since_epoch().count();
    id_last_seen[their_id] = current_time;
    my_id = m_mesh.getMyNodeId();
    id_last_seen[my_id] = current_time;

    std::vector<Facilities::MeshNetwork::NodeId> ids;

    for (auto &id : id_last_seen)
        ids.push_back(id.first);

    int64_t cutoff_time = current_time - std::chrono::milliseconds(5000).count();

    for (auto &id: ids)
        if (id_last_seen[id] < cutoff_time) {
            MY_DEBUG_PRINTF(("Erasing " + to_string(id) + " which has not been seen since " + to_string(id_last_seen[id]) + "; cutoff is " + to_string(cutoff_time) +
                            " and current time is " + to_string(current_time) + "\n").c_str());
            // id_last_seen.erase(id);
        }

    m_index = 0;

    for (auto &id : id_last_seen) {
        if (id.first == my_id)
            break;

        m_index++;
    }

    MY_DEBUG_PRINTF("My index is %d\n", m_index);
    m_x = (m_x + 1) % LEDMATRIX_WIDTH;
}

} // namespace Tasks
