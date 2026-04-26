#include "Arena.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <dirent.h>
#include <dlfcn.h>
#include <random>
#include <cstring>

Arena::Arena(int r, int c) : rows(r), cols(c), maxRounds(10000), sleepInterval(0.5f),
                             liveView(true), currentRound(0), gameOver(false) {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    initBoard();
}

Arena::~Arena() = default;

void Arena::loadConfig(const std::string& configFile) {
    std::ifstream file(configFile);
    if (!file) {
        std::cerr << "Warning: Could not open config file. Using defaults.\n";
        placeObstacles(3, 2, 4);
        return;
    }
    std::string line;
    int flamethrowers = -1, pits = -1, mounds = -1;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) continue;
        std::string key = line.substr(0, colonPos);
        std::string valueStr = line.substr(colonPos + 1);
        auto trim = [](std::string& s) {
            s.erase(0, s.find_first_not_of(" \t\r\n"));
            s.erase(s.find_last_not_of(" \t\r\n") + 1);
        };
        trim(key);
        trim(valueStr);
        if (key == "Arena_Size" || key == "Arena_Size:") {
            std::istringstream iss(valueStr);
            iss >> rows >> cols;
            initBoard();
        } else if (key == "Max_Rounds" || key == "Max_Rounds:") {
            maxRounds = std::stoi(valueStr);
        } else if (key == "Sleep_interval" || key == "Sleep_interval:") {
            sleepInterval = std::stof(valueStr);
        } else if (key == "Game_State_Live" || key == "Game_State_Live:") {
            liveView = (valueStr == "true" || valueStr == "1");
        } else if (key == "Flamethrowers" || key == "Flamethrowers:") {
            flamethrowers = std::max(0, std::stoi(valueStr));
        } else if (key == "Pits" || key == "Pits:") {
            pits = std::max(0, std::stoi(valueStr));
        } else if (key == "Mounds" || key == "Mounds:") {
            mounds = std::max(0, std::stoi(valueStr));
        }
    }
    if (flamethrowers != -1 || pits != -1 || mounds != -1)
        placeObstacles(flamethrowers, pits, mounds);
    else
        placeObstacles(3, 2, 4);
}
void Arena::placeObstacles(int numFlamethrowers, int numPits, int numMounds) {
    auto place = [&](char type, int count) {
        for (int i = 0; i < count; ++i) {
            for (int attempt = 0; attempt < 1000; ++attempt) {
                int r = rand() % rows, c = rand() % cols;
                if (board[r][c] == '.') {
                    board[r][c] = type;
                    break;
                }
            }
        }
    };
    if (numFlamethrowers > 0) place('F', numFlamethrowers);
    if (numPits > 0) place('P', numPits);
    if (numMounds > 0) place('M', numMounds);
}

void Arena::initBoard() {
    board.assign(rows, std::vector<char>(cols, '.'));
}

void Arena::updateBoard() {
    for (auto& rob : robots) {
        if (board[rob.row][rob.col] == rob.symbol || board[rob.row][rob.col] == 'X') {
            board[rob.row][rob.col] = '.';
        }
    }
    for (auto& rob : robots) {
        if (!rob.dead) {
            board[rob.row][rob.col] = rob.symbol;
        } else {
            board[rob.row][rob.col] = 'X';
        }
    }
}

void Arena::printBoard() const {
    std::cout << "\n";
    std::cout << "   ";
    for (int j = 0; j < cols; ++j) std::cout << (j % 10) << " ";
    std::cout << "\n";
    for (int i = 0; i < rows; ++i) {
        std::cout << (i % 10) << "  ";
        for (int j = 0; j < cols; ++j) {
            std::cout << board[i][j] << " ";
        }
        std::cout << "\n";
    }
    std::cout << std::endl;
}

bool Arena::isValidCell(int r, int c) const {
    return r >= 0 && r < rows && c >= 0 && c < cols;
}

bool Arena::isWalkable(int r, int c, bool ignoreRobots) const {
    if (!isValidCell(r, c)) return false;
    char cell = board[r][c];
    if (cell == '.') return true;
    if (cell == 'M' || cell == 'X') return false;
    if (cell == 'P' || cell == 'F') return true;
    if (!ignoreRobots && (cell >= '!' && cell <= '~')) return false;
    return true;
}

Arena::RobotInstance* Arena::getRobotAt(int r, int c) {
    if (!isValidCell(r,c)) return nullptr;
    for (auto& rob : robots) {
        if (!rob.dead && rob.row == r && rob.col == c)
            return &rob;
    }
    return nullptr;
}

int Arena::calculateDamage(WeaponType weapon) {
    switch (weapon) {
        case railgun:   return 10 + rand() % 11;
        case hammer:    return 50 + rand() % 11;
        case grenade:   return 10 + rand() % 31;
        case flamethrower: return 30 + rand() % 21;
        default: return 10;
    }
}

int Arena::applyArmorReduction(int damage, int armor) {
    float reduction = std::min(0.9f, armor * 0.1f);
    return std::max(1, static_cast<int>(damage * (1.0f - reduction)));
}

void Arena::applyDamage(RobotInstance& target, int damage, RobotBase* attacker) {
    if (target.dead) return;
    int armor = target.robot->get_armor();
    int actualDamage = applyArmorReduction(damage, armor);
    target.robot->reduce_armor(1);
    int newHealth = target.robot->take_damage(actualDamage);
    std::cout << target.robot->m_name << " takes " << actualDamage << " damage (health now " << newHealth << ").\n";
    if (newHealth <= 0) {
        target.dead = true;
        board[target.row][target.col] = 'X';
        std::cout << target.robot->m_name << " is destroyed!\n";
    }
}

std::vector<std::pair<int,int>> Arena::getRadarBeamCells(int startRow, int startCol, int direction) {
    std::vector<std::pair<int,int>> cells;
    if (direction == 0) {
        for (int dr = -1; dr <= 1; ++dr)
            for (int dc = -1; dc <= 1; ++dc)
                if (!(dr == 0 && dc == 0)) {
                    int nr = startRow + dr, nc = startCol + dc;
                    if (isValidCell(nr,nc)) cells.emplace_back(nr,nc);
                }
        return cells;
    }
    int dr = directions[direction].first;
    int dc = directions[direction].second;
    int perpDr = (dr != 0) ? 0 : 1;
    int perpDc = (dc != 0) ? 0 : 1;
    int step = 1;
    while (true) {
        int centerR = startRow + dr * step;
        int centerC = startCol + dc * step;
        if (!isValidCell(centerR, centerC)) break;
        cells.emplace_back(centerR, centerC);
        int leftR = centerR - perpDr;
        int leftC = centerC - perpDc;
        if (isValidCell(leftR, leftC)) cells.emplace_back(leftR, leftC);
        int rightR = centerR + perpDr;
        int rightC = centerC + perpDc;
        if (isValidCell(rightR, rightC)) cells.emplace_back(rightR, rightC);
        step++;
    }
    return cells;
}

std::vector<RadarObj> Arena::performRadarScan(const RobotInstance& robot, int direction) {
    std::vector<RadarObj> results;
    auto cells = getRadarBeamCells(robot.row, robot.col, direction);
    for (auto [r,c] : cells) {
        char cell = board[r][c];
        char typeChar = '.';
        if (cell == 'M') typeChar = 'M';
        else if (cell == 'P') typeChar = 'P';
        else if (cell == 'F') typeChar = 'F';
        else if (cell == 'X') typeChar = 'X';
        else if (cell >= '!' && cell <= '~') {
            RobotInstance* rob = getRobotAt(r, c);
            if (rob && !rob->dead) typeChar = 'R';
            else {
                board[r][c] = '.';
                typeChar = '.';
            }
        }
        if (typeChar != '.') results.emplace_back(typeChar, r, c);
    }
    return results;
}

void Arena::handleShoot(RobotInstance& shooter, int targetRow, int targetCol) {
    WeaponType weapon = shooter.robot->get_weapon();
    if (weapon == grenade && shooter.grenadesLeft <= 0) {
        std::cout << shooter.robot->m_name << " has no grenades left!\n";
        return;
    }
    int damage = calculateDamage(weapon);
    std::cout << shooter.robot->m_name << " fires its " << weapon << " at (" << targetRow << "," << targetCol << ") for " << damage << " base damage.\n";
    if (weapon == grenade) {
        shooter.grenadesLeft--;
        shooter.robot->decrement_grenades();
        for (int dr = -1; dr <= 1; ++dr)
            for (int dc = -1; dc <= 1; ++dc) {
                int r = targetRow + dr, c = targetCol + dc;
                if (auto* other = getRobotAt(r,c)) applyDamage(*other, damage, shooter.robot.get());
            }
    }
    else if (weapon == railgun) {
        int dr = (targetRow - shooter.row);
        int dc = (targetCol - shooter.col);
        int stepR = (dr == 0) ? 0 : (dr > 0 ? 1 : -1);
        int stepC = (dc == 0) ? 0 : (dc > 0 ? 1 : -1);
        int r = shooter.row + stepR, c = shooter.col + stepC;
        while (isValidCell(r,c)) {
            if (auto* other = getRobotAt(r,c)) applyDamage(*other, damage, shooter.robot.get());
            r += stepR; c += stepC;
        }
    }
    else if (weapon == hammer) {
        int dr = 0, dc = 0;
        if (targetRow != shooter.row) dr = (targetRow > shooter.row) ? 1 : -1;
        if (targetCol != shooter.col) dc = (targetCol > shooter.col) ? 1 : -1;
        int r = shooter.row + dr, c = shooter.col + dc;
        if (auto* other = getRobotAt(r,c)) applyDamage(*other, damage, shooter.robot.get());
    }
    else if (weapon == flamethrower) {
        int dr = (targetRow - shooter.row);
        int dc = (targetCol - shooter.col);
        if (dr == 0 && dc == 0) return;
        int stepR = (dr == 0) ? 0 : (dr > 0 ? 1 : -1);
        int stepC = (dc == 0) ? 0 : (dc > 0 ? 1 : -1);
        int perpR = (stepR == 0) ? 1 : 0;
        int perpC = (stepC == 0) ? 1 : 0;
        for (int len = 1; len <= 4; ++len) {
            int centerR = shooter.row + stepR * len;
            int centerC = shooter.col + stepC * len;
            if (!isValidCell(centerR, centerC)) break;
            for (int w = -1; w <= 1; ++w) {
                int r = centerR + w * perpR;
                int c = centerC + w * perpC;
                if (auto* other = getRobotAt(r,c)) applyDamage(*other, damage, shooter.robot.get());
            }
        }
    }
    if (weapon == grenade) {
        std::cout << shooter.robot->m_name << " has " << shooter.grenadesLeft << " grenades left.\n";
    }
}

void Arena::handleMove(RobotInstance& robot, int direction, int distance) {
    int maxSpeed = robot.robot->get_move_speed();
    int steps = std::min(distance, maxSpeed);
    if (steps == 0) return;
    int dr = directions[direction].first;
    int dc = directions[direction].second;
    int newRow = robot.row, newCol = robot.col;
    for (int s = 1; s <= steps; ++s) {
        int nextRow = robot.row + dr * s;
        int nextCol = robot.col + dc * s;
        if (!isWalkable(nextRow, nextCol, false)) break;
        newRow = nextRow;
        newCol = nextCol;
    }
    if (newRow == robot.row && newCol == robot.col) {
        std::cout << robot.robot->m_name << " cannot move from (" << newRow << "," << newCol << ").\n";
        return;
    }
    board[robot.row][robot.col] = '.';
    robot.row = newRow;
    robot.col = newCol;
    robot.robot->move_to(newRow, newCol);
    board[newRow][newCol] = robot.symbol;
    std::cout << robot.robot->m_name << " moves to (" << newRow << "," << newCol << ").\n";
    char cell = board[newRow][newCol];
    applyObstacleEffect(robot, cell);
}

void Arena::applyObstacleEffect(RobotInstance& robot, char obstacle) {
    if (obstacle == 'P') {
        std::cout << robot.robot->m_name << " falls into a pit and can no longer move!\n";
        robot.robot->disable_movement();
    } else if (obstacle == 'F') {
        int damage = 30 + rand() % 21;
        std::cout << robot.robot->m_name << " steps on a flamethrower trap and takes " << damage << " damage!\n";
        applyDamage(robot, damage, nullptr);
        if (robot.dead) {
            board[robot.row][robot.col] = '.';
        }
    }
}

void Arena::loadRobotsFromDir(const std::string& dirPath) {
    DIR* dir = opendir(dirPath.c_str());
    if (!dir) { std::cerr << "Cannot open directory " << dirPath << std::endl; return; }
    struct dirent* entry;
    std::vector<std::string> robotFiles;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name.find("Robot_") == 0 && name.find(".cpp") != std::string::npos)
            robotFiles.push_back(name);
    }
    closedir(dir);
    char symbols[] = {'@', '#', '!', '$', '%', '&', '+', '=', '?', '^'};
    int symIdx = 0;
    for (const auto& fileName : robotFiles) {
        std::string baseName = fileName.substr(0, fileName.find_last_of('.'));
        std::string cppPath = dirPath + "/" + fileName;
        std::string soPath = dirPath + "/" + baseName + ".so";
        std::string compileCmd = "g++ -shared -fPIC -o " + soPath + " " + cppPath + " RobotBase.o -I. -std=c++20";
        if (system(compileCmd.c_str()) != 0) {
            std::cerr << "Compilation failed for " << fileName << std::endl;
            continue;
        }
        void* handle = dlopen(soPath.c_str(), RTLD_LAZY);
        if (!handle) { std::cerr << "dlopen failed: " << dlerror() << std::endl; continue; }
        auto create = (RobotBase*(*)()) dlsym(handle, "create_robot");
        auto summary = (const char*(*)()) dlsym(handle, "robot_summary");
        if (!create || !summary) { std::cerr << "Missing exports in " << fileName << std::endl; dlclose(handle); continue; }
        const char* summ = summary();
        if (!summ || strlen(summ) < 1 || strlen(summ) > 50) { std::cerr << "Invalid summary length in " << fileName << std::endl; dlclose(handle); continue; }
        RobotBase* rawRobot = create();
        rawRobot->set_boundaries(rows, cols);
        bool placed = false;
        for (int attempt = 0; attempt < 500; ++attempt) {
            int r = rand() % rows, c = rand() % cols;
            if (board[r][c] == '.') {
                rawRobot->move_to(r, c);
                rawRobot->set_boundaries(rows, cols);
                RobotInstance inst;
                inst.robot.reset(rawRobot);
                inst.row = r; inst.col = c; inst.dead = false;
                inst.grenadesLeft = (rawRobot->get_weapon() == grenade) ? 10 : 0;
                inst.symbol = symbols[symIdx++ % (sizeof(symbols)/sizeof(symbols[0]))];
                robots.push_back(std::move(inst));
                board[r][c] = inst.symbol;
                placed = true;
                break;
            }
        }
        if (!placed) { std::cerr << "No free cell for robot " << rawRobot->m_name << std::endl; delete rawRobot; dlclose(handle); }
    }
}

void Arena::runGame() {
    while (!gameOver && currentRound < maxRounds) {
        currentRound++;
        std::cout << "\n=========== Starting round " << currentRound << " ===========\n";
        updateBoard();
        printBoard();

        int aliveCount = 0;
        for (auto& r : robots) if (!r.dead) aliveCount++;
        if (aliveCount <= 1) {
            gameOver = true;
            break;
        }

        for (size_t i = 0; i < robots.size(); ++i) {
            auto& rob = robots[i];
            if (rob.dead) continue;
            std::cout << "\n" << rob.robot->print_stats() << std::endl;
            int radarDir = 0;
            rob.robot->get_radar_direction(radarDir);
            auto radarResults = performRadarScan(rob, radarDir);
            rob.robot->process_radar_results(radarResults);
            int shootRow = 0, shootCol = 0;
            bool willShoot = rob.robot->get_shot_location(shootRow, shootCol);
            if (willShoot) {
                handleShoot(rob, shootRow, shootCol);
            } else {
                int moveDir = 0, moveDist = 0;
                rob.robot->get_move_direction(moveDir, moveDist);
                if (moveDir != 0 && moveDist > 0) {
                    handleMove(rob, moveDir, moveDist);
                } else {
                    std::cout << rob.robot->m_name << " does nothing this turn.\n";
                }
            }
            if (liveView) usleep(static_cast<useconds_t>(sleepInterval * 1000000));
        }
    }

    std::cout << "\n===== GAME OVER =====\n";
    int finalAlive = 0;
    std::string finalWinner;
    for (auto& r : robots) {
        if (!r.dead) {
            finalAlive++;
            finalWinner = r.robot->m_name;
        }
    }
    if (finalAlive == 1) {
        std::cout << "Winner: " << finalWinner << "!\n";
    } else if (finalAlive == 0) {
        std::cout << "No winner (all robots destroyed).\n";
    } else {
        std::cout << "No winner (ran out of rounds, " << finalAlive << " robots remain).\n";
    }
}
