#include "RobotBase.h"
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <limits>
#include <algorithm>

class Robot_ANSI : public RobotBase
{
private:
    int targetRow = -1, targetCol = -1;
    bool hasTarget = false;
    int flipCooldown = 0;

    int distance(int r1, int c1, int r2, int c2) const {
        return std::abs(r1 - r2) + std::abs(c1 - c2);
    }

    bool inside(int row, int col) const {
        return row >= 0 && row < m_board_row_max && col >= 0 && col < m_board_col_max;
    }

public:
    Robot_ANSI() : RobotBase(4, 3, railgun) {
        m_name = "ANSI";
        std::srand(static_cast<unsigned>(std::time(nullptr)));
    }

    void get_radar_direction(int& radar_direction) override {
        radar_direction = 0;
    }

    void process_radar_results(const std::vector<RadarObj>& results) override {
        int currentRow, currentCol;
        get_current_location(currentRow, currentCol);
        hasTarget = false;
        int closestDist = std::numeric_limits<int>::max();
        for (const auto& obj : results) {
            if (obj.m_type == 'R') {
                int d = distance(currentRow, currentCol, obj.m_row, obj.m_col);
                if (d < closestDist) {
                    closestDist = d;
                    targetRow = obj.m_row;
                    targetCol = obj.m_col;
                    hasTarget = true;
                }
            } else if (obj.m_type == 'X' && distance(currentRow, currentCol, obj.m_row, obj.m_col) == 1) {
            }
        }
        if (hasTarget && closestDist > 5)
    }

    bool get_shot_location(int& shotRow, int& shotCol) override {
        if (!hasTarget) return false;
        int currentRow, currentCol;
        get_current_location(currentRow, currentCol);
        int dist = distance(currentRow, currentCol, targetRow, targetCol);
        if (flipCooldown == 0 && dist <= 2 && (std::rand() % 3 == 0)) {
            flipCooldown = 3;
        } else if (flipCooldown > 0) {
            flipCooldown--;
        }
        shotRow = targetRow;
        shotCol = targetCol;
        return true;
    }

    void get_move_direction(int& direction, int& distance) override {
        int currentRow, currentCol;
        get_current_location(currentRow, currentCol);

        if (hasTarget) {
            int dr = (targetRow > currentRow) ? 1 : (targetRow < currentRow) ? -1 : 0;
            int dc = (targetCol > currentCol) ? 1 : (targetCol < currentCol) ? -1 : 0;
            if      (dr == -1 && dc == 0) direction = 1;
            else if (dr == -1 && dc == 1) direction = 2;
            else if (dr == 0  && dc == 1) direction = 3;
            else if (dr == 1  && dc == 1) direction = 4;
            else if (dr == 1  && dc == 0) direction = 5;
            else if (dr == 1  && dc == -1) direction = 6;
            else if (dr == 0  && dc == -1) direction = 7;
            else if (dr == -1 && dc == -1) direction = 8;
            else direction = 0;
            int nr = currentRow + directions[direction].first;
            int nc = currentCol + directions[direction].second;
            if (!inside(nr, nc)) {
                for (int d : {5,1,3,7}) {
                    nr = currentRow + directions[d].first;
                    nc = currentCol + directions[d].second;
                    if (inside(nr, nc)) { direction = d; break; }
                }
            }
            distance = get_move_speed();
            return;
        }

        direction = (std::rand() % 8) + 1;
        int nr = currentRow + directions[direction].first;
        int nc = currentCol + directions[direction].second;
        if (!inside(nr, nc)) {
            for (int d = 1; d <= 8; ++d) {
                nr = currentRow + directions[d].first;
                nc = currentCol + directions[d].second;
                if (inside(nr, nc)) {
                    direction = d;
                    break;
                }
            }
        }
        distance = get_move_speed();
    }
};

extern "C" RobotBase* create_robot() { return new Robot_ANSI(); }
extern "C" const char* robot_summary() { return "ANSI: Random movement, fights when enemy spotted."; }
