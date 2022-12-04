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
    mvprintw(6, 68 + index * 9, "    ");
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

        wrefresh(win);
        refresh();

        free(arr);
    } while (inputChar != 'q' && inputChar != 'Q');

    finalize();


    return NULL;
}

void erasePlayer(int index) {
    int x, y;
    x = players->players[index].xPosition;
    y = players->players[index].yPosition;
    // printf("%d %d", x, y);
    mvwprintw(win, x, y, " ");
    wrefresh(win);
    refresh();
}

void *playerActionListener(void *ptr) {

    for (int i = 0; i < MAX_PLAYER_COUNT; i++)
        playerCommunicator[i]->currentlyMoving = 0;

    sleep(4u);
    inf_loop:

    while (players->totalPlayers >= MINIMAL_PLAYERS_TO_PLAY) {

        for (int i = 0; i < MAX_PLAYER_COUNT; i++) {
            if (playerCommunicator[i]->playerStatus == CONNECTED) {
//                pthread_mutex_lock(&playerCommunicator[i]->connectorMutex);

                playerCommunicator[i]->currentlyMoving = 1;

//                pthread_mutex_unlock(&playerCommunicator[i]->connectorMutex);

                sleep(ROUND_DURATION_SECONDS);

//                pthread_mutex_lock(&playerCommunicator[i]->connectorMutex);

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
//                    pthread_mutex_unlock(&playerCommunicator[i]->connectorMutex);

                    continue;
                }

                mvprintw(17 + debugging_counter++, 60, "Player %d gave input: %d", i + 1,
                         playerCommunicator[i]->playerInput);

                wrefresh(win);
                refresh();

                player_move_dir moveDir = getMoveDirFromInput(playerCommunicator[i]->playerInput);

//                pthread_mutex_unlock(&playerCommunicator[i]->connectorMutex);

                movePlayer(i, moveDir);
                mvprintw(17 + debugging_counter++, 60, "Player %d Move direction: %d", i + 1, moveDir);
                debugging_counter++;
                wrefresh(win);
                refresh();
//                pthread_mutex_lock(&playerCommunicator[i]->connectorMutex);

                playerCommunicator[i]->currentlyMoving = 0;

//                pthread_mutex_unlock(&playerCommunicator[i]->connectorMutex);
            }

            sleep(BETWEEN_ROUNDS_SLEEP);

        }
    }

    goto inf_loop;

    return NULL;
}

_Noreturn void *playerConnector(__attribute__((unused)) void *ptr) {

    while (1) {
        pthread_mutex_lock(&playerSharedConnector->pthreadMutex);

        if (playerSharedConnector->playerConnected == 1) {
            playerSharedConnector->playerConnected = 0;

            int indexAt = findFreeIndex();

            if (indexAt == -1) {
                puts("Player couldn't connect (the game is full)");
                continue;
            }

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

            playerSharedConnector->totalPlayerCount++;

            free(arr);
        }

        pthread_mutex_unlock(&playerSharedConnector->pthreadMutex);
    }
}

void createConnector(void) {
    key_t key = ftok(FILE_CONNECTOR, 0);

    int sharedBlockId = shmget(key, sizeof(player_connector_t), IPC_CREAT);

    playerSharedConnector =
            (player_connector_t *) shmat(sharedBlockId, NULL, 0);

    pthread_mutexattr_t mutexattr;
    pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_setpshared(&mutexattr, PTHREAD_PROCESS_SHARED);

    pthread_mutex_init(&playerSharedConnector->pthreadMutex, &mutexattr);

    pthread_mutexattr_destroy(&mutexattr);

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


        pthread_mutexattr_t mutexattr;
        pthread_mutexattr_init(&mutexattr);
        pthread_mutexattr_setpshared(&mutexattr, PTHREAD_PROCESS_SHARED);

        pthread_mutex_init(&(playerCommunicator[i]->connectorMutex), &mutexattr);
        pthread_mutexattr_destroy(&mutexattr);

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


int main(void) {
    createConnector();
    createCommunicator();

    prepareServer();
    displayMap();

    createAndDisplayServerStatistics();

    pthread_create(&playerListenerThread, NULL, playerConnector, NULL);
    pthread_create(&inputListenerThread, NULL, inputListener, NULL);
    pthread_create(&playerKeyListener, NULL, playerActionListener, NULL);

    sleep(DEBUG_SLEEP);

    endwin();
    finalize();
    return 0;
}

void finalize(void) {
    free(players);

    pthread_mutex_destroy(&playerSharedConnector->pthreadMutex);
    int conectorIndex = getSharedBlock(FILE_CONNECTOR, sizeof(player_connector_t), 0);
    shmctl(conectorIndex, IPC_RMID, NULL);

    for (int i = 0; i < MAX_PLAYER_COUNT; i++) {
        pthread_mutex_destroy(&(playerCommunicator[i]->connectorMutex));
        int controllerIndex = getSharedBlock(FILE_COMMUNICATOR, sizeof(struct communicator_t), i);
        shmctl(controllerIndex, IPC_RMID, NULL);
    }

    endwin();
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

    updateRoundNumber();

    pthread_mutex_lock(&playerSharedConnector->pthreadMutex);

    playerSharedConnector->rounds++;

    pthread_mutex_unlock(&playerSharedConnector->pthreadMutex);

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
        mvwprintw(win, 0, 0, "Dupa");
        if (isOnCampsite) {
            fieldStatus[xFrom][yFrom] = CAMPSITE;
            mvwprintw(win, xFrom, yFrom, "A");
            wrefresh(win);
            refresh();
        }

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

        playerCommunicator[index]->currentlyAtX = xTo;
        playerCommunicator[index]->currentlyAtY = yTo;

        players->players[index].xPosition = xTo;
        players->players[index].yPosition = yTo;

        fieldStatus[xTo][yTo] = getStatusFromIndex(index);

        mvwprintw(win, xTo, yTo, "%c", '1' + index);

        updatePlayerPosition(index);
        fillSharedMap(index);

        return 0;
    }
    if (fieldStatusTo == WALL) {
        fillSharedMap(index);
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
        }
        else {
            fieldStatus[xFrom][yFrom] = FREE_BLOCK;
            mvwprintw(win, xFrom, yFrom, " ");
        }

        fieldStatus[xTo][yTo] = getStatusFromIndex(index);

        playerCommunicator[index]->currentlyAtX = xTo;
        playerCommunicator[index]->currentlyAtY = yTo;
        playerCommunicator[index]->coinsBrought += playerCommunicator[index]->coinsPicked;
        playerCommunicator[index]->coinsPicked = 0;

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

        return 0;
    }
    if (fieldStatusTo == WILD_BEAST) {
        // TODO Wild beast
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
    mvwprintw(win, 1, 1, "Collision has occured");
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
