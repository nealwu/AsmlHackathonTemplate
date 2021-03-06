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
#include <cmath>
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

const int N = 32;
const int CHANGE_DISPLAY_TIME = 4e6;

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
        grid.push_back(std::string(N, ' '));

    double center = (N - 1) / 2.0;

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            double distance = sqrt((i - center) * (i - center) + (j - center) * (j - center));
            grid[i][j] = distance <= radius + 0.5 && (full || distance >= radius - 0.5) ? '*' : ' ';
        }
    }

    return grid;
}

std::vector<std::string> make_asml() {
    return {
        "                                ",
        "                                ",
        "                                ",
        "                                ",
        "                                ",
        "                                ",
        "                                ",
        "                                ",
        "                                ",
        "                                ",
        "                                ",
        "                                ",
        "    *     ****  *     *  *      ",
        "   * *    *     **   **  *      ",
        "  *   *   **    * * * *  *      ",
        " *******    **  *  *  *  *      ",
        " *     *     *  *     *  *      ",
        " *     *  ****  *     *  ****** ",
        "                                ",
        "                                ",
        "                                ",
        "                                ",
        "                                ",
        "                                ",
        "                                ",
        "                                ",
        "                                ",
        "                                ",
        "                                ",
        "                                ",
        "                                ",
        "                                "
    };
}

int scale_round(int x, int nodes) {
    return round((double) x / (8 * nodes - 1) * (N - 1));
}

std::vector<std::string> scale(std::vector<std::string> original, int nodes) {
    std::vector<std::string> scaled(N, std::string(N, ' '));

    for (int x = 0; x < 8 * nodes; x++)
        for (int y = 0; y < 8 * nodes; y++)
            scaled[x + 4 * (4 - nodes)][y] = original[scale_round(x, nodes)][scale_round(y, nodes)];

    return scaled;
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
    next_time_goal = -1;
    m_grids.resize(4);

    m_grids[0] = make_circle((N - 1) / 2.0, true);
    m_grids[1] = make_circle(5, true);
    m_grids[2] = make_circle((N - 1) / 2.0 ,false);
    m_grids[3] = make_asml();

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

int get_next_change_time(int time) {
    time += CHANGE_DISPLAY_TIME;
    return time - time % CHANGE_DISPLAY_TIME;
}

//! Update display
void ExampleDisplayTask::execute() {
    m_lmd.clear();
    int64_t current_time = m_mesh.getNodeTime();
    bool empty_display = false;

    if (next_time_goal == -1) {
        next_time_goal = get_next_change_time(current_time);
        current_grid = (next_time_goal / CHANGE_DISPLAY_TIME) % m_grids.size();
    }

    if (current_time >= next_time_goal) {
        current_grid = (next_time_goal / CHANGE_DISPLAY_TIME) % m_grids.size();
        empty_display = true;
        next_time_goal = get_next_change_time(current_time);
        MY_DEBUG_PRINTF(("Current time is " + to_string(current_time) + "; next time goal is " + to_string(next_time_goal) + "\n").c_str());
    }

    int64_t time_diff = current_time - (next_time_goal - CHANGE_DISPLAY_TIME);
    time_diff = min(time_diff, CHANGE_DISPLAY_TIME - time_diff);
    int intensity = time_diff >= CHANGE_DISPLAY_TIME / 4 ? 5 : 5 * time_diff / (CHANGE_DISPLAY_TIME / 4);
    m_lmd.setIntensity(intensity);

    Facilities::MeshNetwork::NodeId my_id = m_mesh.getMyNodeId();
    id_last_seen[my_id] = id_last_seen[my_id];

    if (!empty_display || m_static_index != -1) {
        std::vector<std::string> &m_grid = m_grids[m_static_index == -1 ? current_grid : m_static_index];
        std::vector<std::string> scaled = scale(m_grid, id_last_seen.size());
        MY_DEBUG_PRINTF(("Displaying with " + to_string(id_last_seen.size()) + " nodes" + "\n").c_str());
        assert((int) scaled.size() == LEDMATRIX_WIDTH);

        for (int row = 0; row < (int) scaled.size(); row++)
            for (int col = m_index * LEDMATRIX_HEIGHT; col < (m_index + 1) * LEDMATRIX_HEIGHT; col++)
                m_lmd.setPixel(display_row(row), col % LEDMATRIX_HEIGHT, scaled[row][col] != ' ');
    }

    // Flip the pixel at m_x, 0
    // m_lmd.setPixel(display_row(m_x), 0, !m_lmd.getPixel(display_row(m_x), 0));
    m_lmd.display();
}

void ExampleDisplayTask::receivedCb(Facilities::MeshNetwork::NodeId nodeId, String& msg) {
    if (msg.startsWith("IMG")) {
        update(msg.substring(3));
        return;
    }
    if (!msg.startsWith("XYZ"))
        return;

    MY_DEBUG_PRINTF("Received message: %s\n", msg.c_str());
    char str[100];
    Facilities::MeshNetwork::NodeId my_id = m_mesh.getMyNodeId();
    Facilities::MeshNetwork::NodeId their_id;
    sscanf(msg.c_str(), "%s %u", str, &their_id);
    assert(string(str) == "XYZ");

    int64_t current_time = m_mesh.getNodeTime();
    id_last_seen[their_id] = current_time;
    id_last_seen[my_id] = current_time;

    std::vector<Facilities::MeshNetwork::NodeId> ids;

    for (auto &id : id_last_seen)
        ids.push_back(id.first);

    // Things are alive if we got an update from them at most 10 seconds ago
    int64_t cutoff_time = current_time - 10e6;

    for (auto &id: ids)
        if (id_last_seen[id] < cutoff_time) {
            MY_DEBUG_PRINTF(("Erasing " + to_string(id) + " which has not been seen since " + to_string(id_last_seen[id]) + "; cutoff is " + to_string(cutoff_time) +
                            " and current time is " + to_string(current_time) + "\n").c_str());
            id_last_seen.erase(id);
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
