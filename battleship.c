#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define GRID_SIZE 10
#define SHIP_TYPES 4
#define MAX_NAME_LENGTH 20
#define MAX_INPUT_LENGTH 50
#define MAX_RADAR_SWEEPS 3

typedef enum { false, true } bool;

typedef enum {
    EASY,
    MEDIUM,
    HARD
} DifficultyLevel;

typedef struct {
    int x;
    int y;
} Coordinate;

typedef struct {
    Coordinate coord;
    bool active;
} SmokeScreen;

typedef struct {
    char name[20];
    int size;
    int hits;
    bool sunk;
    char symbol;
} Ship;

typedef struct {
    Ship ships[SHIP_TYPES];
} Fleet;

typedef struct {
    char name[MAX_NAME_LENGTH];
    char grid[GRID_SIZE][GRID_SIZE];
    char trackingGrid[GRID_SIZE][GRID_SIZE];
    int radarSweepsUsed;
    int smokeScreensUsed;
    int shipsSunk;
    int shipsRemaining;
    bool artilleryAvailable;
    bool torpedoAvailable;
    bool isBot;
    SmokeScreen smokeScreens[SHIP_TYPES];
    Coordinate potentialTargets[GRID_SIZE * GRID_SIZE];
    int potentialTargetCount;
    Coordinate lastArtilleryCoord;
    int lastArtilleryHits;
    DifficultyLevel difficulty;
    int turnNumber; // Added to track the number of turns
} Player;

void initializePlayer(Player* player, bool isBot, DifficultyLevel difficulty);
void initializeGrid(char grid[GRID_SIZE][GRID_SIZE]);
void displayGrid(char grid[GRID_SIZE][GRID_SIZE], bool showShips);
void placeShips(Player* player, Fleet* fleet);
void placeShipsBot(Player* bot, Fleet* fleet);
bool isValidPlacement(char grid[GRID_SIZE][GRID_SIZE], Coordinate coord, int size, char orientation);
void placeShipOnGrid(char grid[GRID_SIZE][GRID_SIZE], Coordinate coord, int size, char orientation, char symbol);
Coordinate parseCoordinate(const char* input);
void clearScreen();
void gameLoop(Player* currentPlayer, Player* opponent, Fleet* currentFleet, Fleet* opponentFleet, bool hardMode);
void performMove(Player* player, Player* opponent, Fleet* opponentFleet, bool hardMode);
void performBotMove(Player* bot, Player* opponent, Fleet* opponentFleet, bool hardMode);
int fire(Player* player, Player* opponent, Fleet* opponentFleet, Coordinate coord, bool hardMode, char* sunkShipName);
void radarSweep(Player* player, Player* opponent, Coordinate coord);
bool smokeScreen(Player* player, Coordinate coord);
void artillery(Player* player, Player* opponent, Fleet* opponentFleet, Coordinate coord, bool hardMode);
void torpedo(Player* player, Player* opponent, Fleet* opponentFleet, const char* input, bool hardMode);
bool checkWin(Fleet* fleet);
void updateShipStatus(Ship* ship);
void unlockSpecialMoves(Player* player, Player* opponent);
void displayTrackingGrid(Player* player, bool hardMode);
bool isValidCommand(const char* command, Player* player);
void getInput(char* input, int size);
void coordinateToString(Coordinate coord, char* coordStr);
void toLowerCase(char* str);
void flushInputBuffer();
int getRandomNumber(int min, int max);
Coordinate getRandomCoordinate();
Coordinate getNextTarget(Player* bot, Fleet* opponentFleet);
void addAdjacentTargets(Player* bot, Coordinate coord);
void calculateProbabilityGrid(Player* bot, Fleet* opponentFleet, int probabilityGrid[GRID_SIZE][GRID_SIZE]);
Coordinate getBestArtilleryTarget(Player* bot);
int countUntargetedTilesInArtilleryArea(Player* bot, Coordinate coord);
bool chooseTorpedoTarget(Player* bot, Player* opponent, Fleet* opponentFleet, bool hardMode);
void addPotentialTarget(Player* player, Coordinate coord);
Coordinate getSmokeScreenCoordinateForBot(Player* bot);
void handleEdgeCoordinates(int* start, int* end);

int main() {
    srand((unsigned int)time(NULL));
    Player player1, botPlayer;
    Fleet fleet1, fleet2;
    bool hardMode = false;
    char difficultyInput[MAX_INPUT_LENGTH];
    char botDifficultyInput[MAX_INPUT_LENGTH];
    DifficultyLevel botDifficulty = MEDIUM; // Default bot difficulty

    printf("Choose bot difficulty level (easy/medium/hard): ");
    getInput(botDifficultyInput, sizeof(botDifficultyInput));
    toLowerCase(botDifficultyInput);
    if (strcmp(botDifficultyInput, "easy") == 0) {
        botDifficulty = EASY;
    } else if (strcmp(botDifficultyInput, "medium") == 0) {
        botDifficulty = MEDIUM;
    } else if (strcmp(botDifficultyInput, "hard") == 0) {
        botDifficulty = HARD;
    } else {
        printf("Invalid input. Defaulting to medium difficulty.\n");
    }

    printf("Choose tracking difficulty level (easy/hard): ");
    getInput(difficultyInput, sizeof(difficultyInput));
    toLowerCase(difficultyInput);
    if (strcmp(difficultyInput, "hard") == 0) {
        hardMode = true;
    }

    do {
        printf("Enter your name: ");
        getInput(player1.name, sizeof(player1.name));
        if (strlen(player1.name) == 0) {
            printf("Name cannot be empty. Please enter a valid name.\n");
        }
    } while (strlen(player1.name) == 0);

    strcpy(botPlayer.name, "Bot");

    initializePlayer(&player1, false, MEDIUM);
    initializePlayer(&botPlayer, true, botDifficulty);

    Player* currentPlayer = (rand() % 2 == 0) ? &player1 : &botPlayer;
    Player* opponent = (currentPlayer == &player1) ? &botPlayer : &player1;

    Fleet* currentFleet = (currentPlayer == &player1) ? &fleet1 : &fleet2;
    Fleet* opponentFleet = (currentPlayer == &player1) ? &fleet2 : &fleet1;

    printf("%s will play first.\n", currentPlayer->name);

    const Ship defaultShips[SHIP_TYPES] = {
        {"Carrier", 5, 0, false, 'C'},
        {"Battleship", 4, 0, false, 'B'},
        {"Destroyer", 3, 0, false, 'D'},
        {"Submarine", 2, 0, false, 'S'}
    };
    memcpy(fleet1.ships, defaultShips, sizeof(defaultShips));
    memcpy(fleet2.ships, defaultShips, sizeof(defaultShips));

    placeShips(&player1, &fleet1);
    clearScreen();
    placeShipsBot(&botPlayer, &fleet2);
    clearScreen();

    gameLoop(currentPlayer, opponent, currentFleet, opponentFleet, hardMode);

    return 0;
}

void initializePlayer(Player* player, bool isBot, DifficultyLevel difficulty) {
    initializeGrid(player->grid);
    initializeGrid(player->trackingGrid);
    player->radarSweepsUsed = 0;
    player->smokeScreensUsed = 0;
    player->shipsSunk = 0;
    player->shipsRemaining = SHIP_TYPES;
    player->artilleryAvailable = false;
    player->torpedoAvailable = false;
    player->isBot = isBot;
    player->potentialTargetCount = 0;
    player->lastArtilleryHits = 0;
    player->lastArtilleryCoord.x = -1;
    player->lastArtilleryCoord.y = -1;
    player->difficulty = difficulty;
    player->turnNumber = 0; // Initialize turn number
    for (int i = 0; i < SHIP_TYPES; i++) {
        player->smokeScreens[i].active = false;
    }
}

void initializeGrid(char grid[GRID_SIZE][GRID_SIZE]) {
    memset(grid, '~', sizeof(char) * GRID_SIZE * GRID_SIZE);
}

void displayGrid(char grid[GRID_SIZE][GRID_SIZE], bool showShips) {
    printf("   A B C D E F G H I J\n");
    for (int i = 0; i < GRID_SIZE; i++) {
        printf("%2d", i + 1);
        for (int j = 0; j < GRID_SIZE; j++) {
            char cell = grid[i][j];
            if (!showShips && cell >= 'A' && cell <= 'Z') {
                printf(" ~");
            } else {
                printf(" %c", cell);
            }
        }
        printf("\n");
    }
}

void placeShips(Player* player, Fleet* fleet) {
    if (player->isBot) {
        placeShipsBot(player, fleet);
        return;
    }

    char input[MAX_INPUT_LENGTH], orientation[MAX_INPUT_LENGTH];
    Coordinate coord;

    printf("%s, place your ships on the grid.\n", player->name);
    for (int i = 0; i < SHIP_TYPES; i++) {
        bool placed = false;
        while (!placed) {
            displayGrid(player->grid, true);
            printf("Enter coordinates and orientation (horizontal/vertical) for %s (size %d): ",
                   fleet->ships[i].name, fleet->ships[i].size);

            getInput(input, sizeof(input));
            char* token = strtok(input, " ");
            if (!token) {
                printf("Invalid input format.\n");
                continue;
            }
            strcpy(input, token);

            token = strtok(NULL, " ");
            if (!token) {
                printf("Invalid input format.\n");
                continue;
            }
            strcpy(orientation, token);

            toLowerCase(input);
            toLowerCase(orientation);

            coord = parseCoordinate(input);
            if (coord.x == -1 || coord.y == -1) {
                printf("Invalid coordinates.\n");
                continue;
            }

            char dir = tolower(orientation[0]);
            if (dir != 'h' && dir != 'v') {
                printf("Invalid orientation.\n");
                continue;
            }

            if (isValidPlacement(player->grid, coord, fleet->ships[i].size, dir)) {
                placeShipOnGrid(player->grid, coord, fleet->ships[i].size, dir, fleet->ships[i].symbol);
                placed = true;
                clearScreen();
            } else {
                printf("Invalid placement.\n");
            }
        }
    }
}

void placeShipsBot(Player* bot, Fleet* fleet) {
    for (int i = 0; i < SHIP_TYPES; i++) {
        bool placed = false;
        while (!placed) {
            Coordinate coord = getRandomCoordinate();
            char orientation = (rand() % 2 == 0) ? 'h' : 'v';
            if (isValidPlacement(bot->grid, coord, fleet->ships[i].size, orientation)) {
                placeShipOnGrid(bot->grid, coord, fleet->ships[i].size, orientation, fleet->ships[i].symbol);
                placed = true;
            }
        }
    }
}

bool isValidPlacement(char grid[GRID_SIZE][GRID_SIZE], Coordinate coord, int size, char orientation) {
    int x = coord.x;
    int y = coord.y;

    if (orientation == 'h') {
        if (x + size > GRID_SIZE) return false;
        for (int i = 0; i < size; i++)
            if (grid[y][x + i] != '~') return false;
    } else if (orientation == 'v') {
        if (y + size > GRID_SIZE) return false;
        for (int i = 0; i < size; i++)
            if (grid[y + i][x] != '~') return false;
    } else {
        return false;
    }
    return true;
}

void placeShipOnGrid(char grid[GRID_SIZE][GRID_SIZE], Coordinate coord, int size, char orientation, char symbol) {
    int x = coord.x;
    int y = coord.y;

    for (int i = 0; i < size; i++) {
        if (orientation == 'h') {
            grid[y][x + i] = symbol;
        } else {
            grid[y + i][x] = symbol;
        }
    }
}

Coordinate parseCoordinate(const char* input) {
    Coordinate coord = { -1, -1 };
    if (strlen(input) < 2 || strlen(input) > 3) return coord;

    char col = input[0];
    int row = atoi(&input[1]);

    if (isalpha(col)) {
        coord.x = tolower(col) - 'a';
    } else {
        return coord;
    }

    if (row >= 1 && row <= GRID_SIZE) {
        coord.y = row - 1;
    }

    if (coord.x < 0 || coord.x >= GRID_SIZE || coord.y < 0 || coord.y >= GRID_SIZE) {
        coord.x = coord.y = -1;
    }

    return coord;
}

void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void gameLoop(Player* currentPlayer, Player* opponent, Fleet* currentFleet, Fleet* opponentFleet, bool hardMode) {
    while (true) {
        if (currentPlayer->isBot) {
            performBotMove(currentPlayer, opponent, opponentFleet, hardMode);
        } else {
            performMove(currentPlayer, opponent, opponentFleet, hardMode);
        }

        if (checkWin(opponentFleet)) {
            printf("%s wins!\n", currentPlayer->name);
            break;
        }

        Player* tempPlayer = currentPlayer;
        currentPlayer = opponent;
        opponent = tempPlayer;

        Fleet* tempFleet = currentFleet;
        currentFleet = opponentFleet;
        opponentFleet = tempFleet;
    }
}

void performMove(Player* player, Player* opponent, Fleet* opponentFleet, bool hardMode) {
    clearScreen();
    char input[MAX_INPUT_LENGTH];
    bool validMove = false;

    while (!validMove) {
        printf("%s's turn.\n", player->name);
        displayTrackingGrid(player, hardMode);
        printf("Available moves:\n");
        printf("1. Fire [coordinate]\n");
        printf("2. Radar [coordinate] (Used %d/%d)\n", player->radarSweepsUsed, MAX_RADAR_SWEEPS);
        if (player->smokeScreensUsed < player->shipsSunk) {
            printf("3. Smoke [coordinate] (Used %d)\n", player->smokeScreensUsed);
        }
        if (player->artilleryAvailable) {
            printf("4. Artillery [coordinate]\n");
        }
        if (player->torpedoAvailable) {
            printf("5. Torpedo [row/column]\n");
        }
        printf("Enter your move: ");
        getInput(input, sizeof(input));
        toLowerCase(input);

        char* command = strtok(input, " ");
        char* argument = strtok(NULL, " ");

        if (!command || !argument) {
            printf("Invalid input format.\n");
            return;
        }

        if (!isValidCommand(command, player)) {
            printf("Invalid command or command not available.\n");
            return;
        }

        if (strcmp(command, "fire") == 0) {
            Coordinate coord = parseCoordinate(argument);
            if (coord.x != -1 && coord.y != -1) {
                char sunkShipName[20] = "";
                int result = fire(player, opponent, opponentFleet, coord, hardMode, sunkShipName);
                if (result == 0) {
                    printf("Miss!\n");
                } else if (result == 1) {
                    printf("Hit!\n");
                } else if (result == 2) {
                    printf("Hit!\nYou sunk the opponent's %s!\n", sunkShipName);
                    unlockSpecialMoves(player, opponent);
                } else if (result == 3) {
                    printf("Already targeted this coordinate.\n");
                }
                validMove = true;
                printf("Press Enter to continue...");
                getchar();
            } else {
                printf("Invalid coordinates.\n");
                return;
            }
        } else if (strcmp(command, "radar") == 0) {
            if (player->radarSweepsUsed < MAX_RADAR_SWEEPS) {
                Coordinate coord = parseCoordinate(argument);
                if (coord.x != -1 && coord.y != -1) {
                    radarSweep(player, opponent, coord);
                    player->radarSweepsUsed++;
                    validMove = true;
                    printf("Press Enter to continue...");
                    getchar();
                } else {
                    printf("Invalid coordinates.\n");
                    return;
                }
            } else {
                printf("Radar sweeps limit reached.\n");
                return;
            }
        } else if (strcmp(command, "smoke") == 0) {
            Coordinate coord = parseCoordinate(argument);
            if (coord.x != -1 && coord.y != -1) {
                if (smokeScreen(player, coord)) {
                    validMove = true;
                    printf("Press Enter to continue...");
                    getchar();
                } else {
                    return;
                }
            } else {
                printf("Invalid coordinates.\n");
                return;
            }
        } else if (strcmp(command, "artillery") == 0) {
            Coordinate coord = parseCoordinate(argument);
            if (coord.x != -1 && coord.y != -1) {
                artillery(player, opponent, opponentFleet, coord, hardMode);
                player->artilleryAvailable = false;
                validMove = true;
                printf("Press Enter to continue...");
                getchar();
            } else {
                printf("Invalid coordinates.\n");
                return;
            }
        } else if (strcmp(command, "torpedo") == 0) {
            if (argument[0] != '\0') {
                torpedo(player, opponent, opponentFleet, argument, hardMode);
                player->torpedoAvailable = false;
                validMove = true;
                printf("Press Enter to continue...");
                getchar();
            } else {
                printf("Invalid input.\n");
                return;
            }
        }
    }
}

void performBotMove(Player* bot, Player* opponent, Fleet* opponentFleet, bool hardMode) {
    printf("%s's turn.\n", bot->name);

    bot->turnNumber++; // Increment bot's turn number

    Coordinate coord;
    char sunkShipName[20] = "";
    int result = -1;

    bool moveMade = false;

    if (bot->difficulty == EASY) {
        // FIRE is a priority
        // No targeting mode after a hit, unless radar has found enemy ships

        // Radar is done at the 6-10th turn at every 10 turns
        int turnInInterval = (bot->turnNumber - 1) % 10 + 1;

        // Use Radar if allowed and available
        if (!moveMade && bot->radarSweepsUsed < MAX_RADAR_SWEEPS &&
            turnInInterval >= 6 && turnInInterval <= 10) {

            coord = getRandomCoordinate();
            printf("%s uses Radar at ", bot->name);
            char coordStr[5];
            coordinateToString(coord, coordStr);
            printf("%s\n", coordStr);
            radarSweep(bot, opponent, coord);
            bot->radarSweepsUsed++;
            moveMade = true;

            // Bot can enter targeting mode after radar finds ships
        }

        // Artillery is done only at the 7-10th turn at every 10 turns when it's available
        if (!moveMade && bot->artilleryAvailable &&
            turnInInterval >= 7 && turnInInterval <= 10) {

            coord = getBestArtilleryTarget(bot);
            printf("%s uses Artillery at ", bot->name);
            char coordStr[5];
            coordinateToString(coord, coordStr);
            printf("%s\n", coordStr);
            artillery(bot, opponent, opponentFleet, coord, hardMode);
            bot->artilleryAvailable = false;
            moveMade = true;
        }

        // Smoke is at 10th turn at every 10 turns when it's available
        if (!moveMade && bot->smokeScreensUsed < bot->shipsSunk &&
            turnInInterval == 10) {

            Coordinate smokeCoord = getSmokeScreenCoordinateForBot(bot);
            if (smokeCoord.x != -1 && smokeCoord.y != -1 && smokeScreen(bot, smokeCoord)) {
                printf("%s deployed a smoke screen.\n", bot->name);
                moveMade = true;
            }
        }

        // Torpedo is done only at the 10-15th turn at every 15 turns when it's available
        int turnInInterval15 = (bot->turnNumber - 1) % 15 + 1;
        if (!moveMade && bot->torpedoAvailable &&
            turnInInterval15 >= 10 && turnInInterval15 <= 15) {

            if (!chooseTorpedoTarget(bot, opponent, opponentFleet, hardMode)) {
                // Fallback to fire if no valid torpedo target
                coord = getNextTarget(bot, opponentFleet);
                if (coord.x != -1 && coord.y != -1) {
                    printf("%s fires at ", bot->name);
                    char coordStr[5];
                    coordinateToString(coord, coordStr);
                    printf("%s\n", coordStr);
                    result = fire(bot, opponent, opponentFleet, coord, hardMode, sunkShipName);
                } else {
                    printf("%s has no valid targets to fire.\n", bot->name);
                }
            }
            bot->torpedoAvailable = false;
            moveMade = true;
        }

        // Targeting Mode after radar has found enemy ships
        if (!moveMade && bot->potentialTargetCount > 0) {
            coord = bot->potentialTargets[--bot->potentialTargetCount];
            printf("%s fires at ", bot->name);
            char coordStr[5];
            coordinateToString(coord, coordStr);
            printf("%s (Targeting mode)\n", coordStr);
            result = fire(bot, opponent, opponentFleet, coord, hardMode, sunkShipName);
            moveMade = true;
        }

        // Fire
        if (!moveMade) {
            coord = getNextTarget(bot, opponentFleet);
            if (coord.x != -1 && coord.y != -1) {
                printf("%s fires at ", bot->name);
                char coordStr[5];
                coordinateToString(coord, coordStr);
                printf("%s\n", coordStr);
                result = fire(bot, opponent, opponentFleet, coord, hardMode, sunkShipName);
                moveMade = true;
            } else {
                printf("%s has no valid targets to fire.\n", bot->name);
            }
        }

        // Process fire result
        if (result != -1) {
            if (result == 0) {
                printf("Miss!\n");
            } else if (result == 1) {
                printf("Hit!\n");
                // Do not add adjacent targets in EASY difficulty after a hit
            } else if (result == 2) {
                printf("%s sunk your %s!\n", bot->name, sunkShipName);
                bot->potentialTargetCount = 0;
                unlockSpecialMoves(bot, opponent);
            } else if (result == 3) {
                printf("Already targeted this coordinate.\n");
            }
        }

    } else {
        // Determine move probabilities based on difficulty level
        int radarChance, artilleryChance, torpedoChance, smokeChance;
        switch (bot->difficulty) {
            case MEDIUM:
                radarChance = 50;
                artilleryChance = 35;
                torpedoChance = 30;
                smokeChance = 30;
                break;
            case HARD:
                radarChance = 50;
                artilleryChance = 100; // Aggressive use
                torpedoChance = 100;
                smokeChance = 100;
                break;
            default:
                radarChance = 0;
                artilleryChance = 0;
                torpedoChance = 0;
                smokeChance = 0;
                break;
        }

        // Smoke Screen
        if (bot->smokeScreensUsed < bot->shipsSunk && !moveMade && (rand() % 100) < smokeChance) {
            Coordinate smokeCoord = getSmokeScreenCoordinateForBot(bot);
            if (smokeCoord.x != -1 && smokeCoord.y != -1 && smokeScreen(bot, smokeCoord)) {
                printf("%s deployed a smoke screen.\n", bot->name);
                moveMade = true;
            }
        }

        // Artillery
        if (bot->artilleryAvailable && !moveMade && (rand() % 100) < artilleryChance) {
            coord = getBestArtilleryTarget(bot);
            printf("%s uses Artillery at ", bot->name);
            char coordStr[5];
            coordinateToString(coord, coordStr);
            printf("%s\n", coordStr);
            artillery(bot, opponent, opponentFleet, coord, hardMode);
            bot->artilleryAvailable = false;
            moveMade = true;
        }

        // Torpedo
        if (bot->torpedoAvailable && !moveMade && (rand() % 100) < torpedoChance) {
            if (!chooseTorpedoTarget(bot, opponent, opponentFleet, hardMode)) {
                // Fallback to fire if no valid torpedo target
                coord = getNextTarget(bot, opponentFleet);
                if (coord.x != -1 && coord.y != -1) {
                    printf("%s fires at ", bot->name);
                    char coordStr[5];
                    coordinateToString(coord, coordStr);
                    printf("%s\n", coordStr);
                    result = fire(bot, opponent, opponentFleet, coord, hardMode, sunkShipName);
                } else {
                    printf("%s has no valid targets to fire.\n", bot->name);
                }
            }
            bot->torpedoAvailable = false;
            moveMade = true;
        }

        // Radar
        if (!moveMade && bot->radarSweepsUsed < MAX_RADAR_SWEEPS && (rand() % 100) < radarChance) {
            coord = getRandomCoordinate();
            printf("%s uses Radar at ", bot->name);
            char coordStr[5];
            coordinateToString(coord, coordStr);
            printf("%s\n", coordStr);
            radarSweep(bot, opponent, coord);
            bot->radarSweepsUsed++;
            moveMade = true;
        }

        // Targeting Mode
        if (!moveMade && bot->potentialTargetCount > 0) {
            coord = bot->potentialTargets[--bot->potentialTargetCount];
            printf("%s fires at ", bot->name);
            char coordStr[5];
            coordinateToString(coord, coordStr);
            printf("%s (Targeting mode)\n", coordStr);
            result = fire(bot, opponent, opponentFleet, coord, hardMode, sunkShipName);
            moveMade = true;
        }

        // Fire
        if (!moveMade) {
            coord = getNextTarget(bot, opponentFleet);
            if (coord.x != -1 && coord.y != -1) {
                printf("%s fires at ", bot->name);
                char coordStr[5];
                coordinateToString(coord, coordStr);
                printf("%s\n", coordStr);
                result = fire(bot, opponent, opponentFleet, coord, hardMode, sunkShipName);
                moveMade = true;
            } else {
                printf("%s has no valid targets to fire.\n", bot->name);
            }
        }

        // Process fire result
        if (result != -1) {
            if (result == 0) {
                printf("Miss!\n");
            } else if (result == 1) {
                printf("Hit!\n");
                addAdjacentTargets(bot, coord);
            } else if (result == 2) {
                printf("%s sunk your %s!\n", bot->name, sunkShipName);
                bot->potentialTargetCount = 0;
                unlockSpecialMoves(bot, opponent);
            } else if (result == 3) {
                printf("Already targeted this coordinate.\n");
            }
        }
    }

    printf("Press Enter to continue...");
    getchar();
}

int fire(Player* player, Player* opponent, Fleet* opponentFleet, Coordinate coord, bool hardMode, char* sunkShipName) {

    char cell = opponent->grid[coord.y][coord.x];

    if (cell == '~') {
        opponent->grid[coord.y][coord.x] = 'o';
        if (!hardMode || player->isBot) {
            player->trackingGrid[coord.y][coord.x] = 'o';
        }
        return 0;
    } else if (cell >= 'A' && cell <= 'Z') {
        if (opponent->grid[coord.y][coord.x] == 'X') {
            return 3;
        }
        opponent->grid[coord.y][coord.x] = 'X';
        player->trackingGrid[coord.y][coord.x] = '*';

        for (int i = 0; i < SHIP_TYPES; i++) {
            if (opponentFleet->ships[i].symbol == cell) {
                opponentFleet->ships[i].hits++;
                updateShipStatus(&opponentFleet->ships[i]);
                if (opponentFleet->ships[i].sunk) {
                    strcpy(sunkShipName, opponentFleet->ships[i].name);
                    player->shipsSunk++;
                    opponent->shipsRemaining--;
                    return 2;
                }
                return 1;
            }
        }
    } else if (cell == 'o' || cell == 'X') {
        return 3;
    }
    return -1;
}

void radarSweep(Player* player, Player* opponent, Coordinate coord) {
    if (coord.x < 0 || coord.x >= GRID_SIZE || coord.y < 0 || coord.y >= GRID_SIZE) {
        printf("Invalid coordinates for radar sweep.\n");
        return;
    }

    int xStart = coord.x;
    int yStart = coord.y;
    int xEnd = coord.x + 1;
    int yEnd = coord.y + 1;

    handleEdgeCoordinates(&xStart, &xEnd);
    handleEdgeCoordinates(&yStart, &yEnd);

    for (int i = 0; i < opponent->smokeScreensUsed; i++) {
        if (opponent->smokeScreens[i].active) {
            Coordinate smokeCoord = opponent->smokeScreens[i].coord;
            int sxStart = smokeCoord.x;
            int syStart = smokeCoord.y;
            int sxEnd = smokeCoord.x + 1;
            int syEnd = smokeCoord.y + 1;

            handleEdgeCoordinates(&sxStart, &sxEnd);
            handleEdgeCoordinates(&syStart, &syEnd);

            if (!(xEnd < sxStart || xStart > sxEnd || yEnd < syStart || yStart > syEnd)) {
                printf("Radar sweep found no enemy ships (area obscured by smoke).\n");
                opponent->smokeScreens[i].active = false;
                return;
            }
        }
    }

    bool found = false;
    for (int i = yStart; i <= yEnd; i++) {
        for (int j = xStart; j <= xEnd; j++) {
            if (opponent->grid[i][j] >= 'A' && opponent->grid[i][j] <= 'Z') {
                found = true;
                if (player->isBot) {
                    Coordinate targetCoord = { j, i };
                    addPotentialTarget(player, targetCoord);
                }
            }
        }
    }

    if (found) {
        printf("Radar detected enemy ships near the target area.\n");
    } else {
        printf("Radar sweep found no enemy ships.\n");
    }
}

bool smokeScreen(Player* player, Coordinate coord) {
    if (coord.x < 0 || coord.x >= GRID_SIZE || coord.y < 0 || coord.y >= GRID_SIZE) {
        printf("Invalid coordinates. Smoke screen not deployed.\n");
        return false;
    }

    if (player->smokeScreensUsed >= player->shipsSunk) {
        printf("No smoke screens available. You must sink more ships to use another smoke screen.\n");
        return false;
    }

    player->smokeScreens[player->smokeScreensUsed].coord = coord;
    player->smokeScreens[player->smokeScreensUsed].active = true;
    player->smokeScreensUsed++;
    printf("Smoke screen deployed.\n");
    clearScreen();
    return true;
}

void artillery(Player* player, Player* opponent, Fleet* opponentFleet, Coordinate coord, bool hardMode) {
    int totalHits = 0;
    int totalMisses = 0;
    char sunkShips[SHIP_TYPES][20] = { "" };
    int sunkShipsCount = 0;

    int xStart = coord.x;
    int yStart = coord.y;
    int xEnd = coord.x + 1;
    int yEnd = coord.y + 1;

    handleEdgeCoordinates(&xStart, &xEnd);
    handleEdgeCoordinates(&yStart, &yEnd);

    printf("Artillery strike results at %c%d:\n", 'A' + coord.x, coord.y + 1);
    for (int i = yStart; i <= yEnd; i++) {
        for (int j = xStart; j <= xEnd; j++) {
            Coordinate tempCoord = { j, i };
            char sunkShipName[20] = "";
            int result = fire(player, opponent, opponentFleet, tempCoord, hardMode, sunkShipName);
            if (result == 0) {
                totalMisses++;
            } else if (result == 1) {
                totalHits++;
            } else if (result == 2) {
                totalHits++;
                strcpy(sunkShips[sunkShipsCount++], sunkShipName);
            }
        }
    }

    if (player->isBot) {
        player->lastArtilleryHits = totalHits;
        player->lastArtilleryCoord = coord;
    }

    printf("Total Hits: %d\nTotal Misses: %d\n", totalHits, totalMisses);

    if (sunkShipsCount > 0) {
        for (int i = 0; i < sunkShipsCount; i++) {
            if (player->isBot) {
                printf("%s sunk your %s!\n", player->name, sunkShips[i]);
            } else {
                printf("You sunk the opponent's %s!\n", sunkShips[i]);
            }
        }
        unlockSpecialMoves(player, opponent);
    }
}

void torpedo(Player* player, Player* opponent, Fleet* opponentFleet, const char* input, bool hardMode) {
    int totalHits = 0;
    int totalMisses = 0;
    char sunkShips[SHIP_TYPES][20] = { "" };
    int sunkShipsCount = 0;

    printf("Torpedo attack results on %s:\n", isalpha(input[0]) ? "column" : "row");

    if (isalpha(input[0])) {
        int col = tolower(input[0]) - 'a';
        if (col < 0 || col >= GRID_SIZE) {
            printf("Invalid column.\n");
            return;
        }
        printf("Torpedoing column %c:\n", 'A' + col);
        for (int i = 0; i < GRID_SIZE; i++) {
            Coordinate coord = { col, i };
            char sunkShipName[20] = "";
            int result = fire(player, opponent, opponentFleet, coord, hardMode, sunkShipName);
            if (result == 0) {
                totalMisses++;
            } else if (result == 1) {
                totalHits++;
            } else if (result == 2) {
                totalHits++;
                strcpy(sunkShips[sunkShipsCount++], sunkShipName);
            }
        }
    } else {
        int row = atoi(input) - 1;
        if (row < 0 || row >= GRID_SIZE) {
            printf("Invalid row.\n");
            return;
        }
        printf("Torpedoing row %d:\n", row + 1);
        for (int i = 0; i < GRID_SIZE; i++) {
            Coordinate coord = { i, row };
            char sunkShipName[20] = "";
            int result = fire(player, opponent, opponentFleet, coord, hardMode, sunkShipName);
            if (result == 0) {
                totalMisses++;
            } else if (result == 1) {
                totalHits++;
            } else if (result == 2) {
                totalHits++;
                strcpy(sunkShips[sunkShipsCount++], sunkShipName);
            }
        }
    }

    printf("Total Hits: %d\nTotal Misses: %d\n", totalHits, totalMisses);

    if (sunkShipsCount > 0) {
        for (int i = 0; i < sunkShipsCount; i++) {
            if (player->isBot) {
                printf("%s sunk your %s!\n", player->name, sunkShips[i]);
            } else {
                printf("You sunk the opponent's %s!\n", sunkShips[i]);
            }
        }
        unlockSpecialMoves(player, opponent);
    }
}

bool checkWin(Fleet* fleet) {
    for (int i = 0; i < SHIP_TYPES; i++) {
        if (!fleet->ships[i].sunk) {
            return false;
        }
    }
    return true;
}

void updateShipStatus(Ship* ship) {
    if (ship->hits >= ship->size) {
        ship->sunk = true;
    }
}

void unlockSpecialMoves(Player* player, Player* opponent) {
    if (opponent->shipsRemaining == 0) {
        return;
    }

    if (!player->artilleryAvailable) {
        player->artilleryAvailable = true;
        if (player->isBot) {
            printf("%s has unlocked Artillery for the next turn!\n", player->name);
        } else {
            printf("Artillery will be available for your next turn!\n");
        }
    }

    if (opponent->shipsRemaining == 1 && !player->torpedoAvailable) {
        player->torpedoAvailable = true;
        if (player->isBot) {
            printf("%s has unlocked Torpedo for the next turn!\n", player->name);
        } else {
            printf("Torpedo will be available for your next turn!\n");
        }
    }

    if (player->shipsSunk > player->smokeScreensUsed && player->smokeScreensUsed < SHIP_TYPES) {
        printf("%s has unlocked a Smoke Screen for the next turn!\n", player->name);
    }
}

void displayTrackingGrid(Player* player, bool hardMode) {
    printf("Opponent's Grid:\n");
    displayGrid(player->trackingGrid, !hardMode);
}

bool isValidCommand(const char* command, Player* player) {
    if (strcmp(command, "fire") == 0 || strcmp(command, "radar") == 0) {
        return true;
    }
    if (strcmp(command, "smoke") == 0 && player->smokeScreensUsed < player->shipsSunk) {
        return true;
    }
    if (strcmp(command, "artillery") == 0 && player->artilleryAvailable) {
        return true;
    }
    if (strcmp(command, "torpedo") == 0 && player->torpedoAvailable) {
        return true;
    }
    return false;
}

void getInput(char* input, int size) {
    if (fgets(input, size, stdin) != NULL) {
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] != '\n') {
            flushInputBuffer();
        }
        input[strcspn(input, "\n")] = '\0';
    }
}

void coordinateToString(Coordinate coord, char* coordStr) {
    coordStr[0] = 'A' + coord.x;
    sprintf(&coordStr[1], "%d", coord.y + 1);
    coordStr[strlen(coordStr)] = '\0';
}

void toLowerCase(char* str) {
    for (; *str; ++str) *str = tolower(*str);
}

void flushInputBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int getRandomNumber(int min, int max) {
    return rand() % (max - min + 1) + min;
}

Coordinate getRandomCoordinate() {
    Coordinate coord;
    coord.x = getRandomNumber(0, GRID_SIZE - 1);
    coord.y = getRandomNumber(0, GRID_SIZE - 1);
    return coord;
}

Coordinate getNextTarget(Player* bot, Fleet* opponentFleet) {
    int probabilityGrid[GRID_SIZE][GRID_SIZE];
    calculateProbabilityGrid(bot, opponentFleet, probabilityGrid);

    int maxProbability = -1;
    Coordinate bestCoords[GRID_SIZE * GRID_SIZE];
    int bestCoordsCount = 0;

    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            if (bot->trackingGrid[y][x] == '~') {
                int prob = probabilityGrid[y][x];
                if (prob > maxProbability) {
                    maxProbability = prob;
                    bestCoordsCount = 0;
                    bestCoords[bestCoordsCount++] = (Coordinate){ x, y };
                } else if (prob == maxProbability) {
                    bestCoords[bestCoordsCount++] = (Coordinate){ x, y };
                }
            }
        }
    }

    if (bestCoordsCount > 0) {
        // Randomly select among the best coordinates
        int idx = rand() % bestCoordsCount;
        return bestCoords[idx];
    }

    // Fallback to random if no valid targets
    return getRandomCoordinate();
}

void calculateProbabilityGrid(Player* bot, Fleet* opponentFleet, int probabilityGrid[GRID_SIZE][GRID_SIZE]) {
    // Initialize probability grid to zero
    memset(probabilityGrid, 0, sizeof(int) * GRID_SIZE * GRID_SIZE);

    bool hasHits = false;
    // First, check if there are any hits on the tracking grid
    for (int y = 0; y < GRID_SIZE && !hasHits; y++) {
        for (int x = 0; x < GRID_SIZE && !hasHits; x++) {
            if (bot->trackingGrid[y][x] == '*') {
                hasHits = true;
            }
        }
    }

    // Iterate over each remaining ship
    for (int shipIdx = 0; shipIdx < SHIP_TYPES; shipIdx++) {
        Ship currentShip = opponentFleet->ships[shipIdx];
        if (currentShip.sunk) {
            continue; // Skip sunk ships
        }

        int shipSize = currentShip.size;

        // Horizontal placements
        for (int y = 0; y < GRID_SIZE; y++) {
            for (int x = 0; x <= GRID_SIZE - shipSize; x++) {
                bool valid = true;
                bool overlapsHit = false;
                for (int k = 0; k < shipSize; k++) {
                    char cell = bot->trackingGrid[y][x + k];
                    if (cell == 'o') { // Miss
                        valid = false;
                        break;
                    } else if (cell == '*') {
                        overlapsHit = true;
                    }
                }
                if (valid) {
                    // If not in targeting mode (no hits), only consider placements on checkerboard
                    if (!hasHits) {
                        bool onCheckerboard = true;
                        for (int k = 0; k < shipSize; k++) {
                            int tx = x + k;
                            if ((tx + y) % 2 != 0) {
                                onCheckerboard = false;
                                break;
                            }
                        }
                        if (!onCheckerboard) {
                            continue; // Skip this placement
                        }
                    }
                    // If overlaps with a hit, give higher probability
                    int increment = overlapsHit ? 10 : 1;
                    for (int k = 0; k < shipSize; k++) {
                        probabilityGrid[y][x + k] += increment;
                    }
                }
            }
        }

        // Vertical placements
        for (int x = 0; x < GRID_SIZE; x++) {
            for (int y = 0; y <= GRID_SIZE - shipSize; y++) {
                bool valid = true;
                bool overlapsHit = false;
                for (int k = 0; k < shipSize; k++) {
                    char cell = bot->trackingGrid[y + k][x];
                    if (cell == 'o') { // Miss
                        valid = false;
                        break;
                    } else if (cell == '*') {
                        overlapsHit = true;
                    }
                }
                if (valid) {
                    // If not in targeting mode (no hits), only consider placements on checkerboard
                    if (!hasHits) {
                        bool onCheckerboard = true;
                        for (int k = 0; k < shipSize; k++) {
                            int ty = y + k;
                            if ((x + ty) % 2 != 0) {
                                onCheckerboard = false;
                                break;
                            }
                        }
                        if (!onCheckerboard) {
                            continue; // Skip this placement
                        }
                    }
                    // If overlaps with a hit, give higher probability
                    int increment = overlapsHit ? 10 : 1;
                    for (int k = 0; k < shipSize; k++) {
                        probabilityGrid[y + k][x] += increment;
                    }
                }
            }
        }
    }
}

void addAdjacentTargets(Player* bot, Coordinate coord) {
    int dx[] = { 0, 1, 0, -1 }; // N, E, S, W
    int dy[] = { -1, 0, 1, 0 };

    // Check for previous hits to determine direction
    for (int dir = 0; dir < 4; dir++) {
        int nx = coord.x + dx[dir];
        int ny = coord.y + dy[dir];

        if (nx >= 0 && nx < GRID_SIZE && ny >= 0 && ny < GRID_SIZE) {
            if (bot->trackingGrid[ny][nx] == '*') {
                // Direction found; extend in this direction
                int ex = coord.x;
                int ey = coord.y;
                while (true) {
                    ex += dx[dir];
                    ey += dy[dir];
                    if (ex < 0 || ex >= GRID_SIZE || ey < 0 || ey >= GRID_SIZE) break;
                    if (bot->trackingGrid[ey][ex] != '~') break;
                    Coordinate newCoord = { ex, ey };
                    addPotentialTarget(bot, newCoord);
                }
                return;
            }
        }
    }

    // If no direction is found, add all adjacent cells
    for (int dir = 0; dir < 4; dir++) {
        int nx = coord.x + dx[dir];
        int ny = coord.y + dy[dir];

        if (nx >= 0 && nx < GRID_SIZE && ny >= 0 && ny < GRID_SIZE) {
            if (bot->trackingGrid[ny][nx] == '~') {
                Coordinate newCoord = { nx, ny };
                addPotentialTarget(bot, newCoord);
            }
        }
    }
}

void addPotentialTarget(Player* player, Coordinate coord) {
    for (int i = 0; i < player->potentialTargetCount; i++) {
        if (player->potentialTargets[i].x == coord.x && player->potentialTargets[i].y == coord.y) {
            return;
        }
    }
    if (player->potentialTargetCount < GRID_SIZE * GRID_SIZE) {
        player->potentialTargets[player->potentialTargetCount++] = coord;
    }
}

Coordinate getBestArtilleryTarget(Player* bot) {
    Coordinate bestCoord = { -1, -1 };
    int maxUntargeted = 0;

    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            Coordinate coord = { x, y };
            int untargetedCount = countUntargetedTilesInArtilleryArea(bot, coord);
            if (untargetedCount > maxUntargeted) {
                maxUntargeted = untargetedCount;
                bestCoord = coord;
                if (maxUntargeted == 4) {
                    return bestCoord;
                }
            }
        }
    }

    if (maxUntargeted > 0) {
        return bestCoord;
    } else {
        return getRandomCoordinate();
    }
}

int countUntargetedTilesInArtilleryArea(Player* bot, Coordinate coord) {
    int count = 0;
    int xStart = coord.x;
    int yStart = coord.y;
    int xEnd = coord.x + 1;
    int yEnd = coord.y + 1;

    handleEdgeCoordinates(&xStart, &xEnd);
    handleEdgeCoordinates(&yStart, &yEnd);

    for (int i = yStart; i <= yEnd; i++) {
        for (int j = xStart; j <= xEnd; j++) {
            if (bot->trackingGrid[i][j] == '~') {
                count++;
            }
        }
    }
    return count;
}

bool chooseTorpedoTarget(Player* bot, Player* opponent, Fleet* opponentFleet, bool hardMode) {
    int maxUntargeted = 0;
    char targetType = 'r';
    int targetIndex = -1;

    for (int row = 0; row < GRID_SIZE; row++) {
        int untargetedFound = 0;
        for (int col = 0; col < GRID_SIZE; col++) {
            if (bot->trackingGrid[row][col] == '~') {
                untargetedFound++;
            }
        }
        if (untargetedFound > maxUntargeted) {
            maxUntargeted = untargetedFound;
            targetType = 'r';
            targetIndex = row;
        }
    }

    for (int col = 0; col < GRID_SIZE; col++) {
        int untargetedFound = 0;
        for (int row = 0; row < GRID_SIZE; row++) {
            if (bot->trackingGrid[row][col] == '~') {
                untargetedFound++;
            }
        }
        if (untargetedFound > maxUntargeted) {
            maxUntargeted = untargetedFound;
            targetType = 'c';
            targetIndex = col;
        }
    }

    if (targetIndex == -1) {
        return false;
    }

    if (targetType == 'r') {
        printf("%s uses Torpedo at row %d\n", bot->name, targetIndex + 1);
        char rowStr[3];
        sprintf(rowStr, "%d", targetIndex + 1);
        torpedo(bot, opponent, opponentFleet, rowStr, hardMode);
    } else if (targetType == 'c') {
        printf("%s uses Torpedo at column %c\n", bot->name, 'A' + targetIndex);
        char colStr[2];
        colStr[0] = 'a' + targetIndex;
        colStr[1] = '\0';
        torpedo(bot, opponent, opponentFleet, colStr, hardMode);
    }

    return true;
}

Coordinate getSmokeScreenCoordinateForBot(Player* bot) {
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            bool hasShip = false;
            int xStart = x;
            int yStart = y;
            int xEnd = x + 1;
            int yEnd = y + 1;

            handleEdgeCoordinates(&xStart, &xEnd);
            handleEdgeCoordinates(&yStart, &yEnd);

            for (int i = yStart; i <= yEnd; i++) {
                for (int j = xStart; j <= xEnd; j++) {
                    char cell = bot->grid[i][j];
                    if (cell >= 'A' && cell <= 'Z') {
                        hasShip = true;
                        break;
                    }
                }
                if (hasShip) break;
            }
            if (hasShip) {
                Coordinate coord = { xStart, yStart };
                return coord;
            }
        }
    }
    Coordinate invalidCoord = { -1, -1 };
    return invalidCoord;
}

void handleEdgeCoordinates(int* start, int* end) {
    if (*start < 0) *start = 0;
    if (*end >= GRID_SIZE) *end = GRID_SIZE - 1;
}


