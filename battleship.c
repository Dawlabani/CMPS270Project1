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

typedef struct {
    int x;
    int y;
} Coordinate;

// Modified Player struct with bot-related fields
typedef struct {
    char name[MAX_NAME_LENGTH];
    char grid[GRID_SIZE][GRID_SIZE];           // Player's own grid
    char trackingGrid[GRID_SIZE][GRID_SIZE];   // Player's view of opponent's grid
    int radarSweepsUsed;
    int smokeScreensUsed;
    int shipsSunk;
    int shipsRemaining;
    bool artilleryAvailableNextTurn;
    bool torpedoAvailableNextTurn;
    bool artilleryAvailable;
    bool torpedoAvailable;
    bool isBot; // New field to identify bot players
    struct {
        int x;
        int y;
        bool active; // Add an 'active' flag
    } smokeScreens[SHIP_TYPES];
} Player;

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

// Function prototypes
void initializePlayer(Player* player, bool isBot); // Modified to include isBot
void initializeGrid(char grid[GRID_SIZE][GRID_SIZE]);
void displayGrid(char grid[GRID_SIZE][GRID_SIZE], bool showShips);
void placeShips(Player* player, Fleet* fleet);
bool isValidPlacement(char grid[GRID_SIZE][GRID_SIZE], Coordinate coord, int size, char orientation);
void placeShipOnGrid(char grid[GRID_SIZE][GRID_SIZE], Coordinate coord, int size, char orientation, char symbol);
Coordinate parseCoordinate(const char* input);
void clearScreen();
void gameLoop(Player* player1, Player* player2, Fleet* fleet1, Fleet* fleet2, bool hardMode);
void swapPlayers(Player** currentPlayer, Player** opponent);
void swapFleets(Fleet** currentFleet, Fleet** opponentFleet);
void performMove(Player* player, Player* opponent, Fleet* opponentFleet, bool hardMode);
int fire(Player* player, Player* opponent, Fleet* opponentFleet, Coordinate coord, bool hardMode, char* sunkShipName);
void radarSweep(Player* player, Player* opponent, Coordinate coord);
void smokeScreen(Player* player, Coordinate coord);
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

// Function Implementations

int main() {
    srand((unsigned int)time(NULL));
    Player player1, player2;
    Fleet fleet1, fleet2;
    bool hardMode = false;
    char difficulty[MAX_INPUT_LENGTH];

    // Initialize players and fleets
    initializePlayer(&player1, false); // Human player
    initializePlayer(&player2, true);  // Bot player

    // Introduction to the bot
    printf("Welcome to Battleship! You will be playing against the Bot.\n");

    // Ask for tracking difficulty
    printf("Choose tracking difficulty level (easy/hard): ");
    getInput(difficulty, sizeof(difficulty));
    toLowerCase(difficulty);

    if (strcmp(difficulty, "hard") == 0) {
        hardMode = true;
    }

    // Get player names with input validation
    do {
        printf("Enter your name: ");
        getInput(player1.name, sizeof(player1.name));
        if (strlen(player1.name) == 0) {
            printf("Name cannot be empty. Please enter a valid name.\n");
        }
    } while (strlen(player1.name) == 0);

    // Assign name to bot player
    strcpy(player2.name, "Bot");

    // Randomly choose first player
    Player* currentPlayer = (rand() % 2 == 0) ? &player1 : &player2;
    Player* opponent = (currentPlayer == &player1) ? &player2 : &player1;
    printf("%s will play first.\n", currentPlayer->name);

    // Initialize fleets
    strcpy(fleet1.ships[0].name, "Carrier");
    fleet1.ships[0].size = 5;
    fleet1.ships[0].hits = 0;
    fleet1.ships[0].sunk = false;
    fleet1.ships[0].symbol = 'C';

    strcpy(fleet1.ships[1].name, "Battleship");
    fleet1.ships[1].size = 4;
    fleet1.ships[1].hits = 0;
    fleet1.ships[1].sunk = false;
    fleet1.ships[1].symbol = 'B';

    strcpy(fleet1.ships[2].name, "Destroyer");
    fleet1.ships[2].size = 3;
    fleet1.ships[2].hits = 0;
    fleet1.ships[2].sunk = false;
    fleet1.ships[2].symbol = 'D';

    strcpy(fleet1.ships[3].name, "Submarine");
    fleet1.ships[3].size = 2;
    fleet1.ships[3].hits = 0;
    fleet1.ships[3].sunk = false;
    fleet1.ships[3].symbol = 'S';

    fleet2 = fleet1; // Both players have the same fleet setup

    // Players place their ships
    placeShips(&player1, &fleet1);
    clearScreen();
    placeShips(&player2, &fleet2); // Bot places ships automatically
    clearScreen();

    // Start the game loop
    gameLoop(&player1, &player2, &fleet1, &fleet2, hardMode);

    return 0;
}

void initializePlayer(Player* player, bool isBot) {
    initializeGrid(player->grid);
    initializeGrid(player->trackingGrid);
    player->radarSweepsUsed = 0;
    player->smokeScreensUsed = 0;
    player->shipsSunk = 0;
    player->shipsRemaining = SHIP_TYPES;
    player->artilleryAvailable = false;
    player->torpedoAvailable = false;
    player->isBot = isBot; // Set bot status
    for (int i = 0; i < SHIP_TYPES; i++) {
        player->smokeScreens[i].x = -1;
        player->smokeScreens[i].y = -1;
        player->smokeScreens[i].active = false; // Initialize as inactive
    }
}

void initializeGrid(char grid[GRID_SIZE][GRID_SIZE]) {
    for (int i = 0; i < GRID_SIZE; i++)
        for (int j = 0; j < GRID_SIZE; j++)
            grid[i][j] = '~';
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
        // Bot automatically places ships randomly
        char orientations[] = { 'h', 'v' };
        for (int i = 0; i < SHIP_TYPES; i++) {
            bool placed = false;
            while (!placed) {
                Coordinate coord;
                coord.x = rand() % GRID_SIZE;
                coord.y = rand() % GRID_SIZE;
                char orientation = orientations[rand() % 2];
                if (isValidPlacement(player->grid, coord, fleet->ships[i].size, orientation)) {
                    placeShipOnGrid(player->grid, coord, fleet->ships[i].size, orientation, fleet->ships[i].symbol);
                    placed = true;
                }
            }
        }
        printf("Bot has placed its ships.\n");
        return;
    }

    char input[MAX_INPUT_LENGTH], orientation[MAX_INPUT_LENGTH];
    Coordinate coord;

    printf("%s, place your ships on the grid.\n", player->name);
    for (int i = 0; i < SHIP_TYPES; i++) {
        bool placed = false;
        while (!placed) {
            displayGrid(player->grid, true);
            printf("Enter coordinates and orientation (h/v) for %s (size %d): ", fleet->ships[i].name, fleet->ships[i].size);

            getInput(input, sizeof(input));
            char* token = strtok(input, " ");
            if (token == NULL) {
                printf("Invalid input format. Press Enter to continue...");
                fflush(stdout);
                getchar();
                continue;
            }
            strcpy(input, token);

            token = strtok(NULL, " ");
            if (token == NULL) {
                printf("Invalid input format. Press Enter to continue...");
                fflush(stdout);
                getchar();
                continue;
            }
            strcpy(orientation, token);

            toLowerCase(input);
            toLowerCase(orientation);

            coord = parseCoordinate(input);
            if (coord.x == -1 || coord.y == -1) {
                printf("Invalid coordinates. Press Enter to continue...");
                fflush(stdout);
                getchar();
                continue;
            }

            char dir = tolower(orientation[0]);
            if (dir != 'h' && dir != 'v') {
                printf("Invalid orientation. Use 'h' for horizontal or 'v' for vertical. Press Enter to continue...");
                fflush(stdout);
                getchar();
                continue;
            }

            if (isValidPlacement(player->grid, coord, fleet->ships[i].size, dir)) {
                placeShipOnGrid(player->grid, coord, fleet->ships[i].size, dir, fleet->ships[i].symbol);
                placed = true;
                clearScreen();
            } else {
                printf("Invalid placement. Ships cannot overlap or go out of bounds. Press Enter to continue...");
                fflush(stdout);
                getchar();
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

    if (orientation == 'h') {
        for (int i = 0; i < size; i++)
            grid[y][x + i] = symbol;
    } else if (orientation == 'v') {
        for (int i = 0; i < size; i++)
            grid[y + i][x] = symbol;
    }
}

Coordinate parseCoordinate(const char* input) {
    Coordinate coord = { -1, -1 };
    if (strlen(input) < 2 || strlen(input) > 3) return coord;

    char col = input[0];
    int row = atoi(&input[1]);

    if (col >= 'A' && col <= 'J') {
        coord.x = col - 'A';
    } else if (col >= 'a' && col <= 'j') {
        coord.x = col - 'a';
    } else {
        return coord;
    }

    if (row >= 1 && row <= GRID_SIZE) {
        coord.y = row - 1;
    } else {
        coord.y = -1;
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

void gameLoop(Player* player1, Player* player2, Fleet* fleet1, Fleet* fleet2, bool hardMode) {
    Player* currentPlayer = player1;
    Player* opponent = player2;
    Fleet* currentFleet = fleet1;
    Fleet* opponentFleet = fleet2;

    while (true) {
        performMove(currentPlayer, opponent, opponentFleet, hardMode);
        if (checkWin(opponentFleet)) {
            printf("%s wins!\n", currentPlayer->name);
            break;
        }
        swapPlayers(&currentPlayer, &opponent);
        swapFleets(&currentFleet, &opponentFleet); // Swap fleets correctly
    }
}

void swapPlayers(Player** currentPlayer, Player** opponent) {
    Player* temp = *currentPlayer;
    *currentPlayer = *opponent;
    *opponent = temp;
}

void swapFleets(Fleet** currentFleet, Fleet** opponentFleet) {
    Fleet* temp = *currentFleet;
    *currentFleet = *opponentFleet;
    *opponentFleet = temp;
}

void performMove(Player* player, Player* opponent, Fleet* opponentFleet, bool hardMode) {
    clearScreen();
    char input[MAX_INPUT_LENGTH * 2];
    char command[MAX_INPUT_LENGTH];
    char argument[MAX_INPUT_LENGTH];
    bool validMove = false;

    // Unlock special moves if available
    if (player->artilleryAvailableNextTurn) {
        player->artilleryAvailable = true;
        player->artilleryAvailableNextTurn = false;
    }
    if (player->torpedoAvailableNextTurn) {
        player->torpedoAvailable = true;
        player->torpedoAvailableNextTurn = false;
    }

    while (!validMove) {
        printf("%s's turn.\n", player->name);
        displayTrackingGrid(player, hardMode);
        printf("Available moves:\n");
        printf("Fire [coordinate]\n");
        printf("Radar [coordinate] (Used %d/%d)\n", player->radarSweepsUsed, MAX_RADAR_SWEEPS);
        if (player->smokeScreensUsed < player->shipsSunk) {
            printf("Smoke [coordinate] (Used %d)\n", player->smokeScreensUsed);
        }
        if (player->artilleryAvailable) {
            printf("Artillery [coordinate]\n");
        }
        if (player->torpedoAvailable) {
            printf("Torpedo [row/column]\n");
        }

        if (player->isBot) {
            // Bot's turn: fire randomly
            Coordinate botCoord;
            bool validCoord = false;
            while (!validCoord) {
                botCoord.x = rand() % GRID_SIZE;
                botCoord.y = rand() % GRID_SIZE;
                char cell = opponent->grid[botCoord.y][botCoord.x];
                if (cell != 'o' && cell != 'X') {
                    validCoord = true;
                }
            }
            char sunkShipName[20] = "";
            printf("Bot fires at %c%d.\n", 'A' + botCoord.x, botCoord.y + 1);
            int result = fire(player, opponent, opponentFleet, botCoord, hardMode, sunkShipName);
            if (result == 0) {
                printf("Miss!\n");
            } else if (result == 1) {
                printf("Hit!\n");
            } else if (result == 2) {
                printf("Hit!\n");
                printf("Bot sunk your %s!\n", sunkShipName);
                // Unlock special moves after sinking a ship
                unlockSpecialMoves(player, opponent);
            } else if (result == 3) {
                printf("Already targeted this coordinate.\n");
            }
            validMove = true;
            printf("Press Enter to continue...");
            fflush(stdout);
            getchar();
            return;
        }

        printf("Enter your move: ");
        getInput(input, sizeof(input));
        toLowerCase(input);

        char* token = strtok(input, " ");
        if (token == NULL) {
            printf("Invalid input format.\n");
            printf("You lose your turn. Press Enter to continue...");
            fflush(stdout);
            getchar();
            return; // Player loses turn
        }
        strcpy(command, token);

        token = strtok(NULL, " ");
        if (token == NULL) {
            printf("Invalid input format.\n");
            printf("You lose your turn. Press Enter to continue...");
            fflush(stdout);
            getchar();
            return; // Player loses turn
        }
        strcpy(argument, token);

        if (!isValidCommand(command, player)) {
            printf("Invalid command or command not available.\n");
            printf("You lose your turn. Press Enter to continue...");
            fflush(stdout);
            getchar();
            return; // Player loses turn
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
                    printf("Hit!\n");
                    printf("You sunk the opponent's %s!\n", sunkShipName);
                    // Unlock special moves after sinking a ship
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
                return; // Player loses turn
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
                    return; // Player loses turn
                }
            } else {
                printf("You cannot deploy a radar sweep as you have reached the limit.\n");
                printf("You lose your turn. Press Enter to continue...");
                fflush(stdout);
                getchar();
                return; // Player loses turn
            }
        } else if (strcmp(command, "smoke") == 0) {
            if (player->smokeScreensUsed < player->shipsSunk) {
                Coordinate coord = parseCoordinate(argument);
                if (coord.x != -1 && coord.y != -1) {
                    smokeScreen(player, coord);
                    validMove = true;
                    printf("Press Enter to continue...");
                    fflush(stdout);
                    getchar();
                } else {
                    printf("Invalid coordinates.\n");
                    printf("You lose your turn. Press Enter to continue...");
                    fflush(stdout);
                    getchar();
                    return; // Player loses turn
                }
            } else {
                printf("You cannot deploy a smoke screen as you have reached the limit.\n");
                printf("You lose your turn. Press Enter to continue...");
                fflush(stdout);
                getchar();
                return; // Player loses turn
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
                return; // Player loses turn
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
                return; // Player loses turn
            }
        }
    }
}

int fire(Player* player, Player* opponent, Fleet* opponentFleet, Coordinate coord, bool hardMode, char* sunkShipName) {
    // The bot firing is handled in performMove, not here
    // So 'fire' function only processes the firing at coord

    char cell = opponent->grid[coord.y][coord.x];

    if (cell == '~') {
        opponent->grid[coord.y][coord.x] = 'o';
        if (!hardMode) {
            player->trackingGrid[coord.y][coord.x] = 'o';
        }
        return 0; // Miss
    } else if (cell >= 'A' && cell <= 'Z') {
        if (opponent->grid[coord.y][coord.x] == 'X') {
            return 3; // Already targeted
        }
        opponent->grid[coord.y][coord.x] = 'X';
        player->trackingGrid[coord.y][coord.x] = '*';

        // Find the ship based on the symbol
        Ship* ship = NULL;
        for (int i = 0; i < SHIP_TYPES; i++) {
            if (opponentFleet->ships[i].symbol == cell) {
                ship = &opponentFleet->ships[i];
                break;
            }
        }

        if (ship != NULL) {
            ship->hits++;
            updateShipStatus(ship);

            if (ship->sunk) {
                strcpy(sunkShipName, ship->name);
                player->shipsSunk++;
                opponent->shipsRemaining--;
                return 2; // Hit and sunk
            } else {
                return 1; // Hit but not sunk
            }
        }
    } else if (cell == 'o' || cell == 'X') {
        return 3; // Already targeted
    }
    return -1; // Should not reach here
}

void radarSweep(Player* player, Player* opponent, Coordinate coord) {
    if (coord.x < 0 || coord.x > GRID_SIZE - 2 || coord.y < 0 || coord.y > GRID_SIZE - 2) {
        printf("Invalid coordinates for radar sweep.\n");
        return;
    }
    bool found = false;

    // Check if the radar sweep area overlaps any active smoke screen areas
    for (int s = 0; s < player->smokeScreensUsed; s++) {
        if (!player->smokeScreens[s].active) continue; // Skip inactive smoke screens

        int sx = player->smokeScreens[s].x;
        int sy = player->smokeScreens[s].y;
        if (coord.x + 1 >= sx && coord.x <= sx + 1 &&
            coord.y + 1 >= sy && coord.y <= sy + 1) {
            printf("Radar sweep area is obscured by a smoke screen. No information gained.\n");
            player->smokeScreens[s].active = false; // Deactivate the smoke screen
            return;
        }
    }

    // Proceed to check for ships
    for (int i = coord.y; i <= coord.y + 1; i++) {
        for (int j = coord.x; j <= coord.x + 1; j++) {
            if (opponent->grid[i][j] >= 'A' && opponent->grid[i][j] <= 'Z') {
                found = true;
                break;
            }
        }
        if (found) break;
    }
    if (found) {
        printf("Enemy ships detected within the radar sweep area.\n");
    } else {
        printf("No enemy ships detected within the radar sweep area.\n");
    }
}

void smokeScreen(Player* player, Coordinate coord) {
    if (coord.x < 0 || coord.x > GRID_SIZE - 2 || coord.y < 0 || coord.y > GRID_SIZE - 2) {
        printf("Invalid coordinates for smoke screen.\n");
        return;
    }
    // Store the smoke screen area and set it as active
    player->smokeScreens[player->smokeScreensUsed].x = coord.x;
    player->smokeScreens[player->smokeScreensUsed].y = coord.y;
    player->smokeScreens[player->smokeScreensUsed].active = true; // Set as active
    player->smokeScreensUsed++;
    clearScreen();
    printf("Smoke screen deployed.\n");
}

void artillery(Player* player, Player* opponent, Fleet* opponentFleet, Coordinate coord, bool hardMode) {
    int totalHits = 0;
    int totalMisses = 0;
    int alreadyTargeted = 0; // New variable to count already targeted tiles
    char sunkShips[SHIP_TYPES][20] = { "" };
    int sunkShipsCount = 0;

    printf("Artillery strike results:\n");
    // Target a 2x2 area starting from the provided coordinate
    for (int i = coord.y; i <= coord.y + 1; i++) {
        for (int j = coord.x; j <= coord.x + 1; j++) {
            if (i >= 0 && i < GRID_SIZE && j >= 0 && j < GRID_SIZE) {
                Coordinate tempCoord = { j, i };
                char sunkShipName[20] = "";
                int result = fire(player, opponent, opponentFleet, tempCoord, hardMode, sunkShipName);
                if (result == 0) {
                    totalMisses++;
                } else if (result == 1) {
                    totalHits++;
                } else if (result == 2) {
                    totalHits++;
                    strcpy(sunkShips[sunkShipsCount++], sunkShipName); // Correctly copy the sunk ship's name
                } else if (result == 3) {
                    alreadyTargeted++;
                }
            }
        }
    }

    printf("Total Hits: %d\n", totalHits);
    printf("Total Misses: %d\n", totalMisses);
    if (alreadyTargeted > 0) {
        printf("Already Targeted Tiles: %d\n", alreadyTargeted);
    }

    if (sunkShipsCount > 0) {
        for (int i = 0; i < sunkShipsCount; i++) {
            printf("You sunk the opponent's %s!\n", sunkShips[i]);
        }
        unlockSpecialMoves(player, opponent); // Unlock special moves if any ships were sunk
    }
}

void torpedo(Player* player, Player* opponent, Fleet* opponentFleet, const char* input, bool hardMode) {
    int totalHits = 0;
    int totalMisses = 0;
    int alreadyTargeted = 0; // New variable to count already targeted tiles
    char sunkShips[SHIP_TYPES][20] = { "" };
    int sunkShipsCount = 0;

    printf("Torpedo attack results:\n");

    if (isalpha(input[0])) {
        int col = tolower(input[0]) - 'a';
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
            } else if (result == 3) {
                alreadyTargeted++;
            }
        }
    } else {
        int row = atoi(input) - 1;
        if (row >= 0 && row < GRID_SIZE) {
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
                } else if (result == 3) {
                    alreadyTargeted++;
                }
            }
        } else {
            printf("Invalid row or column.\n");
            printf("You lose your turn. Press Enter to continue...");
            fflush(stdout);
            getchar();
            return;
        }
    }

    printf("Total Hits: %d\n", totalHits);
    printf("Total Misses: %d\n", totalMisses);
    if (alreadyTargeted > 0) {
        printf("Already Targeted Tiles: %d\n", alreadyTargeted);
    }

    if (sunkShipsCount > 0) {
        for (int i = 0; i < sunkShipsCount; i++) {
            printf("You sunk the opponent's %s!\n", sunkShips[i]);
        }
        unlockSpecialMoves(player, opponent); // Unlock special moves if any ships were sunk
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
    if (!player->artilleryAvailable && !player->artilleryAvailableNextTurn) {
        player->artilleryAvailableNextTurn = true;
        printf("Artillery will be available for your next turn!\n");
    }
    if (opponent->shipsRemaining == 1 && !player->torpedoAvailable && !player->torpedoAvailableNextTurn) {
        player->torpedoAvailableNextTurn = true;
        printf("Torpedo will be available for your next turn!\n");
    }
    // Notify about smoke screen availability
    if (player->smokeScreensUsed < player->shipsSunk) {
        printf("Smoke screen will be available for your next turn!\n");
    }
}

void displayTrackingGrid(Player* player, bool hardMode) {
    printf("Opponent's Grid:\n");
    displayGrid(player->trackingGrid, !hardMode);
}

bool isValidCommand(const char* command, Player* player) {
    if (strcmp(command, "fire") == 0 ||
        strcmp(command, "radar") == 0) {
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
        input[strcspn(input, "\n")] = '\0'; // Remove newline character
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
