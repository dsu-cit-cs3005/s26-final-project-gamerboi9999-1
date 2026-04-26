#pragma once

#include <vector>
#include <memory>
#include <string>
#include "RobotBase.h"

class Arena {
public:
    Arena(int rows, int cols);
    ~Arena();

    void loadConfig(const std::string& configFile);
    void loadRobotsFromDir(const std::string& dirPath);
    void runGame();

private:
    struct RobotInstance {
        std::unique_ptr<RobotBase> robot;
        int row, col;
        int grenadesLeft;
        bool dead;
        char symbol;
    };

    int rows, cols;
    int maxRounds;
    float sleepInterval;
    bool liveView;

    std::vector<std::vector<char>> board;
    std::vector<RobotInstance> robots;
    int currentRound;
    bool gameOver;
    std::string winnerName;

    void placeObstacles(int numFlamethrowers, int numPits, int numMounds);
    void initBoard();
    void updateBoard();
    void printBoard() const;
    void applyDamage(RobotInstance& target, int damage, RobotBase* attacker);
    std::vector<RadarObj> performRadarScan(const RobotInstance& robot, int direction);
    void handleShoot(RobotInstance& shooter, int targetRow, int targetCol);
    void handleMove(RobotInstance& robot, int direction, int distance);
    bool isValidCell(int r, int c) const;
    bool isWalkable(int r, int c, bool ignoreRobots = false) const;
    RobotInstance* getRobotAt(int r, int c);
    void applyObstacleEffect(RobotInstance& robot, char obstacle);
    int calculateDamage(WeaponType weapon);
    int applyArmorReduction(int damage, int armor);

    std::vector<std::pair<int,int>> getRadarBeamCells(int startRow, int startCol, int direction);
};
