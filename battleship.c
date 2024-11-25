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

// Coordinate structure to represent positions on the grid
typedef struct {
    int x;
    int y;
} Coordinate;

// Player structure with bot-related fields and artillery tracking
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
    bool isBot; // Flag to identify bot players
    struct {
        int x;
        int y;
        bool active; // Active flag for smoke screens
    } smokeScreens[SHIP_TYPES];
    Coordinate lastArtilleryCoord;    // Tracks last artillery strike coordinates
    int lastArtilleryHits;            // Tracks hits from last artillery strike

    // Added for targeting in fire function
    bool hasTarget;                   // Indicates if bot has a target to pursue
    Coordinate targetQueue[4];        // Queue of targets to pursue
    int targetQueueCount;             // Number of targets in queue
} Player;

// Ship structure representing each ship type
typedef struct {
    char name[20];
    int size;
    int hits;
    bool sunk;
    char symbol;
} Ship;

// Fleet structure containing all ships
typedef struct {
    Ship ships[SHIP_TYPES];
} Fleet;

// Function prototypes
void initializePlayer(Player* player, bool isBot); // Initialize player with bot status
void initializeGrid(char grid[GRID_SIZE][GRID_SIZE]); // Initialize the game grid
void displayGrid(char grid[GRID_SIZE][GRID_SIZE], bool showShips); // Display the grid
void placeShips(Player* player, Fleet* fleet); // Place ships on the grid
bool isValidPlacement(char grid[GRID_SIZE][GRID_SIZE], Coordinate coord, int size, char orientation); // Validate ship placement
void placeShipOnGrid(char grid[GRID_SIZE][GRID_SIZE], Coordinate coord, int size, char orientation, char symbol); // Place ship on grid
Coordinate parseCoordinate(const char* input); // Parse user input into coordinates
void clearScreen(); // Clear the console screen
void gameLoop(Player* player1, Player* player2, Fleet* fleet1, Fleet* fleet2, bool hardMode); // Main game loop
void swapPlayers(Player** currentPlayer, Player** opponent); // Swap current and opponent players
void swapFleets(Fleet** currentFleet, Fleet** opponentFleet); // Swap current and opponent fleets
void performMove(Player* player, Player* opponent, Fleet* opponentFleet, bool hardMode); // Perform a player's move
int fire(Player* player, Player* opponent, Fleet* opponentFleet, Coordinate coord, bool hardMode, char* sunkShipName); // Handle firing at a coordinate
void radarSweep(Player* player, Player* opponent, Coordinate coord); // Perform a radar sweep
bool smokeScreen(Player* player, Coordinate coord); // Deploy a smoke screen (Updated Function)
void artillery(Player* player, Player* opponent, Fleet* opponentFleet, Coordinate coord, bool hardMode); // Perform an artillery strike
void torpedo(Player* player, Player* opponent, Fleet* opponentFleet, const char* input, bool hardMode); // Perform a torpedo attack
bool checkWin(Fleet* fleet); // Check if all ships are sunk
void updateShipStatus(Ship* ship); // Update ship's sunk status
void unlockSpecialMoves(Player* player, Player* opponent); // Unlock special moves after sinking ships
void displayTrackingGrid(Player* player, bool hardMode); // Display the opponent's tracking grid
bool isValidCommand(const char* command, Player* player); // Validate user commands
void getInput(char* input, int size); // Get user input
void coordinateToString(Coordinate coord, char* coordStr); // Convert coordinates to string
void toLowerCase(char* str); // Convert string to lowercase
void flushInputBuffer(); // Flush the input buffer

// Function Implementations

int main() {
    srand((unsigned int)time(NULL)); // Seed the random number generator

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

// Initialize player with bot status and reset all fields
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
    player->lastArtilleryCoord.x = -1; // Initialize artillery coordinates
    player->lastArtilleryCoord.y = -1;
    player->lastArtilleryHits = 0;     // Initialize artillery hits
    for (int i = 0; i < SHIP_TYPES; i++) {
        player->smokeScreens[i].x = -1;
        player->smokeScreens[i].y = -1;
        player->smokeScreens[i].active = false; // Initialize as inactive
    }
    // Initialize targeting fields
    player->hasTarget = false;
    player->targetQueueCount = 0;
    // Initialize targetQueue as empty
}

// Initialize the game grid with '~' to represent water
void initializeGrid(char grid[GRID_SIZE][GRID_SIZE]) {
    for (int i = 0; i < GRID_SIZE; i++)
        for (int j = 0; j < GRID_SIZE; j++)
            grid[i][j] = '~';
}

// Display the grid to the player
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

// Place ships on the grid for both human and bot players
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
            printf("Enter coordinates and orientation (h/v) for %s (size %d): ",
                   fleet->ships[i].name, fleet->ships[i].size);

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
            }
            else {
                printf("Invalid placement. Ships cannot overlap or go out of bounds. Press Enter to continue...");
                fflush(stdout);
                getchar();
            }
        }
    }
}

// Validate if a ship can be placed at the given coordinates with the specified orientation
bool isValidPlacement(char grid[GRID_SIZE][GRID_SIZE], Coordinate coord, int size, char orientation) {
    int x = coord.x;
    int y = coord.y;

    if (orientation == 'h') {
        if (x + size > GRID_SIZE) return false;
        for (int i = 0; i < size; i++)
            if (grid[y][x + i] != '~') return false;
    }
    else if (orientation == 'v') {
        if (y + size > GRID_SIZE) return false;
        for (int i = 0; i < size; i++)
            if (grid[y + i][x] != '~') return false;
    }
    else {
        return false;
    }
    return true;
}

// Place a ship on the grid based on the starting coordinate and orientation
void placeShipOnGrid(char grid[GRID_SIZE][GRID_SIZE], Coordinate coord, int size, char orientation, char symbol) {
    int x = coord.x;
    int y = coord.y;

    if (orientation == 'h') {
        for (int i = 0; i < size; i++)
            grid[y][x + i] = symbol;
    }
    else if (orientation == 'v') {
        for (int i = 0; i < size; i++)
            grid[y + i][x] = symbol;
    }
}

// Parse user input into grid coordinates
Coordinate parseCoordinate(const char* input) {
    Coordinate coord = { -1, -1 };
    if (strlen(input) < 2 || strlen(input) > 3) return coord;

    char col = input[0];
    int row = atoi(&input[1]);

    if (col >= 'A' && col <= 'J') {
        coord.x = col - 'A';
    }
    else if (col >= 'a' && col <= 'j') {
        coord.x = col - 'a';
    }
    else {
        return coord;
    }

    if (row >= 1 && row <= GRID_SIZE) {
        coord.y = row - 1;
    }
    else {
        coord.y = -1;
    }

    return coord;
}

// Clear the console screen
void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

// Main game loop handling turns and checking for win conditions
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

// Swap the current player with the opponent
void swapPlayers(Player** currentPlayer, Player** opponent) {
    Player* temp = *currentPlayer;
    *currentPlayer = *opponent;
    *opponent = temp;
}

// Swap the current fleet with the opponent's fleet
void swapFleets(Fleet** currentFleet, Fleet** opponentFleet) {
    Fleet* temp = *currentFleet;
    *currentFleet = *opponentFleet;
    *opponentFleet = temp;
}

// Perform a player's move, handling both human and bot turns
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

        if (player->isBot) {
            // Bot's turn: decide on a move
            // Enhanced targeting for FIRE mode only

            // Prioritize special moves
            if (player->artilleryAvailable) {
                // Choose a random coordinate for artillery
                Coordinate coord = { rand() % GRID_SIZE, rand() % GRID_SIZE };
                printf("Bot uses Artillery at %c%d.\n", 'A' + coord.x, coord.y + 1);
                artillery(player, opponent, opponentFleet, coord, hardMode);
                player->artilleryAvailable = false;
                validMove = true;
            }
            else if (player->torpedoAvailable) {
                // Choose to torpedo a random row or column
                if (rand() % 2 == 0) {
                    // Torpedo a random column
                    char col = 'A' + (rand() % GRID_SIZE);
                    char colStr[2] = { col, '\0' };
                    printf("Bot uses Torpedo at column %c.\n", col);
                    torpedo(player, opponent, opponentFleet, colStr, hardMode);
                }
                else {
                    // Torpedo a random row
                    int row = (rand() % GRID_SIZE) + 1;
                    char rowStr[3];
                    sprintf(rowStr, "%d", row);
                    printf("Bot uses Torpedo at row %d.\n", row);
                    torpedo(player, opponent, opponentFleet, rowStr, hardMode);
                }
                player->torpedoAvailable = false;
                validMove = true;
            }
            else if (player->radarSweepsUsed < MAX_RADAR_SWEEPS) {
                // Use radar sweep
                Coordinate coord = { rand() % GRID_SIZE, rand() % GRID_SIZE };
                printf("Bot uses Radar at %c%d.\n", 'A' + coord.x, coord.y + 1);
                radarSweep(player, opponent, coord);
                player->radarSweepsUsed++;
                validMove = true;
            }
            else if (player->smokeScreensUsed < player->shipsSunk) {
                // Deploy smoke screen
                Coordinate coord = { rand() % GRID_SIZE, rand() % GRID_SIZE };
                printf("Bot deploys a Smoke Screen at %c%d.\n", 'A' + coord.x, coord.y + 1);
                smokeScreen(player, coord);
                validMove = true;
            }
            else {
                // Default to firing with enhanced targeting
                Coordinate coord;
                bool validCoord = false;
                if (player->hasTarget && player->targetQueueCount > 0) {
                    // If there are targets in the queue, prioritize them
                    coord = player->targetQueue[--player->targetQueueCount];
                }
                else {
                    // Fire randomly
                    while (!validCoord) {
                        coord.x = rand() % GRID_SIZE;
                        coord.y = rand() % GRID_SIZE;
                        char cell = opponent->grid[coord.y][coord.x];
                        if (cell != 'o' && cell != 'X') {
                            validCoord = true;
                        }
                    }
                }
                char sunkShipName[20] = "";
                printf("Bot fires at %c%d.\n", 'A' + coord.x, coord.y + 1);
                int result = fire(player, opponent, opponentFleet, coord, hardMode, sunkShipName);
                if (result == 0) {
                    printf("Miss!\n");
                }
                else if (result == 1) {
                    printf("Hit!\n");
                    // Add adjacent targets to the queue
                    if (!player->hasTarget) {
                        player->hasTarget = true;
                        // Define adjacent coordinates: up, right, down, left
                        Coordinate adj[4];
                        adj[0].x = coord.x;
                        adj[0].y = coord.y - 1;
                        adj[1].x = coord.x + 1;
                        adj[1].y = coord.y;
                        adj[2].x = coord.x;
                        adj[2].y = coord.y + 1;
                        adj[3].x = coord.x - 1;
                        adj[3].y = coord.y;
                        for (int i = 0; i < 4; i++) {
                            // Check if within grid and not already targeted
                            if (adj[i].x >=0 && adj[i].x < GRID_SIZE && adj[i].y >=0 && adj[i].y < GRID_SIZE) {
                                char cell = opponent->grid[adj[i].y][adj[i].x];
                                if (cell != 'o' && cell != 'X') {
                                    // Add to queue if not already in queue
                                    bool exists = false;
                                    for (int j = 0; j < player->targetQueueCount; j++) {
                                        if (player->targetQueue[j].x == adj[i].x && player->targetQueue[j].y == adj[i].y) {
                                            exists = true;
                                            break;
                                        }
                                    }
                                    if (!exists && player->targetQueueCount < 4) {
                                        player->targetQueue[player->targetQueueCount++] = adj[i];
                                    }
                                }
                            }
                        }
                    }
                }
                else if (result == 2) {
                    printf("Hit!\n");
                    printf("Bot sunk your %s!\n", sunkShipName);
                    // Unlock special moves after sinking a ship
                    unlockSpecialMoves(player, opponent);
                    // Reset targeting
                    player->hasTarget = false;
                    player->targetQueueCount = 0;
                }
                else if (result == 3) {
                    printf("Already targeted this coordinate.\n");
                }
                validMove = true;
            }

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
                }
                else if (result == 1) {
                    printf("Hit!\n");
                    // Add adjacent targets to the queue
                    if (!player->hasTarget) {
                        player->hasTarget = true;
                        // Define adjacent coordinates: up, right, down, left
                        Coordinate adj[4];
                        adj[0].x = coord.x;
                        adj[0].y = coord.y - 1;
                        adj[1].x = coord.x + 1;
                        adj[1].y = coord.y;
                        adj[2].x = coord.x;
                        adj[2].y = coord.y + 1;
                        adj[3].x = coord.x - 1;
                        adj[3].y = coord.y;
                        for (int i = 0; i < 4; i++) {
                            // Check if within grid and not already targeted
                            if (adj[i].x >=0 && adj[i].x < GRID_SIZE && adj[i].y >=0 && adj[i].y < GRID_SIZE) {
                                char cell = opponent->grid[adj[i].y][adj[i].x];
                                if (cell != 'o' && cell != 'X') {
                                    // Add to queue if not already in queue
                                    bool exists = false;
                                    for (int j = 0; j < player->targetQueueCount; j++) {
                                        if (player->targetQueue[j].x == adj[i].x && player->targetQueue[j].y == adj[i].y) {
                                            exists = true;
                                            break;
                                        }
                                    }
                                    if (!exists && player->targetQueueCount < 4) {
                                        player->targetQueue[player->targetQueueCount++] = adj[i];
                                    }
                                }
                            }
                        }
                    }
                }
                else if (result == 2) {
                    printf("Hit!\n");
                    printf("You sunk the opponent's %s!\n", sunkShipName);
                    // Unlock special moves after sinking a ship
                    unlockSpecialMoves(player, opponent);
                    // Reset targeting
                    player->hasTarget = false;
                    player->targetQueueCount = 0;
                }
                else if (result == 3) {
                    printf("Already targeted this coordinate.\n");
                }
                validMove = true;
                printf("Press Enter to continue...");
                fflush(stdout);
                getchar();
            }
            else {
                printf("Invalid coordinates.\n");
                printf("You lose your turn. Press Enter to continue...");
                fflush(stdout);
                getchar();
                return; // Player loses turn
            }
        }
        else if (strcmp(command, "radar") == 0) {
            if (player->radarSweepsUsed < MAX_RADAR_SWEEPS) {
                Coordinate coord = parseCoordinate(argument);
                if (coord.x != -1 && coord.y != -1) {
                    radarSweep(player, opponent, coord);
                    player->radarSweepsUsed++;
                    validMove = true;
                    printf("Press Enter to continue...");
                    fflush(stdout);
                    getchar();
                }
                else {
                    printf("Invalid coordinates.\n");
                    printf("You lose your turn. Press Enter to continue...");
                    fflush(stdout);
                    getchar();
                    return; // Player loses turn
                }
            }
            else {
                printf("You cannot deploy a radar sweep as you have reached the limit.\n");
                printf("You lose your turn. Press Enter to continue...");
                fflush(stdout);
                getchar();
                return; // Player loses turn
            }
        }
        else if (strcmp(command, "smoke") == 0) {
            if (player->smokeScreensUsed < player->shipsSunk) {
                Coordinate coord = parseCoordinate(argument);
                if (coord.x != -1 && coord.y != -1) {
                    if (smokeScreen(player, coord)) { // Updated function usage
                        validMove = true;
                        printf("Press Enter to continue...");
                        fflush(stdout);
                        getchar();
                    }
                    else {
                        // Failed to deploy smoke screen
                        return; // Player loses turn
                    }
                }
                else {
                    printf("Invalid coordinates.\n");
                    printf("You lose your turn. Press Enter to continue...");
                    fflush(stdout);
                    getchar();
                    return; // Player loses turn
                }
            }
            else {
                printf("You cannot deploy a smoke screen as you have reached the limit.\n");
                printf("You lose your turn. Press Enter to continue...");
                fflush(stdout);
                getchar();
                return; // Player loses turn
            }
        }
        else if (strcmp(command, "artillery") == 0) {
            Coordinate coord = parseCoordinate(argument);
            if (coord.x != -1 && coord.y != -1) {
                artillery(player, opponent, opponentFleet, coord, hardMode); // Revised function
                player->artilleryAvailable = false;
                validMove = true;
                printf("Press Enter to continue...");
                fflush(stdout);
                getchar();
            }
            else {
                printf("Invalid coordinates.\n");
                printf("You lose your turn. Press Enter to continue...");
                fflush(stdout);
                getchar();
                return; // Player loses turn
            }
        }
        else if (strcmp(command, "torpedo") == 0) {
            if (argument[0] != '\0') {
                torpedo(player, opponent, opponentFleet, argument, hardMode);
                player->torpedoAvailable = false;
                validMove = true;
                printf("Press Enter to continue...");
                fflush(stdout);
                getchar();
            }
            else {
                printf("Invalid input.\n");
                printf("You lose your turn. Press Enter to continue...");
                fflush(stdout);
                getchar();
                return; // Player loses turn
            }
        }
    }
}

// Handle deploying a smoke screen (Updated Function)
bool smokeScreen(Player* player, Coordinate coord) {
    // Validate coordinates are within the grid boundaries
    if (coord.x < 0 || coord.x >= GRID_SIZE || coord.y < 0 || coord.y >= GRID_SIZE) {
        printf("Invalid coordinates. Smoke screen not deployed.\n");
        return false;
    }

    // Check if the player has smoke screens available based on ships sunk
    if (player->smokeScreensUsed >= player->shipsSunk) {
        printf("No smoke screens available. You must sink more ships to use another smoke screen.\n");
        return false;
    }

    // Deploy the smoke screen by setting its coordinates and active status
    player->smokeScreens[player->smokeScreensUsed].x = coord.x;
    player->smokeScreens[player->smokeScreensUsed].y = coord.y;
    player->smokeScreens[player->smokeScreensUsed].active = true;
    player->smokeScreensUsed++;

    printf("Smoke screen deployed.\n");
    clearScreen();
    return true;
}

// Handle firing at a specific coordinate
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
            }
            else {
                return 1; // Hit but not sunk
            }
        }
    }
    else if (cell == 'o' || cell == 'X') {
        return 3; // Already targeted
    }
    else {
        // Handle any unforeseen cases gracefully
        printf("Error: Unexpected cell value '%c' at (%d, %d).\n", cell, coord.x, coord.y);
        return 3; // Treat as already targeted to maintain game flow
    }
}

// Perform a radar sweep at the specified coordinate
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

    // Proceed to check for ships within the 2x2 area
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
    }
    else {
        printf("No enemy ships detected within the radar sweep area.\n");
    }
}

// Perform an artillery strike at the specified coordinate
void artillery(Player* player, Player* opponent, Fleet* opponentFleet, Coordinate coord, bool hardMode) {
    int totalHits = 0;
    int totalMisses = 0;
    char sunkShips[SHIP_TYPES][20] = { "" };
    int sunkShipsCount = 0;

    // Define the start and end coordinates for the 2x2 artillery strike
    int xStart = coord.x;
    int yStart = coord.y;
    int xEnd = coord.x + 1;
    int yEnd = coord.y + 1;

    // Handle edge cases to prevent out-of-bounds access
    if (xStart < 0) xStart = 0;
    if (yStart < 0) yStart = 0;
    if (xEnd >= GRID_SIZE) xEnd = GRID_SIZE - 1;
    if (yEnd >= GRID_SIZE) yEnd = GRID_SIZE - 1;

    printf("Artillery strike results at %c%d:\n", 'A' + coord.x, coord.y + 1);

    // Perform the artillery strike on the defined 2x2 area
    for (int i = yStart; i <= yEnd; i++) {
        for (int j = xStart; j <= xEnd; j++) {
            Coordinate tempCoord = { j, i };
            char sunkShipName[20] = "";
            int result = fire(player, opponent, opponentFleet, tempCoord, hardMode, sunkShipName);

            if (result == 0) {
                totalMisses++;
            }
            else if (result == 1) {
                totalHits++;
            }
            else if (result == 2) {
                totalHits++;
                strcpy(sunkShips[sunkShipsCount++], sunkShipName);
            }
            // Note: Result 3 (already targeted) is not counted in this version
        }
    }

    // If the player is a bot, track the last artillery coordinates and hits
    if (player->isBot) {
        player->lastArtilleryHits = totalHits;
        player->lastArtilleryCoord = coord;
    }

    printf("Total Hits: %d\nTotal Misses: %d\n", totalHits, totalMisses);

    // Notify about any sunk ships
    if (sunkShipsCount > 0) {
        for (int i = 0; i < sunkShipsCount; i++) {
            if (player->isBot) {
                printf("%s sunk your %s!\n", player->name, sunkShips[i]);
            }
            else {
                printf("You sunk the opponent's %s!\n", sunkShips[i]);
            }
        }
        // Unlock special moves if any ships were sunk
        unlockSpecialMoves(player, opponent);
    }
}

// Perform a torpedo attack based on user input
void torpedo(Player* player, Player* opponent, Fleet* opponentFleet, const char* input, bool hardMode) {
    int totalHits = 0;
    int totalMisses = 0;
    char sunkShips[SHIP_TYPES][20] = { "" };
    int sunkShipsCount = 0;

    printf("Torpedo attack results on %s:\n", isalpha(input[0]) ? "column" : "row");

    if (isalpha(input[0])) {
        // Torpedoing a column
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
            }
            else if (result == 1) {
                totalHits++;
            }
            else if (result == 2) {
                totalHits++;
                strcpy(sunkShips[sunkShipsCount++], sunkShipName);
            }
        }
    }
    else {
        // Torpedoing a row
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
            }
            else if (result == 1) {
                totalHits++;
            }
            else if (result == 2) {
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
            }
            else {
                printf("You sunk the opponent's %s!\n", sunkShips[i]);
            }
        }
        // Unlock special moves if any ships were sunk
        unlockSpecialMoves(player, opponent);
    }
}

// Check if all ships in the fleet are sunk
bool checkWin(Fleet* fleet) {
    for (int i = 0; i < SHIP_TYPES; i++) {
        if (!fleet->ships[i].sunk) {
            return false;
        }
    }
    return true;
}

// Update the sunk status of a ship based on hits
void updateShipStatus(Ship* ship) {
    if (ship->hits >= ship->size) {
        ship->sunk = true;
    }
}

// Unlock special moves after sinking ships
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

// Display the opponent's tracking grid to the player
void displayTrackingGrid(Player* player, bool hardMode) {
    printf("Opponent's Grid:\n");
    displayGrid(player->trackingGrid, !hardMode);
}

// Validate user commands based on availability
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

// Get user input safely
void getInput(char* input, int size) {
    if (fgets(input, size, stdin) != NULL) {
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] != '\n') {
            flushInputBuffer();
        }
        input[strcspn(input, "\n")] = '\0'; // Remove newline character
    }
}

// Convert coordinates to string representation
void coordinateToString(Coordinate coord, char* coordStr) {
    coordStr[0] = 'A' + coord.x;
    sprintf(&coordStr[1], "%d", coord.y + 1);
    coordStr[strlen(coordStr)] = '\0';
}

// Convert a string to lowercase
void toLowerCase(char* str) {
    for (; *str; ++str) *str = tolower(*str);
}

// Flush the input buffer to remove any extraneous input
void flushInputBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}
