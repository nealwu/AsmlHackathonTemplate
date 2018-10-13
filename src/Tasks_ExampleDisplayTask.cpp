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
#include <math.h>
namespace Tasks {


const int ExampleDisplayTask::LEDMATRIX_WIDTH = 32;
const int ExampleDisplayTask::LEDMATRIX_HEIGHT = 8;
const int ExampleDisplayTask::LEDMATRIX_SEGMENTS = 4;
const int ExampleDisplayTask::LEDMATRIX_INTENSITY = 5;
const int ExampleDisplayTask::LEDMATRIX_CS_PIN = 16;
const unsigned long ExampleDisplayTask::POLL_DELAY_MS = 100;

const int N = 32;

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

std::vector<std::string> make_circle(double radius, bool full = true) {
    std::vector<std::string> grid;

    for (int i = 0; i < N; i++)
        grid.push_back(string(N, ' '));

    double center = (N - 1) / 2.0;

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            double distance = sqrt((i - center) * (i - center) + (j - center) * (j - center));
            grid[i][j] = distance <= radius + 0.5 ? '*' : ' ';
        }
    }

    return grid;
}


void ExampleDisplayTask::setTransmit(void *task) {
    transmit_task = task;
}

//! Initializes the LED Matrix display.
ExampleDisplayTask::ExampleDisplayTask(Facilities::MeshNetwork& mesh) :
    Task(POLL_DELAY_MS , TASK_FOREVER, std::bind(&ExampleDisplayTask::execute, this)),
    m_mesh(mesh), m_lmd(LEDMATRIX_SEGMENTS, LEDMATRIX_CS_PIN), m_x(0), m_static_index(-1) {
    m_lmd.setEnabled(true);
    m_lmd.setIntensity(LEDMATRIX_INTENSITY);

    m_mesh.onReceive(std::bind(&ExampleDisplayTask::receivedCb, this, std::placeholders::_1, std::placeholders::_2));

    m_index = 0;
    current_grid = 0;
    next_time_goal = 0;
    m_grids.resize(2);

    m_grids[0] = make_circle((N - 1) / 2.0, true);
    m_grids[1] = make_circle(5, true);

    // for (int row = 0; row < N; row++) {
    //     assert((int) m_grid[row].size() == N);

    //     for (int col = 0; col < N; col++) {
    //         int cell_index = max(abs(2 * row - (N - 1)), abs(2 * col - (N - 1)));
    //         m_grid[row][col] = cell_index % 8 >= 6 ? '*' : ' ';
    //     }
    // }
}

int display_row(int row) {
    // Fix the row numbering, which is in a very strange order: 7 -> 0, 15 -> 8, 23 -> 16, 31 -> 24
    return row ^ 7;
}

//! Update display
void ExampleDisplayTask::execute() {
    m_lmd.clear();
    int64_t current_time = std::chrono::steady_clock::now().time_since_epoch().count();
    bool empty_display = false;

    if (current_time >= next_time_goal) {
        current_grid = (current_grid + 1) % m_grids.size();
        empty_display = true;

        // Flip after 4 seconds.
        next_time_goal = current_time + 4e9;
    }

    if (!empty_display || m_static_index != -1) {
        std::vector<std::string> &m_grid = m_grids[m_static_index == -1 ? current_grid : m_static_index];
        assert((int) m_grid.size() == LEDMATRIX_WIDTH);

        for (int row = 0; row < (int) m_grid.size(); row++)
            for (int col = m_index * LEDMATRIX_HEIGHT; col < (m_index + 1) * LEDMATRIX_HEIGHT; col++)
                m_lmd.setPixel(display_row(row), col % LEDMATRIX_HEIGHT, m_grid[row][col] != ' ');
    }

    // Flip the pixel at m_x, 0
    // m_lmd.setPixel(display_row(m_x), 0, !m_lmd.getPixel(display_row(m_x), 0));
    m_lmd.display();

    MY_DEBUG_PRINTF(("Time: " + to_string(m_mesh.getNodeTime()) + "\n").c_str());
}

void ExampleDisplayTask::receivedCb(Facilities::MeshNetwork::NodeId nodeId, String& msg) {
    if (msg.startsWith("IMG")) {
        update(msg.substring(3));
        return;
    }
    if (!msg.startsWith("XYZ"))
        return;

    static std::map<Facilities::MeshNetwork::NodeId, int64_t> id_last_seen;

    MY_DEBUG_PRINTF("Received message: %s\n", msg.c_str());
    char str[100];
    Facilities::MeshNetwork::NodeId their_id, my_id;
    int seconds_to_next = -1;
    sscanf(msg.c_str(), "%s %u %d", str, &their_id, &seconds_to_next);
    MY_DEBUG_PRINTF(("seconds_to_next is " + to_string(seconds_to_next) + "\n").c_str());

    if (seconds_to_next >= 0) {
        int64_t current_time = std::chrono::steady_clock::now().time_since_epoch().count();
        next_time_goal = current_time + seconds_to_next * 1e9;
        ((ExampleTransmitTask *) transmit_task)->next_time = next_time_goal;
        MY_DEBUG_PRINTF(("Got a time; setting next time goal to " + to_string(next_time_goal) + "\n").c_str());
    }

    assert(string(str) == "XYZ");

    int64_t current_time = std::chrono::steady_clock::now().time_since_epoch().count();
    id_last_seen[their_id] = current_time;
    my_id = m_mesh.getMyNodeId();
    id_last_seen[my_id] = current_time;

    std::vector<Facilities::MeshNetwork::NodeId> ids;

    for (auto &id : id_last_seen)
        ids.push_back(id.first);

    // Things are alive if we got an update from them at most 5 seconds ago
    int64_t cutoff_time = current_time - 5e9;

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

void ExampleDisplayTask::update(String state) {
    m_static_index = 0;
    vector<string> grid = {""};
    for (char ch: state) {
        if (ch == '\n') {
            grid.push_back("");
        } else if (ch == '*' || ch == ' ') {
            grid.back().push_back(ch);
        }
    }

    auto &curGrid = m_grids[m_static_index];
    curGrid.assign(LEDMATRIX_WIDTH, string(LEDMATRIX_WIDTH, ' '));
    for (int i = 0; i < (int) min(grid.size(), curGrid.size()); i++) {
        for (int j = 0; j < (int) min(grid[i].size(), curGrid[i].size()); j++) {
            if (grid[i][j] != ' ') {
                curGrid[i][j] = '*';
            }
        }
    }
}

} // namespace Tasks
