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

// Function prototypes
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
Coordinate getNextTarget(Player* bot);
void addAdjacentTargets(Player* bot, Coordinate coord);
Coordinate getBestArtilleryTarget(Player* bot);
int countUntargetedTilesInArtilleryArea(Player* bot, Coordinate coord);
bool chooseTorpedoTarget(Player* bot, Player* opponent, Fleet* opponentFleet, bool hardMode);
void addPotentialTarget(Player* player, Coordinate coord);
bool isUnderSmoke(Player* opponent, Coordinate coord);
Coordinate getSmokeScreenCoordinateForBot(Player* bot);
void addArtilleryHitTargets(Player* bot, Coordinate coord);
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

/**
 * @brief Initializes a player with default values.
 *
 * @param player Pointer to the player to initialize.
 * @param isBot Boolean indicating if the player is a bot.
 * @param difficulty Difficulty level for the bot.
 */
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

/**
 * @brief Initializes a grid with water symbols.
 *
 * @param grid 2D array representing the grid to initialize.
 */
void initializeGrid(char grid[GRID_SIZE][GRID_SIZE]) {
    memset(grid, '~', sizeof(char) * GRID_SIZE * GRID_SIZE);
}

/**
 * @brief Displays the grid to the console.
 *
 * @param grid 2D array representing the grid to display.
 * @param showShips Boolean indicating whether to show ship positions.
 */
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

/**
 * @brief Allows a human player to place their ships on the grid.
 *
 * @param player Pointer to the player.
 * @param fleet Pointer to the player's fleet.
 */
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

/**
 * @brief Automatically places ships for the bot.
 *
 * @param bot Pointer to the bot player.
 * @param fleet Pointer to the bot's fleet.
 */
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

/**
 * @brief Checks if a ship can be placed at the specified location.
 *
 * @param grid 2D array representing the player's grid.
 * @param coord Coordinate where the ship's starting point is.
 * @param size Size of the ship.
 * @param orientation 'h' for horizontal or 'v' for vertical.
 * @return true if placement is valid, false otherwise.
 */
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

/**
 * @brief Places a ship on the grid.
 *
 * @param grid 2D array representing the player's grid.
 * @param coord Coordinate where the ship's starting point is.
 * @param size Size of the ship.
 * @param orientation 'h' for horizontal or 'v' for vertical.
 * @param symbol Symbol representing the ship.
 */
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

/**
 * @brief Parses a coordinate string (e.g., "A5") into a Coordinate struct.
 *
 * @param input String input representing the coordinate.
 * @return Coordinate struct with x and y values.
 */
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

/**
 * @brief Clears the console screen.
 */
void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

/**
 * @brief Main game loop handling turns and checking for win conditions.
 *
 * @param currentPlayer Pointer to the current player.
 * @param opponent Pointer to the opponent player.
 * @param currentFleet Pointer to the current player's fleet.
 * @param opponentFleet Pointer to the opponent's fleet.
 * @param hardMode Flag indicating if hard mode is enabled.
 */
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

        // Swap players and fleets
        Player* tempPlayer = currentPlayer;
        currentPlayer = opponent;
        opponent = tempPlayer;

        Fleet* tempFleet = currentFleet;
        currentFleet = opponentFleet;
        opponentFleet = tempFleet;
    }
}

/**
 * @brief Handles a human player's move, processing commands and executing actions.
 *
 * @param player Pointer to the player making the move.
 * @param opponent Pointer to the opponent player.
 * @param opponentFleet Pointer to the opponent's fleet.
 * @param hardMode Flag indicating if hard mode is enabled.
 */
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
            printf("You lose your turn. Press Enter to continue...");
            fflush(stdout);
            getchar();
            return;
        }

        if (!isValidCommand(command, player)) {
            printf("Invalid command or command not available.\n");
            printf("You lose your turn. Press Enter to continue...");
            fflush(stdout);
            getchar();
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
                fflush(stdout);
                getchar();
            } else {
                printf("Invalid coordinates.\n");
                printf("You lose your turn. Press Enter to continue...");
                fflush(stdout);
                getchar();
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
                    fflush(stdout);
                    getchar();
                } else {
                    printf("Invalid coordinates.\n");
                    printf("You lose your turn. Press Enter to continue...");
                    fflush(stdout);
                    getchar();
                    return;
                }
            } else {
                printf("Radar sweeps limit reached.\n");
                printf("You lose your turn. Press Enter to continue...");
                fflush(stdout);
                getchar();
                return;
            }
        } else if (strcmp(command, "smoke") == 0) {
            Coordinate coord = parseCoordinate(argument);
            if (coord.x != -1 && coord.y != -1) {
                if (smokeScreen(player, coord)) {
                    validMove = true;
                    printf("Press Enter to continue...");
                    fflush(stdout);
                    getchar();
                } else {
                    // Failed to deploy smoke screen
                    printf("Failed to deploy smoke screen. You lose your turn. Press Enter to continue...");
                    fflush(stdout);
                    getchar();
                    return;
                }
            } else {
                printf("Invalid coordinates.\n");
                printf("You lose your turn. Press Enter to continue...");
                fflush(stdout);
                getchar();
                return;
            }
        } else if (strcmp(command, "artillery") == 0) {
            Coordinate coord = parseCoordinate(argument);
            if (coord.x != -1 && coord.y != -1) {
                artillery(player, opponent, opponentFleet, coord, hardMode);
                player->artilleryAvailable = false;
                validMove = true;
                printf("Press Enter to continue...");
                fflush(stdout);
                getchar();
            } else {
                printf("Invalid coordinates.\n");
                printf("You lose your turn. Press Enter to continue...");
                fflush(stdout);
                getchar();
                return;
            }
        } else if (strcmp(command, "torpedo") == 0) {
            if (argument[0] != '\0') {
                torpedo(player, opponent, opponentFleet, argument, hardMode);
                player->torpedoAvailable = false;
                validMove = true;
                printf("Press Enter to continue...");
                fflush(stdout);
                getchar();
            } else {
                printf("Invalid input.\n");
                printf("You lose your turn. Press Enter to continue...");
                fflush(stdout);
                getchar();
                return;
            }
        }
    }
}

/**
 * @brief Handles the bot's move based on its difficulty level.
 *
 * @param bot Pointer to the bot player.
 * @param opponent Pointer to the opponent player.
 * @param opponentFleet Pointer to the opponent's fleet.
 * @param hardMode Flag indicating if hard mode is enabled.
 */
void performBotMove(Player* bot, Player* opponent, Fleet* opponentFleet, bool hardMode) {
    printf("%s's turn.\n", bot->name);

    bot->turnNumber++; // Increment bot's turn number

    Coordinate coord;
    char sunkShipName[20] = "";
    int result = -1;

    bool moveMade = false;

    if (bot->difficulty == EASY) {
        // EASY: Purely random moves without targeting
        // 10% chance to use special moves

        // Decide whether to use a special move
        int specialMoveChance = 10; // 10% chance
        int roll = getRandomNumber(1, 100);

        if (roll <= specialMoveChance) {
            // Attempt to use a special move
            // Priority: Smoke > Radar > Artillery > Torpedo
            if (bot->smokeScreensUsed < bot->shipsSunk) {
                Coordinate smokeCoord = getSmokeScreenCoordinateForBot(bot);
                if (smokeCoord.x != -1 && smokeCoord.y != -1 && smokeScreen(bot, smokeCoord)) {
                    printf("%s deployed a smoke screen.\n", bot->name);
                    moveMade = true;
                }
            }
            if (!moveMade && bot->radarSweepsUsed < MAX_RADAR_SWEEPS) {
                coord = getRandomCoordinate();
                printf("%s uses Radar at ", bot->name);
                char coordStr[5];
                coordinateToString(coord, coordStr);
                printf("%s\n", coordStr);
                radarSweep(bot, opponent, coord);
                bot->radarSweepsUsed++;
                moveMade = true;
            }
            if (!moveMade && bot->artilleryAvailable) {
                coord = getBestArtilleryTarget(bot);
                printf("%s uses Artillery at ", bot->name);
                char coordStr[5];
                coordinateToString(coord, coordStr);
                printf("%s\n", coordStr);
                artillery(bot, opponent, opponentFleet, coord, hardMode);
                bot->artilleryAvailable = false;
                moveMade = true;
            }
            if (!moveMade && bot->torpedoAvailable) {
                if (chooseTorpedoTarget(bot, opponent, opponentFleet, hardMode)) {
                    bot->torpedoAvailable = false;
                    moveMade = true;
                }
            }
        }

        // If no special move was made, perform a random fire
        if (!moveMade) {
            coord = getRandomCoordinate();
            printf("Bot fires at %c%d.\n", 'A' + coord.x, coord.y + 1);
            result = fire(bot, opponent, opponentFleet, coord, hardMode, sunkShipName);
            if (result == 0) {
                printf("Miss!\n");
            }
            else if (result == 1) {
                printf("Hit!\n");
            }
            else if (result == 2) {
                printf("Hit and sunk the opponent's %s!\n", sunkShipName);
                unlockSpecialMoves(bot, opponent);
            }
            else if (result == 3) {
                printf("Already targeted this coordinate.\n");
            }
            moveMade = true;
        }

        moveMade = true;
    }
    else if (bot->difficulty == MEDIUM) {
        // MEDIUM: Basic targeting after a hit

        // Attempt to use special moves based on probabilities
        // 20% Radar, 15% Artillery, 10% Torpedo
        int radarChance = 20;
        int artilleryChance = 15;
        int torpedoChance = 10;
        int roll = getRandomNumber(1, 100);

        // Use Radar
        if (roll <= radarChance && bot->radarSweepsUsed < MAX_RADAR_SWEEPS) {
            coord = getRandomCoordinate();
            printf("%s uses Radar at ", bot->name);
            char coordStr[5];
            coordinateToString(coord, coordStr);
            printf("%s\n", coordStr);
            radarSweep(bot, opponent, coord);
            bot->radarSweepsUsed++;
            moveMade = true;
        }

        // Use Artillery
        if (!moveMade && roll <= (radarChance + artilleryChance) && bot->artilleryAvailable) {
            coord = getBestArtilleryTarget(bot);
            printf("%s uses Artillery at ", bot->name);
            char coordStr[5];
            coordinateToString(coord, coordStr);
            printf("%s\n", coordStr);
            artillery(bot, opponent, opponentFleet, coord, hardMode);
            bot->artilleryAvailable = false;
            moveMade = true;
        }

        // Use Torpedo
        if (!moveMade && roll <= (radarChance + artilleryChance + torpedoChance) && bot->torpedoAvailable) {
            if (chooseTorpedoTarget(bot, opponent, opponentFleet, hardMode)) {
                bot->torpedoAvailable = false;
                moveMade = true;
            }
        }

        // If no special move was made, proceed to targeting mode or random fire
        if (!moveMade) {
            if (bot->potentialTargetCount > 0) {
                coord = bot->potentialTargets[--bot->potentialTargetCount];
                printf("%s fires at %c%d (Targeting mode).\n", bot->name, 'A' + coord.x, coord.y + 1);
                result = fire(bot, opponent, opponentFleet, coord, hardMode, sunkShipName);
                if (result == 0) {
                    printf("Miss!\n");
                }
                else if (result == 1) {
                    printf("Hit!\n");
                    addAdjacentTargets(bot, coord);
                }
                else if (result == 2) {
                    printf("Hit!\nYou sunk the opponent's %s!\n", sunkShipName);
                    unlockSpecialMoves(bot, opponent);
                }
                else if (result == 3) {
                    printf("Already targeted this coordinate.\n");
                }
            }
            else {
                // No targets in queue, perform a random fire
                coord = getRandomCoordinate();
                printf("Bot fires at %c%d.\n", 'A' + coord.x, coord.y + 1);
                result = fire(bot, opponent, opponentFleet, coord, hardMode, sunkShipName);
                if (result == 0) {
                    printf("Miss!\n");
                }
                else if (result == 1) {
                    printf("Hit!\n");
                    addAdjacentTargets(bot, coord);
                }
                else if (result == 2) {
                    printf("Hit!\nYou sunk the opponent's %s!\n", sunkShipName);
                    unlockSpecialMoves(bot, opponent);
                }
                else if (result == 3) {
                    printf("Already targeted this coordinate.\n");
                }
            }
        }
    }
    else if (bot->difficulty == HARD) {
        // HARD: Advanced strategies with probability density and prioritized targeting

        // Always prioritize special moves when available
        // Use Artillery and Torpedo aggressively
        // Utilize targeting queue effectively

        // Attempt to use Artillery
        if (bot->artilleryAvailable) {
            coord = getBestArtilleryTarget(bot);
            printf("%s uses Artillery at ", bot->name);
            char coordStr[5];
            coordinateToString(coord, coordStr);
            printf("%s\n", coordStr);
            artillery(bot, opponent, opponentFleet, coord, hardMode);
            bot->artilleryAvailable = false;
            moveMade = true;
        }

        // Attempt to use Torpedo
        if (!moveMade && bot->torpedoAvailable) {
            if (chooseTorpedoTarget(bot, opponent, opponentFleet, hardMode)) {
                bot->torpedoAvailable = false;
                moveMade = true;
            }
        }

        // Use Radar if possible
        if (!moveMade && bot->radarSweepsUsed < MAX_RADAR_SWEEPS) {
            coord = getRandomCoordinate();
            printf("%s uses Radar at ", bot->name);
            char coordStr[5];
            coordinateToString(coord, coordStr);
            printf("%s\n", coordStr);
            radarSweep(bot, opponent, coord);
            bot->radarSweepsUsed++;
            moveMade = true;
        }

        // Utilize targeting queue
        if (!moveMade && bot->potentialTargetCount > 0) {
            coord = bot->potentialTargets[--bot->potentialTargetCount];
            printf("%s fires at %c%d (Targeting mode).\n", bot->name, 'A' + coord.x, coord.y + 1);
            result = fire(bot, opponent, opponentFleet, coord, hardMode, sunkShipName);
            if (result == 0) {
                printf("Miss!\n");
            }
            else if (result == 1) {
                printf("Hit!\n");
                addAdjacentTargets(bot, coord);
            }
            else if (result == 2) {
                printf("Hit!\nYou sunk the opponent's %s!\n", sunkShipName);
                unlockSpecialMoves(bot, opponent);
            }
            else if (result == 3) {
                printf("Already targeted this coordinate.\n");
            }
            moveMade = true;
        }

        // If no special move or targeting, perform strategic fire based on probability
        if (!moveMade) {
            coord = getNextTarget(bot);
            if (coord.x != -1 && coord.y != -1) {
                printf("Bot fires at %c%d.\n", 'A' + coord.x, coord.y + 1);
                result = fire(bot, opponent, opponentFleet, coord, hardMode, sunkShipName);
                if (result == 0) {
                    printf("Miss!\n");
                }
                else if (result == 1) {
                    printf("Hit!\n");
                    addAdjacentTargets(bot, coord);
                }
                else if (result == 2) {
                    printf("Hit!\nYou sunk the opponent's %s!\n", sunkShipName);
                    unlockSpecialMoves(bot, opponent);
                }
                else if (result == 3) {
                    printf("Already targeted this coordinate.\n");
                }
                moveMade = true;
            } else {
                printf("Bot has no valid targets to fire.\n");
            }
        }
    }
}

/**
 * @brief Fires at a specified coordinate, updating grids and ship statuses.
 *
 * @param player Pointer to the player making the fire.
 * @param opponent Pointer to the opponent player.
 * @param opponentFleet Pointer to the opponent's fleet.
 * @param coord Coordinate to fire at.
 * @param hardMode Flag indicating if hard mode is enabled.
 * @param sunkShipName Buffer to store the name of a sunk ship, if any.
 * @return int Result of the fire action:
 *             0 - Miss
 *             1 - Hit
 *             2 - Hit and sunk
 *             3 - Already targeted
 */
int fire(Player* player, Player* opponent, Fleet* opponentFleet, Coordinate coord, bool hardMode, char* sunkShipName) {
    char cell = opponent->grid[coord.y][coord.x];

    if (cell == '~') {
        opponent->grid[coord.y][coord.x] = 'o';
        if (!hardMode || player->isBot) {
            player->trackingGrid[coord.y][coord.x] = 'o';
        }
        return 0; // Miss
    }
    else if (cell >= 'A' && cell <= 'Z') {
        if (opponent->grid[coord.y][coord.x] == 'X') {
            return 3; // Already targeted
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
                    return 2; // Hit and sunk
                }
                return 1; // Hit but not sunk
            }
        }
    }
    else if (cell == 'o' || cell == 'X') {
        return 3; // Already targeted
    }
    return -1; // Unexpected
}

/**
 * @brief Performs a radar sweep at the specified coordinate.
 *
 * @param player Pointer to the player performing the radar sweep.
 * @param opponent Pointer to the opponent player.
 * @param coord Coordinate where the radar sweep is deployed.
 */
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

    // Check for overlapping smoke screens
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
                printf("Radar sweep area is obscured by a smoke screen. No information gained.\n");
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

/**
 * @brief Deploys a smoke screen at the specified coordinate.
 *
 * @param player Pointer to the player deploying the smoke screen.
 * @param coord Coordinate where the smoke screen is deployed.
 * @return true if successfully deployed, false otherwise.
 */
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

/**
 * @brief Performs an artillery strike at the specified coordinate.
 *
 * @param player Pointer to the player performing the artillery strike.
 * @param opponent Pointer to the opponent player.
 * @param opponentFleet Pointer to the opponent's fleet.
 * @param coord Coordinate where the artillery strike is deployed.
 * @param hardMode Flag indicating if hard mode is enabled.
 */
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

/**
 * @brief Performs a torpedo attack based on user input.
 *
 * @param player Pointer to the player performing the torpedo attack.
 * @param opponent Pointer to the opponent player.
 * @param opponentFleet Pointer to the opponent's fleet.
 * @param input String input indicating row or column to torpedo.
 * @param hardMode Flag indicating if hard mode is enabled.
 */
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

/**
 * @brief Checks if all ships in the fleet are sunk.
 *
 * @param fleet Pointer to the fleet to check.
 * @return true if all ships are sunk, false otherwise.
 */
bool checkWin(Fleet* fleet) {
    for (int i = 0; i < SHIP_TYPES; i++) {
        if (!fleet->ships[i].sunk) {
            return false;
        }
    }
    return true;
}

/**
 * @brief Updates the sunk status of a ship based on hits.
 *
 * @param ship Pointer to the ship to update.
 */
void updateShipStatus(Ship* ship) {
    if (ship->hits >= ship->size) {
        ship->sunk = true;
    }
}

/**
 * @brief Unlocks special moves after sinking ships or other conditions.
 *
 * @param player Pointer to the player unlocking special moves.
 * @param opponent Pointer to the opponent player.
 */
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

/**
 * @brief Displays the opponent's tracking grid to the player.
 *
 * @param player Pointer to the player.
 * @param hardMode Flag indicating if hard mode is enabled.
 */
void displayTrackingGrid(Player* player, bool hardMode) {
    printf("Opponent's Grid:\n");
    displayGrid(player->trackingGrid, !hardMode);
}

/**
 * @brief Validates if a command is valid and available for the player.
 *
 * @param command String representing the command.
 * @param player Pointer to the player.
 * @return true if valid, false otherwise.
 */
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

/**
 * @brief Safely gets input from the user.
 *
 * @param input Buffer to store the input.
 * @param size Size of the input buffer.
 */
void getInput(char* input, int size) {
    if (fgets(input, size, stdin) != NULL) {
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] != '\n') {
            flushInputBuffer();
        }
        input[strcspn(input, "\n")] = '\0';
    }
}

/**
 * @brief Converts a Coordinate struct to a string representation (e.g., {0,4} -> "A5").
 *
 * @param coord Coordinate to convert.
 * @param coordStr Buffer to store the string representation.
 */
void coordinateToString(Coordinate coord, char* coordStr) {
    coordStr[0] = 'A' + coord.x;
    sprintf(&coordStr[1], "%d", coord.y + 1);
    coordStr[strlen(coordStr)] = '\0';
}

/**
 * @brief Converts a string to lowercase.
 *
 * @param str String to convert.
 */
void toLowerCase(char* str) {
    for (; *str; ++str) *str = tolower(*str);
}

/**
 * @brief Flushes the input buffer to remove any extraneous input.
 */
void flushInputBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

/**
 * @brief Generates a random number between min and max (inclusive).
 *
 * @param min Minimum value.
 * @param max Maximum value.
 * @return int Random number between min and max.
 */
int getRandomNumber(int min, int max) {
    return rand() % (max - min + 1) + min;
}

/**
 * @brief Generates a random coordinate within the grid.
 *
 * @return Coordinate Randomly generated coordinate.
 */
Coordinate getRandomCoordinate() {
    Coordinate coord;
    coord.x = getRandomNumber(0, GRID_SIZE - 1);
    coord.y = getRandomNumber(0, GRID_SIZE - 1);
    return coord;
}

/**
 * @brief Selects the next target for the bot by choosing a random untargeted coordinate.
 *
 * @param bot Pointer to the bot player.
 * @return Coordinate representing the next target.
 */
Coordinate getNextTarget(Player* bot) {
    Coordinate coord;
    Coordinate potentialCoords[GRID_SIZE * GRID_SIZE];
    int count = 0;

    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            if (bot->trackingGrid[y][x] == '~') {
                potentialCoords[count++] = (Coordinate){ x, y };
            }
        }
    }

    if (count > 0) {
        coord = potentialCoords[getRandomNumber(0, count - 1)];
    } else {
        coord.x = coord.y = -1;
    }
    return coord;
}

/**
 * @brief Adds adjacent tiles to the bot's potential target queue after a successful hit.
 *
 * @param bot Pointer to the bot player.
 * @param coord Coordinate where the hit occurred.
 */
void addAdjacentTargets(Player* bot, Coordinate coord) {
    int dx[] = { 0, 1, 0, -1 };
    int dy[] = { -1, 0, 1, 0 };

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

/**
 * @brief Determines the optimal 2x2 area for deploying an artillery strike based on untargeted tiles.
 *
 * @param bot Pointer to the bot player.
 * @return Coordinate representing the top-left corner of the best artillery target area.
 */
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

/**
 * @brief Counts the number of untargeted tiles within a 2x2 artillery strike area.
 *
 * @param bot Pointer to the bot player.
 * @param coord Coordinate representing the top-left corner of the artillery area.
 * @return Number of untargeted tiles in the specified area.
 */
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

/**
 * @brief Selects the best row or column to deploy a torpedo based on untargeted tiles.
 *
 * @param bot Pointer to the bot player.
 * @param opponent Pointer to the opponent player.
 * @param opponentFleet Pointer to the opponent's fleet.
 * @param hardMode Flag indicating if hard mode is enabled.
 * @return true if a torpedo was successfully deployed, false otherwise.
 */
bool chooseTorpedoTarget(Player* bot, Player* opponent, Fleet* opponentFleet, bool hardMode) {
    int maxUntargeted = 0;
    char targetType = 'r';
    int targetIndex = -1;

    // Evaluate rows
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

    // Evaluate columns
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

/**
 * @brief Adds a new potential target to the bot's target queue.
 *
 * @param player Pointer to the player (bot).
 * @param coord Coordinate to add to the potential target queue.
 */
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

/**
 * @brief Checks if a coordinate is under an active smoke screen.
 *
 * @param opponent Pointer to the opponent player.
 * @param coord Coordinate to check.
 * @return true if under smoke, false otherwise.
 */
bool isUnderSmoke(Player* opponent, Coordinate coord) {
    for (int i = 0; i < opponent->smokeScreensUsed; i++) {
        if (opponent->smokeScreens[i].active) {
            Coordinate smokeCoord = opponent->smokeScreens[i].coord;
            int xStart = smokeCoord.x;
            int yStart = smokeCoord.y;
            int xEnd = smokeCoord.x + 1;
            int yEnd = smokeCoord.y + 1;

            handleEdgeCoordinates(&xStart, &xEnd);
            handleEdgeCoordinates(&yStart, &yEnd);

            if (coord.x >= xStart && coord.x <= xEnd &&
                coord.y >= yStart && coord.y <= yEnd) {
                return true;
            }
        }
    }
    return false;
}

/**
 * @brief Determines the best coordinate for the bot to deploy a smoke screen.
 *
 * @param bot Pointer to the bot player.
 * @return Coordinate representing the best smoke screen location, or {-1, -1} if none found.
 */
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

/**
 * @brief Handles edge cases to prevent out-of-bounds access.
 *
 * @param start Pointer to the start coordinate.
 * @param end Pointer to the end coordinate.
 */
void handleEdgeCoordinates(int* start, int* end) {
    if (*start < 0) *start = 0;
    if (*end >= GRID_SIZE) *end = GRID_SIZE - 1;
}

/**
 * @brief Adds targets around successful artillery hits to the bot's potential target queue.
 *
 * @param bot Pointer to the bot player.
 * @param coord Coordinate where the artillery hit occurred.
 */
void addArtilleryHitTargets(Player* bot, Coordinate coord) {
    int xStart = coord.x - 1;
    int yStart = coord.y - 1;
    int xEnd = coord.x + 2;
    int yEnd = coord.y + 2;

    handleEdgeCoordinates(&xStart, &xEnd);
    handleEdgeCoordinates(&yStart, &yEnd);

    for (int i = yStart; i <= yEnd; i++) {
        for (int j = xStart; j <= xEnd; j++) {
            if (bot->trackingGrid[i][j] == '~') {
                Coordinate newCoord = { j, i };
                addPotentialTarget(bot, newCoord);
            }
        }
    }
    bot->lastArtilleryHits = 0;
}
