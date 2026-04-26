#include "RobotBase.h"
#include <vector>
#include <cmath>
#include <limits>
#include <cstdlib>
#include <ctime>

class Robot_Ratboy : public RobotBase {
private:
    int targetRow, targetCol;
    bool hasTarget;

public:
    Robot_Ratboy() : RobotBase(3, 4, railgun), targetRow(-1), targetCol(-1), hasTarget(false) {
        m_name = "Ratboy";
        std::srand(static_cast<unsigned>(std::time(nullptr)));
    }

    void get_radar_direction(int& dir) override { dir = 0; }

    void process_radar_results(const std::vector<RadarObj>& results) override {
        hasTarget = false;
        int currentRow, currentCol;
        get_current_location(currentRow, currentCol);
        int bestDist = std::numeric_limits<int>::max();
        for (const auto& obj : results) {
            if (obj.m_type == 'R') {
                int dist = std::abs(obj.m_row - currentRow) + std::abs(obj.m_col - currentCol);
                if (dist < bestDist) {
                    bestDist = dist;
                    targetRow = obj.m_row;
                    targetCol = obj.m_col;
                    hasTarget = true;
                }
            }
        }
    }

    bool get_shot_location(int& shotRow, int& shotCol) override {
        if (hasTarget) {
            shotRow = targetRow;
            shotCol = targetCol;
            return true;
        }
        return false;
    }

    void get_move_direction(int& direction, int& distance) override {
        int currentRow, currentCol;
        get_current_location(currentRow, currentCol);

        if (hasTarget) {
            int dr = (targetRow > currentRow) ? 1 : (targetRow < currentRow) ? -1 : 0;
            int dc = (targetCol > currentCol) ? 1 : (targetCol < currentCol) ? -1 : 0;
            if (dr == -1 && dc == 0) direction = 1;
            else if (dr == -1 && dc == 1) direction = 2;
            else if (dr == 0 && dc == 1) direction = 3;
            else if (dr == 1 && dc == 1) direction = 4;
            else if (dr == 1 && dc == 0) direction = 5;
            else if (dr == 1 && dc == -1) direction = 6;
            else if (dr == 0 && dc == -1) direction = 7;
            else if (dr == -1 && dc == -1) direction = 8;
            else direction = 0;
            distance = get_move_speed();
            return;
        }

        for (int d = 1; d <= 8; ++d) {
            int dr = directions[d].first;
            int dc = directions[d].second;
            int newRow = currentRow + dr;
            int newCol = currentCol + dc;
            if (newRow >= 0 && newRow < m_board_row_max &&
                newCol >= 0 && newCol < m_board_col_max) {
                direction = d;
                distance = get_move_speed();
                return;
            }
        }
        direction = 1;
        distance = 1;
    }
};

extern "C" RobotBase* create_robot() { return new Robot_Ratboy(); }
extern "C" const char* robot_summary() { return "Scans all, moves to enemy, railguns from anywhere."; }
