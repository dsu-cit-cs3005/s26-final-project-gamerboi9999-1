#include "Arena.h"
#include <iostream>
#include <cstdlib>
#include <ctime>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file>\n";
        return 1;
    }
    srand(static_cast<unsigned>(time(nullptr)));

    int rows = 15, cols = 20;
    Arena arena(rows, cols);
    arena.loadConfig(argv[1]);
    arena.loadRobotsFromDir("./robots");
    arena.runGame();

    return 0;
}
