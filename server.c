#include <stdio.h>
#include <pthread.h>
#include <ncurses.h>
#include "communicator.h"
#include "server.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

int killThreads = 0;
int wildBeastCount = 0;
#define LABYRINTH_WIDTH  51
#define LABYRINTH_HEIGHT 25

#define MAX_DROPPED_TREASURE_COUNT 50

#define MAP_FILENAME "map.txt"
#define WALL_CHAR '█'
#define WALL_CHAR_REPLACED 'O'
#define BUSH_CHAR '#'

#define MINIMAL_PLAYERS_TO_PLAY 1

int debugging_counter = 0;


field_status_t fieldStatus[LABYRINTH_HEIGHT][LABYRINTH_WIDTH];

dropped_treasure_t droppedTreasure[MAX_DROPPED_TREASURE_COUNT];
dropped_treasure_status_t droppedTreasureStatus[MAX_DROPPED_TREASURE_COUNT];
int droppedTreasureCount;

struct communicator_t *playerCommunicator[MAX_PLAYER_COUNT];
player_connector_t *playerSharedConnector;
struct players_t *players; // Not for players processes

WINDOW *win;
WINDOW *inputWindow;

pthread_t playerListenerThread, inputListenerThread, playerKeyListener;

// <Statistics here:> //

int xCaptionStartLoc = 1;
int yCaptionStartLoc = LABYRINTH_WIDTH + 1 + 3;

__pid_t serverProcessId;

int campsiteXCoordinate;
int campsiteYCoordinate;

int roundNumber = 0;

// </Statistics here:> //

void updateRoundNumber(void) {
    mvprintw(xCaptionStartLoc + 2, yCaptionStartLoc, "Round number: %d", ++roundNumber);
}

void updatePlayerPosition(int index) {
    int xAt = players->players[index].xPosition;
    int yAt = players->players[index].yPosition;

    mvprintw(7, 68 + index * 9, "%d/%d", xAt, yAt);
    refresh();
}
void clearPlayerPosition(int index) {
    mvprintw(7, 68 + index * 9, "     ");
    refresh();
}

void updatePlayerProcessID(int index, __pid_t playerID) {
    mvprintw(6, 68 + index * 9, "%d", playerID);
    refresh();
}

void clearPlayerProcessID(int index) {
    mvprintw(6, 68 + index * 9, "      ");
    refresh();
}

void updatePlayerDeathsCount(int index) {
    int playerDeaths = players->players[index].deaths;
    mvprintw(8, 68 + index * 9, "%d", playerDeaths);
    refresh();
}

void clearPlayerDeathsCount(int index) {
    mvprintw(8, 68 + index * 9, "    ");
    refresh();
}

void updatePlayerCarriedCoins(int index) {
    int coinsCarried = players->players[index].coinsCarried;
    mvprintw(11, 68 + index * 9, "%d", coinsCarried);

    refresh();
}

void clearPlayerCarriedCoins(int index) {
    mvprintw(11, 68 + index * 9, "    ");
    refresh();
}

void updatePlayerBroughtCoins(int index) {
    int coinsBrought = players->players[index].coinsBrought;
    mvprintw(12, 68 + index * 9, "%d", coinsBrought);

    refresh();
}

void clearPlayerBroughtCoins(int index) {
    mvprintw(12, 68 + index * 9, "    ");
    refresh();
}

void createAndDisplayServerStatistics(void) {
    serverProcessId = getpid();

    roundNumber = 0;
    int i = 0;
    mvprintw(xCaptionStartLoc + i++, yCaptionStartLoc, "Server's PID: %d", serverProcessId);
    mvprintw(xCaptionStartLoc + i++, yCaptionStartLoc, "Campsite X/Y: %d/%d", campsiteXCoordinate, campsiteYCoordinate);
    mvprintw(xCaptionStartLoc + i++, yCaptionStartLoc, "Round number: %d", roundNumber);
    i++;
    mvprintw(xCaptionStartLoc + i, yCaptionStartLoc, "%s", "Parameter:   Player1  Player2  Player3  Player4");
    i++;
    mvprintw(xCaptionStartLoc + i++, yCaptionStartLoc, "%s", "PID");
//    mvprintw(xCaptionStartLoc + i++, yCaptionStartLoc, "%s", "Type");
    mvprintw(xCaptionStartLoc + i++, yCaptionStartLoc, "%s", "Curr X/Y");
    mvprintw(xCaptionStartLoc + i++, yCaptionStartLoc, "%s", "Deaths");
    i++;
    mvprintw(xCaptionStartLoc + i++, yCaptionStartLoc, "%s", "Coins");
    mvprintw(xCaptionStartLoc + i++, yCaptionStartLoc, "%s", "\tCarried");
    mvprintw(xCaptionStartLoc + i, yCaptionStartLoc, "%s", "\tBrought");
    // TODO legend

    refresh();
}

void prepareServer(void) {
    noecho();
    droppedTreasureCount = 0;

    for (int i = 0; i < MAX_DROPPED_TREASURE_COUNT; i++)
        droppedTreasureStatus[i] = NOT_EXISTS;

    readMap();
    players = calloc(1, sizeof(struct players_t));
}

void *inputListener(__attribute__((unused)) void *ptr) {
    int inputChar;
    noecho();
    do {
        inputChar = getch();
        int *arr = getRandomFreePosition();
        int x = arr[0];
        int y = arr[1];
        if (inputChar == 'c') {
            fieldStatus[x][y] = ONE_COIN;
            mvwprintw(win, x, y, "c");
        }
        if (inputChar == 't') {
            fieldStatus[x][y] = TREASURE;
            mvwprintw(win, x, y, "t");
        }
        if (inputChar == 'T') {
            fieldStatus[x][y] = LARGE_TREASURE;
            mvwprintw(win, x, y, "T");
        }

        if (inputChar == 'q' || inputChar == 'Q') {
            printf("Finalizing");
            free(arr);
            finalize();
        }

        if (inputChar == 'B' || inputChar == 'b') {
            if (wildBeastCount != MAX_BEAST_COUNT) {
                int *arrBeast = getRandomFreePosition();
                int xBeast = arrBeast[0], yBeast = arrBeast[1];

                mvwprintw(win, xBeast, yBeast, "*");
                fieldStatus[xBeast][yBeast] = WILD_BEAST;

                players->wildBeast[wildBeastCount].xPosition = xBeast;
                players->wildBeast[wildBeastCount].yPosition = yBeast;
                wildBeastCount++;
                wrefresh(win);
                refresh();
            }
        }

        wrefresh(win);
        refresh();

        free(arr);
    } while (1);

    return NULL;
}

void erasePlayer(int index) {
    int x, y;
    x = players->players[index].xPosition;
    y = players->players[index].yPosition;

    fieldStatus[x][y] = FREE_BLOCK;

    mvwprintw(win, x, y, " ");
    wrefresh(win);
    refresh();
}

void *playerActionListener(void *ptr) {

    for (int i = 0; i < MAX_PLAYER_COUNT; i++)
        playerCommunicator[i]->currentlyMoving = 0;

    // sleep(2u);
    inf_loop:

    while (players->totalPlayers >= MINIMAL_PLAYERS_TO_PLAY) {

        for (int i = 0; i < MAX_PLAYER_COUNT; i++) {
            if (playerCommunicator[i]->playerStatus == CONNECTED) {

                playerCommunicator[i]->currentlyMoving = 1;
                sem_post(&playerCommunicator[i]->communicatorSemaphore1);

                sleep(ROUND_DURATION_SECONDS);


                if (playerCommunicator[i]->hasJustDisconnected == 1) {
                    erasePlayer(i);
                    playerCommunicator[i]->playerStatus = NOT_CONNECTED;


                    playerSharedConnector->totalPlayerCount--;
                    playerSharedConnector->freeIndex = findFreeIndex();


                    clearPlayerPosition(i);
                    clearPlayerProcessID(i);
                    clearPlayerDeathsCount(i);
                    clearPlayerCarriedCoins(i);
                    clearPlayerBroughtCoins(i);

                    //

                    continue;
                }
                mvprintw(17 + debugging_counter++, 60, "Player %d gave input: %d", i + 1,
                         playerCommunicator[i]->playerInput);

                wrefresh(win);
                refresh();

                player_move_dir moveDir = getMoveDirFromInput(playerCommunicator[i]->playerInput);


                movePlayer(i, moveDir);
                sem_post(&playerCommunicator[i]->communicatorSemaphore2);
                mvprintw(17 + debugging_counter++, 60, "Player %d Move direction: %d", i + 1, moveDir);
                debugging_counter++;
                wrefresh(win);
                refresh();

                playerCommunicator[i]->currentlyMoving = 0;

            }

            sleep(BETWEEN_ROUNDS_SLEEP);

        }

        for (int i = 0; i < wildBeastCount; i++) {
            wrefresh(win);
            refresh();
            moveBeast(i);
        }
    }

    goto inf_loop;

    return NULL;
}

_Noreturn void *playerConnector(__attribute__((unused)) void *ptr) {

    while (1) {

        sem_wait(&playerSharedConnector->connectorSemaphore1);

        int indexAt = findFreeIndex();


        playerCommunicator[indexAt]->playerStatus = CONNECTED;

        playerSharedConnector->freeIndex = findFreeIndex();

        int *arr = getRandomFreePosition();

        fieldStatus[arr[0]][arr[1]] = getStatusFromIndex(indexAt);

        playerCommunicator[indexAt]->currentlyAtX = arr[0];
        playerCommunicator[indexAt]->currentlyAtY = arr[1];

        players->players[indexAt].xPosition = arr[0];
        players->players[indexAt].yPosition = arr[1];

        players->players[indexAt].xStartPosition = arr[0];
        players->players[indexAt].yStartPosition = arr[1];

        playerCommunicator[indexAt]->coinsPicked = 0;
        playerCommunicator[indexAt]->coinsBrought = 0;
        playerCommunicator[indexAt]->deaths = 0;

        updatePlayerDeathsCount(indexAt);
        updatePlayerCarriedCoins(indexAt);
        updatePlayerBroughtCoins(indexAt);

        playerCommunicator[indexAt]->isCollision = 0;
        playerCommunicator[indexAt]->locked = 0;
        playerCommunicator[indexAt]->hasJustDisconnected = 0;

        updatePlayerProcessID(indexAt, playerCommunicator[indexAt]->playerProcessID);
        updatePlayerPosition(indexAt);

        paintPlayer(indexAt, arr[0], arr[1]);

        wrefresh(win);
        refresh();

        players->players[indexAt].deaths = 0;
        players->players[indexAt].coinsCarried = 0;
        players->players[indexAt].coinsBrought = 0;
        players->totalPlayers += 1;
        fillSharedMap(indexAt);
        playerSharedConnector->totalPlayerCount++;


        free(arr);


    }
}

void createConnector(void) {
    key_t key = ftok(FILE_CONNECTOR, 0);

    int sharedBlockId = shmget(key, sizeof(player_connector_t), IPC_CREAT);

    playerSharedConnector =
            (player_connector_t *) shmat(sharedBlockId, NULL, 0);

    sem_init(&playerSharedConnector->connectorSemaphore1, 1, 0);
    sem_init(&playerSharedConnector->connectorSemaphore2, 1, 0);
    playerSharedConnector->totalPlayerCount = 0;
    playerSharedConnector->playerConnected = 0;
    playerSharedConnector->freeIndex = 0;
    playerSharedConnector->rounds = 0;
    playerSharedConnector->serverPid = getpid();

}

void createCommunicator(void) {

    for (int i = 0; i < MAX_PLAYER_COUNT; i++) {

        key_t key = ftok(FILE_COMMUNICATOR, i);

        int sharedBlockId =
                shmget(key, sizeof(struct communicator_t), 0644 | IPC_CREAT);

        *(playerCommunicator + i) =
                (struct communicator_t *) shmat(sharedBlockId, NULL, 0);


        sem_init(&(*(playerCommunicator + i))->communicatorSemaphore1, 1, 0);
        sem_init(&(*(playerCommunicator + i))->communicatorSemaphore2, 1, 0);
        sem_init(&(*(playerCommunicator + i))->communicatorSemaphore3, 1, 0);


        (*(playerCommunicator + i))->playerIndex = i;
        (*(playerCommunicator + i))->playerStatus = NOT_CONNECTED;

        (*(playerCommunicator + i))->mapAround.campsiteX = campsiteXCoordinate;
        (*(playerCommunicator + i))->mapAround.campsiteY = campsiteYCoordinate;
        (*(playerCommunicator + i))->hasJustDisconnected = 0;

    }
}

void displayMap(void) {
    initscr();

    win = newwin(LABYRINTH_HEIGHT, LABYRINTH_WIDTH, 1, 1);

    refresh();

    init_pair(1, COLOR_BLACK, COLOR_YELLOW);
    wbkgd(win, COLOR_PAIR(1));

    for (int i = 0; i < LABYRINTH_HEIGHT; i++) {
        for (int j = 0; j < LABYRINTH_WIDTH; j++) {
            field_status_t buffer = fieldStatus[i][j];
            if (buffer == WALL)
                wprintw(win, "%c", WALL_CHAR_REPLACED);
            else if (buffer == LARGE_TREASURE)
                wattron(win, COLOR_PAIR(1)), wprintw(win, "%c", 'T'), wattroff(win, COLOR_PAIR(1));
            else if (buffer == TREASURE)
                wattron(win, COLOR_PAIR(1)), wprintw(win, "%c", 't'), wattroff(win, COLOR_PAIR(1));
            else if (buffer == ONE_COIN)
                wattron(win, COLOR_PAIR(1)), wprintw(win, "%c", 'c'), wattroff(win, COLOR_PAIR(1));
            else if (buffer == BUSHES)
                wattron(win, COLOR_PAIR(1)), wprintw(win, "%c", '#'), wattroff(win, COLOR_PAIR(1));
            else if (buffer == CAMPSITE)
                wattron(win, COLOR_PAIR(1)), wprintw(win, "%c", 'A'), wattroff(win, COLOR_PAIR(1));
            else if (buffer == DROPPED_TREASURE)
                wattron(win, COLOR_PAIR(1)), wprintw(win, "%c", 'D'), wattroff(win, COLOR_PAIR(1));
            else wprintw(win, "%c", ' ');
        }
        wmove(win, i + 1, 0);
    }

    wrefresh(win);
    refresh();
}



void moveBeast(int index) {
    srand((unsigned int) time(NULL));


    int xBeast = players->wildBeast[index].xPosition;
    int yBeast = players->wildBeast[index].yPosition;
    mvprintw(8, 65, "%d %d", xBeast, yBeast);
    field_status_t beastAt = players->wildBeast[index].currentlyOn;

    beast_move_dir beastMoveDir;
    field_status_t fieldStatusTo;

    int nextMoveDetermined = 0;
    int willKill = 0;

    if (isPlayer(fieldStatus[xBeast + 1][yBeast]))
        beastMoveDir = MOVE_DOWN, nextMoveDetermined = 1, willKill = 1;
    if (isPlayer(fieldStatus[xBeast - 1][yBeast]))
        beastMoveDir = MOVE_UP, nextMoveDetermined = 1, willKill = 1;
    if (isPlayer(fieldStatus[xBeast][yBeast + 1]))
        beastMoveDir = MOVE_RIGHT, nextMoveDetermined = 1, willKill = 1;
    if (isPlayer(fieldStatus[xBeast][yBeast - 1]))
        beastMoveDir = MOVE_LEFT, nextMoveDetermined = 1, willKill = 1;

    if (willKill) {
        if (beastMoveDir == MOVE_DOWN) {
            int playerIndex = getIndexFromStatus(fieldStatus[xBeast + 1][yBeast]);
            mvwprintw(win, xBeast + 1, yBeast, "*");
            fieldStatus[xBeast + 1][yBeast] = WILD_BEAST;

            int xPlayer = players->players[playerIndex].xStartPosition;
            int yPlayer = players->players[playerIndex].yStartPosition;

            while (fieldStatus[xPlayer][yPlayer] != FREE_BLOCK) {
                int *arr = getRandomFreePosition();
                xPlayer = arr[0];
                yPlayer = arr[1];

                free(arr);
            }

            int droppedTreasureVal = players->players[playerIndex].coinsCarried;
            players->players[playerIndex].coinsCarried = 0;

            if (droppedTreasureVal) {
                int droppedTreasureIndex = getFreeTreasurePos();
                droppedTreasure[droppedTreasureIndex].coins = droppedTreasureVal;
                droppedTreasure[droppedTreasureIndex].x = xBeast;
                droppedTreasure[droppedTreasureIndex].y = yBeast;
                droppedTreasureStatus[droppedTreasureIndex] = EXIST;
            }
            players->wildBeast[index].xPosition++;

            fieldStatus[xPlayer][yPlayer] = getStatusFromIndex(playerIndex);
            mvwprintw(win, xPlayer, yPlayer, "%c", '0' + playerIndex);
            wrefresh(win);
            refresh();

            return;
        }
        if (beastMoveDir == MOVE_UP) {
            int playerIndex = getIndexFromStatus(fieldStatus[xBeast - 1][yBeast]);
            mvwprintw(win, xBeast - 1, yBeast, "*");
            fieldStatus[xBeast - 1][yBeast] = WILD_BEAST;

            int xPlayer = players->players[playerIndex].xStartPosition;
            int yPlayer = players->players[playerIndex].yStartPosition;

            while (fieldStatus[xPlayer][yPlayer] != FREE_BLOCK) {
                int *arr = getRandomFreePosition();
                xPlayer = arr[0];
                yPlayer = arr[1];

                free(arr);
            }

            int droppedTreasureVal = players->players[playerIndex].coinsCarried;
            players->players[playerIndex].coinsCarried = 0;

            if (droppedTreasureVal) {
                int droppedTreasureIndex = getFreeTreasurePos();
                droppedTreasure[droppedTreasureIndex].coins = droppedTreasureVal;
                droppedTreasure[droppedTreasureIndex].x = xBeast;
                droppedTreasure[droppedTreasureIndex].y = yBeast;
                droppedTreasureStatus[droppedTreasureIndex] = EXIST;
            }

            players->wildBeast[index].xPosition--;


            fieldStatus[xPlayer][yPlayer] = getStatusFromIndex(playerIndex);
            mvwprintw(win, xPlayer, yPlayer, "%c", '0' + playerIndex);
            wrefresh(win);
            refresh();
            return;
        }
        if (beastMoveDir == MOVE_RIGHT) {
            int playerIndex = getIndexFromStatus(fieldStatus[xBeast][yBeast + 1]);
            mvwprintw(win, xBeast, yBeast + 1, "*");
            fieldStatus[xBeast][yBeast + 1] = WILD_BEAST;

            int xPlayer = players->players[playerIndex].xStartPosition;
            int yPlayer = players->players[playerIndex].yStartPosition;

            while (fieldStatus[xPlayer][yPlayer] != FREE_BLOCK) {
                int *arr = getRandomFreePosition();
                xPlayer = arr[0];
                yPlayer = arr[1];

                free(arr);
            }

            int droppedTreasureVal = players->players[playerIndex].coinsCarried;
            players->players[playerIndex].coinsCarried = 0;

            if (droppedTreasureVal) {
                int droppedTreasureIndex = getFreeTreasurePos();
                droppedTreasure[droppedTreasureIndex].coins = droppedTreasureVal;
                droppedTreasure[droppedTreasureIndex].x = xBeast;
                droppedTreasure[droppedTreasureIndex].y = yBeast;
                droppedTreasureStatus[droppedTreasureIndex] = EXIST;
            }
            players->wildBeast[index].yPosition++;

            fieldStatus[xPlayer][yPlayer] = getStatusFromIndex(playerIndex);
            mvwprintw(win, xPlayer, yPlayer, "%c", '0' + playerIndex);
            wrefresh(win);
            refresh();
            return;
        }
        if (beastMoveDir == MOVE_LEFT) {
            int playerIndex = getIndexFromStatus(fieldStatus[xBeast][yBeast - 1]);
            mvwprintw(win, xBeast, yBeast - 1, "*");
            fieldStatus[xBeast][yBeast - 1] = WILD_BEAST;

            int xPlayer = players->players[playerIndex].xStartPosition;
            int yPlayer = players->players[playerIndex].yStartPosition;

            while (fieldStatus[xPlayer][yPlayer] != FREE_BLOCK) {
                int *arr = getRandomFreePosition();
                xPlayer = arr[0];
                yPlayer = arr[1];

                free(arr);
            }

            int droppedTreasureVal = players->players[playerIndex].coinsCarried;
            players->players[playerIndex].coinsCarried = 0;

            if (droppedTreasureVal) {
                int droppedTreasureIndex = getFreeTreasurePos();
                droppedTreasure[droppedTreasureIndex].coins = droppedTreasureVal;
                droppedTreasure[droppedTreasureIndex].x = xBeast;
                droppedTreasure[droppedTreasureIndex].y = yBeast;
                droppedTreasureStatus[droppedTreasureIndex] = EXIST;
            }
            players->wildBeast[index].yPosition--;

            fieldStatus[xPlayer][yPlayer] = getStatusFromIndex(playerIndex);
            mvwprintw(win, xPlayer, yPlayer, "%c", '0' + playerIndex);
            wrefresh(win);
            refresh();
            return;
        }

        return;
    }


    if (xBeast + 2 < LABYRINTH_WIDTH)
        if (fieldStatus[xBeast + 1][yBeast] == FREE_BLOCK && isPlayer(fieldStatus[xBeast + 2][yBeast]))
            beastMoveDir = MOVE_DOWN, nextMoveDetermined = 1;
    if (xBeast - 2 >= 0)
        if (fieldStatus[xBeast - 1][yBeast] == FREE_BLOCK && isPlayer(fieldStatus[xBeast - 2][yBeast]))
            beastMoveDir = MOVE_UP, nextMoveDetermined = 1;
    if (yBeast + 2 < LABYRINTH_HEIGHT)
        if (fieldStatus[xBeast][yBeast + 1] == FREE_BLOCK && isPlayer(fieldStatus[xBeast][yBeast + 2]))
            beastMoveDir = MOVE_RIGHT, nextMoveDetermined = 1;
    if (yBeast - 2 >= 0)
        if (fieldStatus[xBeast][yBeast - 1] == FREE_BLOCK && isPlayer(fieldStatus[xBeast][yBeast - 2]))
            beastMoveDir = MOVE_LEFT, nextMoveDetermined = 1;


    if (nextMoveDetermined == 1) {

        // Chase //
        if (beastMoveDir == MOVE_DOWN) {
            fieldStatus[xBeast][yBeast] = FREE_BLOCK;
            fieldStatus[xBeast + 1][yBeast] = WILD_BEAST;
            mvwprintw(win, xBeast, yBeast, " "); // TODO WHERE IT WAS! //
            mvwprintw(win, xBeast + 1, yBeast, "*");

            wrefresh(win);
            refresh();

            fillSharedMap(index);
            updateRoundNumber();
        }
        if (beastMoveDir == MOVE_UP) {
            fieldStatus[xBeast][yBeast] = FREE_BLOCK;
            fieldStatus[xBeast - 1][yBeast] = WILD_BEAST;

            mvwprintw(win, xBeast, yBeast, " "); // TODO WHERE IT WAS! //
            mvwprintw(win, xBeast - 1, yBeast, "*");

            wrefresh(win);
            refresh();

            fillSharedMap(index);
            updateRoundNumber();
        }
        if (beastMoveDir == MOVE_LEFT) {
            fieldStatus[xBeast][yBeast] = FREE_BLOCK;
            fieldStatus[xBeast][yBeast - 1] = WILD_BEAST;

            mvwprintw(win, xBeast, yBeast, " "); // TODO WHERE IT WAS! //
            mvwprintw(win, xBeast, yBeast - 1, "*");

            wrefresh(win);
            refresh();

            fillSharedMap(index);
            updateRoundNumber();
        }
        if (beastMoveDir == MOVE_RIGHT) {
            fieldStatus[xBeast][yBeast] = FREE_BLOCK;
            fieldStatus[xBeast][yBeast + 1] = WILD_BEAST;

            mvwprintw(win, xBeast, yBeast, " "); // TODO WHERE IT WAS! //
            mvwprintw(win, xBeast, yBeast + 1, "*");

            wrefresh(win);
            refresh();

            fillSharedMap(index);
            updateRoundNumber();
        }

        return;
    }


    int xRand = rand() % 4;
    if (xRand == 0)
        beastMoveDir = MOVE_LEFT;
    if (xRand == 1)
        beastMoveDir = MOVE_UP;
    if (xRand == 2)
        beastMoveDir = MOVE_RIGHT;
    if (xRand == 3)
        beastMoveDir = MOVE_DOWN;




    refresh();
    wrefresh(win);

    if (beastMoveDir == MOVE_LEFT) {
        fieldStatusTo = fieldStatus[xBeast][yBeast - 1];
    }
    if (beastMoveDir == MOVE_RIGHT) {
        fieldStatusTo = fieldStatus[xBeast][yBeast + 1];
    }
    if (beastMoveDir == MOVE_DOWN) {
        fieldStatusTo = fieldStatus[xBeast + 1][yBeast];
    }
    if (beastMoveDir == MOVE_UP) {
        fieldStatusTo = fieldStatus[xBeast - 1][yBeast];
    }


    if (fieldStatusTo == WALL) {
        for (int i = 0; i < MAX_PLAYERS; i++)
            if (playerCommunicator[i]->playerStatus == CONNECTED)
                fillSharedMap(i);

        updateRoundNumber();
        return;

    } else {

        if (beastMoveDir == MOVE_LEFT) {
            players->wildBeast[index].yPosition--;

            fieldStatus[xBeast][yBeast - 1] = WILD_BEAST;
            mvwprintw(win, xBeast, yBeast - 1,  "*");
        }
        if (beastMoveDir == MOVE_RIGHT) {
            players->wildBeast[index].yPosition++;

            fieldStatus[xBeast][yBeast + 1] = WILD_BEAST;
            mvwprintw(win, xBeast, yBeast + 1, "*");
        }
        if (beastMoveDir == MOVE_DOWN) {
            players->wildBeast[index].xPosition++;

            fieldStatus[xBeast + 1][yBeast] = WILD_BEAST;
            mvwprintw(win, xBeast + 1, yBeast, "*");
        }
        if (beastMoveDir == MOVE_UP) {
            players->wildBeast[index].xPosition--;

            fieldStatus[xBeast - 1][yBeast] = WILD_BEAST;
            mvwprintw(win, xBeast - 1, yBeast, "*");
        }

        fieldStatus[xBeast][yBeast] = FREE_BLOCK;
        mvwprintw(win, xBeast, yBeast, " "); // TODO WHERE IT WAS! //

        wrefresh(win);
        refresh();

        for (int i = 0; i < MAX_PLAYERS; i++)
            if (playerCommunicator[i]->playerStatus == CONNECTED)
                fillSharedMap(i);

        updateRoundNumber();
        return;
    }

}

int main(void) {
    createConnector();
    createCommunicator();

    prepareServer();
    displayMap();

    createAndDisplayServerStatistics();

    pthread_create(&playerListenerThread, NULL, playerConnector, NULL);
    pthread_create(&inputListenerThread, NULL, inputListener, NULL);
    pthread_create(&playerKeyListener, NULL, playerActionListener, NULL);

    pthread_join(playerListenerThread, NULL);
    pthread_join(inputListenerThread, NULL);
    pthread_join(playerKeyListener, NULL);

    // sleep(DEBUG_SLEEP);

    endwin();
    // finalize();
    return 0;
}

void finalize(void) {
    // pthread_t playerListenerThread, inputListenerThread, playerKeyListener;
    free(players);

    int conectorIndex = getSharedBlock(FILE_CONNECTOR, sizeof(player_connector_t), 0);
    shmctl(conectorIndex, IPC_RMID, NULL);

    for (int i = 0; i < MAX_PLAYER_COUNT; i++) {
        int controllerIndex = getSharedBlock(FILE_COMMUNICATOR, sizeof(struct communicator_t), i);
        shmctl(controllerIndex, IPC_RMID, NULL);
    }

    endwin();



    pthread_kill(playerListenerThread, SIGKILL);
    pthread_kill(inputListenerThread, SIGKILL);
    pthread_kill(playerKeyListener, SIGKILL);

}

int getSharedBlock(char *filename, size_t size, int index) {
    key_t key = ftok(filename, index);

    return shmget(key, size, IPC_CREAT);
}

void debugging(void) {
    noecho();
    inputWindow = newwin(8, 32, 15, LABYRINTH_WIDTH + 3);
    box(inputWindow, 0, 0);
    refresh();
    wrefresh(inputWindow);

    keypad(inputWindow, true);
    int c;

    do {
        c = wgetch(inputWindow);
        mvwprintw(inputWindow, 1, 1,"Pressed: %c", c);
        if (c == KEY_UP)
            movePlayer(0, MOVE_UP);
        if (c == KEY_DOWN)
            movePlayer(0, MOVE_DOWN);
        if (c == KEY_LEFT)
            movePlayer(0, MOVE_LEFT);
        if (c == KEY_RIGHT)
            movePlayer(0, MOVE_RIGHT);
    } while (c != 'q');

    refresh();
    wrefresh(inputWindow);

}


int movePlayer(int index, player_move_dir playerMoveDir) {
    int xFrom = players->players[index].xPosition, xTo;
    int yFrom = players->players[index].yPosition, yTo;
    field_status_t fieldStatusFrom = fieldStatus[xFrom][yFrom];



    playerSharedConnector->rounds++;


    int willMove;
    int isOnBushes = fieldStatusFrom == getStatusFromIndexBushed(index);

    int isOnCampsite = (xFrom == campsiteXCoordinate) && (yFrom == campsiteXCoordinate);
    int goingToCampsite;

    if (players->players[index].locked)
        return players->players[index].locked = 0, 1;

    if (playerMoveDir == MOVE_UP) {
        xTo = xFrom - 1;
        yTo = yFrom;
    }

    if (playerMoveDir == MOVE_DOWN) {
        xTo = xFrom + 1;
        yTo = yFrom;
    }

    if (playerMoveDir == MOVE_LEFT) {
        xTo = xFrom;
        yTo = yFrom - 1;
    }

    if (playerMoveDir == MOVE_RIGHT) {
        xTo = xFrom;
        yTo = yFrom + 1;
    }

    goingToCampsite = (xTo == campsiteXCoordinate) && (yTo == campsiteXCoordinate);
    field_status_t fieldStatusTo = fieldStatus[xTo][yTo];

    if (fieldStatusTo == FREE_BLOCK) {
        if (isOnCampsite) {
            fieldStatus[xFrom][yFrom] = CAMPSITE;
            mvwprintw(win, xFrom, yFrom, "A");
            wrefresh(win);
            refresh();
        }
        else {
            if (isOnBushes) {
                fieldStatus[xFrom][yFrom] = BUSHES;
                mvwprintw(win, xFrom, yFrom, "#");
                wrefresh(win);
                refresh();
            } else {
                fieldStatus[xFrom][yFrom] = FREE_BLOCK;
                mvwprintw(win, xFrom, yFrom, " ");
                wrefresh(win);
                refresh();
            }
        }

        playerCommunicator[index]->currentlyAtX = xTo;
        playerCommunicator[index]->currentlyAtY = yTo;

        players->players[index].xPosition = xTo;
        players->players[index].yPosition = yTo;

        fieldStatus[xTo][yTo] = getStatusFromIndex(index);

        mvwprintw(win, xTo, yTo, "%c", '1' + index);

        updatePlayerPosition(index);
        fillSharedMap(index);
        updateRoundNumber();

        return 0;
    }
    if (fieldStatusTo == WALL) {
        fillSharedMap(index);
        updateRoundNumber();
        return 1; // Broadcast communicat
    }
    if (fieldStatusTo == LARGE_TREASURE) {
        if (isOnCampsite) {
            fieldStatus[xFrom][yFrom] = CAMPSITE;
            mvwprintw(win, xFrom, yFrom, "A");
            wrefresh(win);
            refresh();
        }

        players->players[index].coinsCarried += LARGE_TREASURE_COINS;
        players->players[index].xPosition = xTo;
        players->players[index].yPosition = yTo;

        if (isOnBushes) {
            fieldStatus[xFrom][yFrom] = BUSHES;
            mvwprintw(win, xFrom, yFrom, "#");
        }
        else {
            fieldStatus[xFrom][yFrom] = FREE_BLOCK;
            mvwprintw(win, xFrom, yFrom, " ");
        }

        fieldStatus[xTo][yTo] = getStatusFromIndex(index);
        mvwprintw(win, xFrom, yFrom, " ");
        mvwprintw(win, xTo, yTo, "%c", ('1' + index));

        playerCommunicator[index]->currentlyAtX = xTo;
        playerCommunicator[index]->currentlyAtY = yTo;
        playerCommunicator[index]->coinsPicked += LARGE_TREASURE_COINS;
        updatePlayerCarriedCoins(index);

        fillSharedMap(index);
        updateRoundNumber();
        return 0;
    }
    if (fieldStatusTo == TREASURE) {
        if (isOnCampsite) {
            fieldStatus[xFrom][yFrom] = CAMPSITE;
            mvwprintw(win, xFrom, yFrom, "A");
            wrefresh(win);
            refresh();
        }

        players->players[index].coinsCarried += TREASURE_COINS;
        players->players[index].xPosition = xTo;
        players->players[index].yPosition = yTo;

        if (isOnBushes) {
            fieldStatus[xFrom][yFrom] = BUSHES;
            mvwprintw(win, xFrom, yFrom, "#");
        }
        else {
            fieldStatus[xFrom][yFrom] = FREE_BLOCK;
            mvwprintw(win, xFrom, yFrom, " ");
        }

        fieldStatus[xTo][yTo] = getStatusFromIndex(index);

        mvwprintw(win, xFrom, yFrom, " ");
        mvwprintw(win, xTo, yTo, "%c", ('1' + index));

        playerCommunicator[index]->currentlyAtX = xTo;
        playerCommunicator[index]->currentlyAtY = yTo;
        playerCommunicator[index]->coinsPicked += TREASURE_COINS;
        updatePlayerCarriedCoins(index);
        fillSharedMap(index);
        updateRoundNumber();
        return 0;
    }
    if (fieldStatusTo == ONE_COIN) {
        if (isOnCampsite) {
            fieldStatus[xFrom][yFrom] = CAMPSITE;
            mvwprintw(win, xFrom, yFrom, "A");
            wrefresh(win);
            refresh();
        }

        players->players[index].coinsCarried += ONE_COIN_COINS;
        players->players[index].xPosition = xTo;
        players->players[index].yPosition = yTo;

        if (isOnBushes) {
            fieldStatus[xFrom][yFrom] = BUSHES;
            mvwprintw(win, xFrom, yFrom, "#");
        }
        else {
            fieldStatus[xFrom][yFrom] = FREE_BLOCK;
            mvwprintw(win, xFrom, yFrom, " ");
        }

        fieldStatus[xTo][yTo] = getStatusFromIndex(index);

        mvwprintw(win, xTo, yTo, "%c", ('1' + index));

        playerCommunicator[index]->currentlyAtX = xTo;
        playerCommunicator[index]->currentlyAtY = yTo;
        playerCommunicator[index]->coinsPicked += ONE_COIN_COINS;
        updatePlayerCarriedCoins(index);
        fillSharedMap(index);
        updateRoundNumber();
        return 0;
    }
    if (fieldStatusTo == BUSHES) {
        if (isOnCampsite) {
            fieldStatus[xFrom][yFrom] = CAMPSITE;
            mvwprintw(win, xFrom, yFrom, "A");
            wrefresh(win);
            refresh();
        }

        players->players[index].xPosition = xTo;
        players->players[index].yPosition = yTo;
        players->players[index].locked = 1;
        fieldStatus[xTo][yTo] = getStatusFromIndexBushed(index);

        if (isOnBushes) {
            fieldStatus[xFrom][yFrom] = BUSHES;
            mvwprintw(win, xFrom, yFrom, "#");
        }
        else {
            fieldStatus[xFrom][yFrom] = FREE_BLOCK;
            mvwprintw(win, xFrom, yFrom, " ");
        }

        mvwprintw(win, xTo, yTo, "%c", ('1' + index));
        fillSharedMap(index);
        updateRoundNumber();

        return 0;
    }
    if (fieldStatusTo == CAMPSITE) {
        players->players[index].xPosition = xTo;
        players->players[index].yPosition = yTo;
        players->players[index].coinsBrought += players->players[index].coinsCarried;

        players->players[index].coinsCarried = 0;

        updatePlayerBroughtCoins(index);
        updatePlayerCarriedCoins(index);

        if (isOnBushes) {
            fieldStatus[xFrom][yFrom] = BUSHES;
            mvwprintw(win, xFrom, yFrom, "#");
            wrefresh(win);
            refresh();
        }
        else {
            fieldStatus[xFrom][yFrom] = FREE_BLOCK;
            mvwprintw(win, xFrom, yFrom, " ");
            wrefresh(win);
            refresh();
        }

        fieldStatus[xTo][yTo] = getStatusFromIndex(index);
        mvwprintw(win, xTo, yTo, "%c", '1' + index);
        wrefresh(win);
        refresh();

        playerCommunicator[index]->currentlyAtX = xTo;
        playerCommunicator[index]->currentlyAtY = yTo;
        playerCommunicator[index]->coinsBrought += playerCommunicator[index]->coinsPicked;
        playerCommunicator[index]->coinsPicked = 0;

        fillSharedMap(index);
        updateRoundNumber();

        return 0;
    }
    if (fieldStatusTo == DROPPED_TREASURE) {
        if (isOnCampsite) {
            fieldStatus[xFrom][yFrom] = CAMPSITE;
            mvwprintw(win, xFrom, yFrom, "A");
            wrefresh(win);
            refresh();
        }

        players->players[index].xPosition = xTo;
        players->players[index].yPosition = yTo;

        // Makes treasure NOT_EXIST
        int droppedTreasureCoins = getDroppedTreasureCoins(xTo, yTo);

        players->players[index].coinsCarried += droppedTreasureCoins;

        if (isOnBushes) {
            fieldStatus[xFrom][yFrom] = BUSHES;
            mvwprintw(win, xFrom, yFrom, "#");
        } else {
            fieldStatus[xFrom][yFrom] = FREE_BLOCK;
            mvwprintw(win, xFrom, yFrom, " ");
        }

        fieldStatus[xTo][yTo] = getStatusFromIndex(index);
        playerCommunicator[index]->currentlyAtX = xTo;
        playerCommunicator[index]->currentlyAtY = yTo;

        playerCommunicator[index]->coinsPicked += droppedTreasureCoins;
        fillSharedMap(index);
        updateRoundNumber();
        return 0;
    }
    if (fieldStatusTo == PLAYER_1) {
        if (isOnCampsite) {
            fieldStatus[xFrom][yFrom] = CAMPSITE;
            mvwprintw(win, xFrom, yFrom, "A");
            wrefresh(win);
            refresh();
        }
        collision(index, 0);

        wrefresh(win);
        refresh();

        fillSharedMap(index);
        fillSharedMap(index);
        updateRoundNumber();
        return 0;
    }
    if (fieldStatusTo == PLAYER_2) {
        if (isOnCampsite) {
            fieldStatus[xFrom][yFrom] = CAMPSITE;
            mvwprintw(win, xFrom, yFrom, "A");
            wrefresh(win);
            refresh();
        }

        collision(index, 1);

        wrefresh(win);
        refresh();

        fillSharedMap(index);
        fillSharedMap(index);
        updateRoundNumber();
        return 0;
    }
    if (fieldStatusTo == PLAYER_3) {
        if (isOnCampsite) {
            fieldStatus[xFrom][yFrom] = CAMPSITE;
            mvwprintw(win, xFrom, yFrom, "A");
            wrefresh(win);
            refresh();
        }
        collision(index, 2);

        wrefresh(win);
        refresh();

        fillSharedMap(index);
        fillSharedMap(index);
        updateRoundNumber();
        return 0;
    }
    if (fieldStatusTo == PLAYER_4) {
        if (isOnCampsite) {
            fieldStatus[xFrom][yFrom] = CAMPSITE;
            mvwprintw(win, xFrom, yFrom, "A");
            wrefresh(win);
            refresh();
        }
        collision(index, 3);

        wrefresh(win);
        refresh();

        fillSharedMap(index);

        fillSharedMap(index);
        updateRoundNumber();

        return 0;
    }
    if (fieldStatusTo == WILD_BEAST) {

        fillSharedMap(index);
        updateRoundNumber();

        return 0;
    }

    players->players[index].xPosition = xTo;
    players->players[index].yPosition = yTo;
    updatePlayerPosition(index);

    wrefresh(win);
    refresh();

    fillSharedMap(index);

    return 0;
}


//                 p1 -> p2
void collision(int index1, int index2) {
    int xFrom1, yFrom1, xTo1, yTo1, xFrom2, yFrom2, xTo2, yTo2;

    xFrom1 = players->players[index1].xPosition;
    yFrom1 = players->players[index1].yPosition;
    xFrom2 = players->players[index2].xPosition;
    yFrom2 = players->players[index2].yPosition;

    xTo1 = players->players[index1].xStartPosition;
    yTo1 = players->players[index1].yStartPosition;
    xTo2 = players->players[index2].xStartPosition;
    yTo2 = players->players[index2].yStartPosition;

    _Bool isOnBush1 = fieldStatus[xFrom1][yFrom1] == getStatusFromIndexBushed(index1);
    _Bool isOnBush2 = fieldStatus[xFrom2][yFrom2] == getStatusFromIndexBushed(index2);

    _Bool isOnCamp = (xFrom2 == campsiteXCoordinate) && (yFrom2 == campsiteYCoordinate);

    while (fieldStatus[xTo1][yTo1] != FREE_BLOCK) {
        int *arr = getRandomFreePosition();
        xTo1 = arr[0];
        yTo1 = arr[1];
        free(arr);
    }

    while (fieldStatus[xTo2][yTo2] != FREE_BLOCK) {
        int *arr = getRandomFreePosition();
        xTo2 = arr[0];
        yTo2 = arr[1];
        free(arr);
    }

    players->players[index1].xPosition = xTo1;
    players->players[index1].yPosition = yTo1;

    players->players[index2].xPosition = xTo2;
    players->players[index2].yPosition = yTo2;

    playerCommunicator[index1]->currentlyAtX = xTo1;
    playerCommunicator[index1]->currentlyAtY = yTo1;

    playerCommunicator[index2]->currentlyAtX = xTo2;
    playerCommunicator[index2]->currentlyAtY = yTo2;

    int droppedTreasureVal =
            players->players[index1].coinsCarried + players->players[index2].coinsCarried;

    int freeTreasure = getFreeTreasurePos();

    droppedTreasure[freeTreasure].x = xTo2;
    droppedTreasure[freeTreasure].y = yTo2;
    droppedTreasure[freeTreasure].coins = droppedTreasureVal;

    players->players[index1].coinsCarried = 0;
    players->players[index2].coinsCarried = 0;

    updatePlayerCarriedCoins(index1);
    updatePlayerCarriedCoins(index2);

    if (isOnBush1) {
        fieldStatus[xFrom1][yFrom1] = BUSHES;
        mvwprintw(win, xFrom1, yFrom1, "#");
        wrefresh(win);
        refresh();
    }
    if (isOnBush2) {
        fieldStatus[xFrom2][yFrom2] = BUSHES;
        mvwprintw(win, xFrom2, yFrom2, "#");
        wrefresh(win);
        refresh();
    }
    if (isOnCamp) {
        fieldStatus[xFrom2][yFrom2] = CAMPSITE;
        mvwprintw(win, xFrom2, yFrom2, "A");
        wrefresh(win);
        refresh();
    }

    fieldStatus[xFrom2][yFrom2] = DROPPED_TREASURE;

    fieldStatus[xTo1][yTo1] = getStatusFromIndex(index1);
    mvwprintw(win, xTo1, yTo1, "%c", index1 + '1');

    fieldStatus[xTo2][yTo2] = getStatusFromIndex(index2);
    mvwprintw(win, xTo2, yTo2, "%c", index2 + '1');

    wrefresh(win);
    refresh();

}

void paintPlayer(int index, int x, int y) {
    mvwprintw(win, x, y, "%c", (char) (index + '1'));
    wrefresh(win);
    refresh();
}

void readMap(void) {
    FILE *fp = fopen(MAP_FILENAME, "r");

    int buffer;

    for (int i = 0; i < LABYRINTH_HEIGHT; i++) {
        for (int j = 0; j < LABYRINTH_WIDTH; j++) {
            buffer = fgetc(fp);
            if (buffer == WALL_CHAR_REPLACED)
                fieldStatus[i][j] = WALL;
            if (buffer == ' ')
                fieldStatus[i][j] = FREE_BLOCK;
            if (buffer == BUSH_CHAR)
                fieldStatus[i][j] = BUSHES;
            if (buffer == 'c')
                fieldStatus[i][j] = ONE_COIN;
            if (buffer == 't')
                fieldStatus[i][j] = TREASURE;
            if (buffer == 'T')
                fieldStatus[i][j] = LARGE_TREASURE;
            if (buffer == 'A')
                fieldStatus[i][j] = CAMPSITE, campsiteXCoordinate = i, campsiteYCoordinate = j;
            if (buffer == 'D')
                fieldStatus[i][j] = DROPPED_TREASURE;
        }

        fgetc(fp);  // "\n"
    }

    fclose(fp);
}

int findFreeIndex(void) {
    for (int i = 0; i < MAX_PLAYER_COUNT; i++)
        if (playerCommunicator[i]->playerStatus == NOT_CONNECTED)
            return i;

    return -1;
}

int *getRandomFreePosition(void) {
    srand((unsigned int) time(NULL));

    int x, y;

    do {
        x = rand() % LABYRINTH_HEIGHT;
        y = rand() % LABYRINTH_WIDTH;
    } while (fieldStatus[x][y] != FREE_BLOCK);

    int *buffArr = malloc(2u * sizeof(int));
    *buffArr = x;
    *(buffArr + 1) = y;

    return buffArr;
}

field_status_t getFieldStatus(int x, int y) {
    return fieldStatus[x][y];
}

field_status_t getStatusFromIndex(int index) {
    if (index == 0) return PLAYER_1;
    if (index == 1) return PLAYER_2;
    if (index == 2) return PLAYER_3;
    return PLAYER_4;
}

int getIndexFromStatus(field_status_t fieldStatus) {
    if (fieldStatus == PLAYER_1 || fieldStatus == PLAYER_1_ON_BUSH)
        return 0;
    if (fieldStatus == PLAYER_2 || fieldStatus == PLAYER_2_ON_BUSH)
        return 1;
    if (fieldStatus == PLAYER_3 || fieldStatus == PLAYER_3_ON_BUSH)
        return 2;
    if (fieldStatus == PLAYER_4 || fieldStatus == PLAYER_4_ON_BUSH)
        return 3;

    return -1;
}

field_status_t getStatusFromIndexBushed(int index) {
    if (index == 0) return PLAYER_1_ON_BUSH;
    if (index == 1) return PLAYER_2_ON_BUSH;
    if (index == 2) return PLAYER_3_ON_BUSH;
    return PLAYER_4_ON_BUSH;
}

int getDroppedTreasureCoins(int x, int y) {
    for (int i = 0; i < droppedTreasureCount; i++)
        if (droppedTreasureStatus[i] == EXIST && droppedTreasure[i].x == x && droppedTreasure[i].y == y)
            return droppedTreasureStatus[i] = NOT_EXISTS, droppedTreasure->coins;

    return TREASURE_COINS;
}

player_move_dir getMoveDirFromInput(int input) {
    if (input == KEY_LEFT)
        return MOVE_LEFT;
    if (input == KEY_RIGHT)
        return MOVE_RIGHT;
    if (input == KEY_UP)
        return MOVE_UP;
    if (input == KEY_DOWN)
        return MOVE_DOWN;

    return 0;
}

void fillSharedMap(int index) {
    int xPos = players->players[index].xPosition;
    int yPos = players->players[index].yPosition;

    int xCorner = xPos - 2;
    int yCorner = yPos - 2;

    if (xCorner < 0)
        xCorner = 0;
    if (yCorner < 0)
        yCorner = 0;


    while(xCorner + 4 > LABYRINTH_HEIGHT - 1)
        xCorner--;

    while(yCorner + 4 > LABYRINTH_WIDTH - 1)
        yCorner--;

    for (int i = 0; i < RANGE_OF_VIEW; i++)
        for (int j = 0; j < RANGE_OF_VIEW; j++)
            playerCommunicator[index]->mapAround.aroundPlayers[i][j] = fieldStatus[xCorner + i][yCorner + j];

}

int getFreeTreasurePos(void) {
    for (int i = 0; i < MAX_DROPPED_TREASURE_COUNT; i++)
        if (droppedTreasureStatus[i] == NOT_EXISTS)
            return i;

    return -1;
}

int isPlayer(field_status_t fieldStatus1) {
    if (fieldStatus1 == PLAYER_1)
        return 1;
    if (fieldStatus1 == PLAYER_2)
        return 1;
    if (fieldStatus1 == PLAYER_3)
        return 1;
    if (fieldStatus1 == PLAYER_4)
        return 1;
    if (fieldStatus1 == PLAYER_1_ON_BUSH)
        return 1;
    if (fieldStatus1 == PLAYER_2_ON_BUSH)
        return 1;
    if (fieldStatus1 == PLAYER_3_ON_BUSH)
        return 1;
    if (fieldStatus1 == PLAYER_4_ON_BUSH)
        return 1;

    return 0;
}
