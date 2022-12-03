#include <stdio.h>
#include <pthread.h>
#include <ncurses.h>
#include "communicator.h"
#include "server.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <unistd.h>

#define LABYRINTH_WIDTH  51
#define LABYRINTH_HEIGHT 25
#define MAP_FILENAME "map.txt"
#define WALL_CHAR '█'
#define WALL_CHAR_REPLACED 'O'
#define BUSH_CHAR '#'

#define MINIMAL_PLAYERS_TO_PLAY 1

int debugging_counter = 0;


field_status_t fieldStatus[LABYRINTH_HEIGHT][LABYRINTH_WIDTH];

dropped_treasure_t droppedTreasure[50];
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
    mvprintw(xCaptionStartLoc + i, yCaptionStartLoc, "%s", "\tbrought");
    // TODO legend

    refresh();
}

void prepareServer(void) {
    noecho();
    droppedTreasureCount = 0;
    readMap();

    pthread_mutex_init(&playerConnectionMutex, NULL);

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

    // finalize()


    return NULL;
}

void *playerActionListener(void *ptr) {

    for (int i = 0; i < MAX_PLAYER_COUNT; i++)
        playerCommunicator[i]->currentlyMoving = 0;

    sleep(4u);
    e:

    while (players->totalPlayers >= MINIMAL_PLAYERS_TO_PLAY) {

        for (int i = 0; i < MAX_PLAYER_COUNT; i++) {
            if (playerCommunicator[i]->playerStatus == CONNECTED) {
                playerCommunicator[i]->currentlyMoving = 1;

                sleep(ROUND_DURATION_SECONDS);

                mvprintw(17 + debugging_counter++, 60, "Player %d gave input: %d", i, playerCommunicator[i]->playerInput);

                wrefresh(win);
                refresh();

                player_move_dir moveDir = getMoveDirFromInput(playerCommunicator[i]->playerInput);
                updateRoundNumber();
                if (moveDir == PLAYER_QUIT) {
                    // TODO playerQuit() //
                }
                movePlayer(i, moveDir);
                mvprintw(17 + debugging_counter++, 60, "Player %d Move direction: %d", i, moveDir);

                wrefresh(win);
                refresh();

                playerCommunicator[i]->currentlyMoving = 0;
            }
        }
    }

    goto e;

    return NULL;
}

_Noreturn void *playerConnector(__attribute__((unused)) void *ptr) {

    while (1) {

        if (playerSharedConnector->playerConnected == 1) {
            playerSharedConnector->playerConnected = 0;

            int indexAt = findFreeIndex();
//            printf("Conntected at %d index\n", indexAt);

            if (indexAt == -1) {
                puts("Player couldn't connect (the game is full)");

//                pthread_mutex_unlock(&playerConnectionMutex);
                continue;
            }

            pthread_mutex_lock(playerCommunicator[indexAt]->connectorMutex);

            playerCommunicator[indexAt]->playerStatus = CONNECTED;

            playerSharedConnector->freeIndex = findFreeIndex();
//            mvprintw(16, 60, "%d", playerSharedConnector->freeIndex);
//            refresh();

            int *arr = getRandomFreePosition();

            playerCommunicator[indexAt]->currentlyAtX = arr[0];
            playerCommunicator[indexAt]->currentlyAtY = arr[1];

            players->players[indexAt].xPosition = arr[0];
            players->players[indexAt].yPosition = arr[1];

            players->players[indexAt].xStartPosition = arr[0];
            players->players[indexAt].yStartPosition = arr[1];

            playerCommunicator[indexAt]->coinsPicked = 0;
            playerCommunicator[indexAt]->coinsBrought = 0;
            playerCommunicator[indexAt]->deaths = 0;

            paintPlayer(indexAt, arr[0], arr[1]);

            mvprintw(6, 68 + indexAt * 9, "0");
            // wrefresh(win);
            refresh();

            players->players[indexAt].deaths = 0;
            players->players[indexAt].coinsCarried = 0;
            players->players[indexAt].coinsBrought = 0;
            players->totalPlayers += 1;

            playerSharedConnector->totalPlayerCount++;

            fieldStatus[arr[0]][arr[1]] = getStatusFromIndex(indexAt);
            free(arr);
            // printf("Player %d has connected\n", index);
            // printf("Total players = %d\n", players->totalPlayers);

//            pthread_mutex_unlock(playerCommunicator[indexAt]->connectorMutex);

        }
    }
}

void createConnector(void) {
    key_t key = ftok(FILE_CONNECTOR, 0);

    int sharedBlockId = shmget(key, sizeof(player_connector_t), IPC_CREAT);

    playerSharedConnector =
            (player_connector_t *) shmat(sharedBlockId, NULL, 0);

    playerSharedConnector->pthreadMutex = calloc(1u, sizeof(pthread_mutex_t));
    pthread_mutex_init(playerSharedConnector->pthreadMutex, NULL);
    playerSharedConnector->totalPlayerCount = 0;
    playerSharedConnector->playerConnected = 0;
    playerSharedConnector->freeIndex = 0;
    playerSharedConnector->serverPid = getpid();
}

void createCommunicator(void) {

    for (int i = 0; i < MAX_PLAYER_COUNT; i++) {

        key_t key = ftok(FILE_COMMUNICATOR, i);

        int sharedBlockId =
                shmget(key, sizeof(struct communicator_t), 0644 | IPC_CREAT);

        *(playerCommunicator + i) =
                (struct communicator_t *) shmat(sharedBlockId, NULL, 0);


        (*(playerCommunicator + i))->connectorMutex = malloc(1 * sizeof(pthread_mutex_t));

        int test = pthread_mutex_init((*(playerCommunicator + i))->connectorMutex, NULL);
        if (test) {

        }
        (*(playerCommunicator + i))->playerIndex = i;
        (*(playerCommunicator + i))->playerStatus = NOT_CONNECTED;

        (*(playerCommunicator + i))->mapAround.campsiteX = campsiteXCoordinate;
        (*(playerCommunicator + i))->mapAround.campsiteY = campsiteYCoordinate;

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

void createListener(void) {
//    pthread_t playerListenerThread, inputListenerThread;

}

int main(void) {
    createConnector();
    createCommunicator();

    prepareServer();
    displayMap();

    createAndDisplayServerStatistics();

//    pthread_t playerListenerThread, inputListenerThread;
    pthread_create(&playerListenerThread, NULL, playerConnector, NULL);
    pthread_create(&inputListenerThread, NULL, inputListener, NULL);
    pthread_create(&playerKeyListener, NULL, playerActionListener, NULL);

    // pthread_join(playerListenerThread, NULL);
/*
//    debugging();
    pthread_join(playerListenerThread, NULL);
    shmdt(playerSharedConnector);
    free(players);

    endwin();
*/
    sleep(DEBUG_SLEEP);

    finalize();
    return 0;
}

void finalize(void) {
    free(players);
//    free(playerSharedConnector->pthreadMutex);

    shmdt(playerSharedConnector);

    for (int i = 0; i < MAX_PLAYER_COUNT; i++) {
        free(playerCommunicator[i]->connectorMutex);
        shmdt(playerCommunicator[i]);
    }

    endwin();
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

void playDebugging(void) {

}

int movePlayer(int index, player_move_dir playerMoveDir) {
    //mvwprintw(inputWindow, 2, 1, "Trying to move...");
    int xFrom = players->players[index].xPosition, xTo;
    int yFrom = players->players[index].yPosition, yTo;
    field_status_t fieldStatusFrom = fieldStatus[xFrom][yFrom];

    updateRoundNumber();

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

        if (isOnBushes) {
            fieldStatus[xFrom][yFrom] = BUSHES;
            mvwprintw(win, xFrom, yFrom, "#");
        }
        else {
            fieldStatus[xFrom][yFrom] = FREE_BLOCK;
            mvwprintw(win, xFrom, yFrom, " ");
        }

        playerCommunicator[index]->currentlyAtX = xTo;
        playerCommunicator[index]->currentlyAtY = yTo;

        fieldStatus[xTo][yTo] = getStatusFromIndex(index);

        mvwprintw(win, xTo, yTo, "%c", ('1' + index));
    }
    if (fieldStatusTo == WALL) {
        fillSharedMap(index);
        return 1; // Broadcast communicat
    }
    if (fieldStatusTo == LARGE_TREASURE) {
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

    }
    if (fieldStatusTo == TREASURE) {
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

    }
    if (fieldStatusTo == ONE_COIN) {
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

    }
    if (fieldStatusTo == BUSHES) {
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
    }
    if (fieldStatusTo == CAMPSITE) {
        players->players[index].xPosition = xTo;
        players->players[index].yPosition = yTo;
        players->players[index].coinsBrought += players->players[index].coinsCarried;
        // TODO refresh statistics
        players->players[index].coinsCarried = 0;

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
    }
    if (fieldStatusTo == DROPPED_TREASURE) {
        players->players[index].xPosition = xTo;
        players->players[index].yPosition = yTo;
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
    }
    if (fieldStatusTo == PLAYER_1) {
        collision(index, 0);
    }
    if (fieldStatusTo == PLAYER_2) {
        collision(index, 1);
    }
    if (fieldStatusTo == PLAYER_3) {
        collision(index, 2);
    }
    if (fieldStatusTo == PLAYER_4) {
        collision(index, 3);
    }
    if (fieldStatusTo == WILD_BEAST) {

    }

    players->players[index].xPosition = xTo;
    players->players[index].yPosition = yTo;
    wrefresh(win);
    refresh();

    fillSharedMap(index);

    return 0;
}


//                 p1 -> p2
void collision(int index1, int index2) {
    int posFrom1[2];
    posFrom1[0] = players->players[index1].xPosition;
    posFrom1[1] = players->players[index1].yPosition;

    int posFrom2[2]; // Also place of colision //
    posFrom2[0] = players->players[index2].xPosition;
    posFrom2[1] = players->players[index2].yPosition;

    int isOnBush1 = fieldStatus[posFrom1[0]][posFrom1[1]] == getStatusFromIndexBushed(index1);
    int isOnBush2 = fieldStatus[posFrom1[0]][posFrom1[1]] == getStatusFromIndexBushed(index2);


    int *posTo1;
    int *posTo2;
    do {
        posTo1 = getRandomFreePosition();
        posTo2 = getRandomFreePosition();
    } while (posTo1[0] == posTo2[0] && posTo1[1] == posTo2[1]);


    players->players[index1].xPosition = posTo1[0];
    players->players[index1].yPosition = posTo1[1];

    players->players[index2].xPosition = posTo2[0];
    players->players[index2].yPosition = posTo2[1];

    playerCommunicator[index1]->currentlyAtX = posTo1[0];
    playerCommunicator[index1]->currentlyAtY = posTo1[1];

    playerCommunicator[index2]->currentlyAtX = posTo2[0];
    playerCommunicator[index2]->currentlyAtY = posTo2[1];

    int droppedTreasureVal =
            players->players[index1].coinsCarried + players->players[index2].coinsCarried;

    players->players[index1].coinsCarried = 0;
    players->players[index2].coinsCarried = 0;

    playerCommunicator[index1]->coinsPicked = 0;
    playerCommunicator[index2]->coinsPicked = 0;

    if (isOnBush1) {
        fieldStatus[posFrom1[0]][posFrom1[1]] = BUSHES;
        mvwprintw(win, posFrom1[0], posFrom1[1], "#");
    }
    else {
        fieldStatus[posFrom1[0]][posFrom1[1]] = FREE_BLOCK;
        mvwprintw(win, posFrom1[0], posFrom1[1], " ");
    }

    if (isOnBush2) {
        fieldStatus[posFrom2[0]][posFrom2[1]] = BUSHES;
        mvwprintw(win, posFrom2[0], posFrom2[1], "#");
    }
    else {
        fieldStatus[posFrom2[0]][posFrom2[1]] = FREE_BLOCK;
        mvwprintw(win, posFrom2[0], posFrom2[1], " ");
    }

    fieldStatus[posFrom2[0]][posFrom2[1]] = DROPPED_TREASURE;


    wrefresh(win);
    refresh();

    free(posTo1);
    free(posTo2);
}

void paintPlayer(int index, int x, int y) {
    mvwprintw(win, x, y, "%c", (char) (index + '0' + 1));
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

    int *buffArr = malloc(2 * sizeof(int));
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
        if (droppedTreasure[i].x == x && droppedTreasure[i].y == y)
            return droppedTreasure->coins;

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
    if (input == 'q' || input == 'Q')
        return PLAYER_QUIT;

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
