#include <stdio.h>

#include "communicator.h"
#include "player.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <stdlib.h>
#include <ncurses.h>
#include <unistd.h>

int getRandomInputDebug(void);

// TODO FAIL IF DOESN'T EXIST


player_connector_t *playerConnector;
struct communicator_t *playerCommunicator;

pthread_t keyListenerThread;

WINDOW *messagesWindow;
WINDOW *map;
#define LABYRINTH_WIDTH  51

// <Statistics> //
int xCaptionStartLoc = 1;
int yCaptionStartLoc = LABYRINTH_WIDTH + 1 + 3;

__pid_t serverProcessId;
int playerID;

int roundNumber;

// </Statistics //

int getCharFromStatus(field_status_t fieldStatus) {
    if (fieldStatus == WALL)
        return 'O';
    if (fieldStatus == FREE_BLOCK)
        return ' ';
    if (fieldStatus == LARGE_TREASURE)
        return 'T';
    if (fieldStatus == TREASURE)
        return 't';
    if (fieldStatus == ONE_COIN)
        return 'c';
    if (fieldStatus == BUSHES)
        return '#';
    if (fieldStatus == CAMPSITE)
        return 'A';
    if (fieldStatus == DROPPED_TREASURE)
        return 'D';
    if (fieldStatus == WILD_BEAST)
        return '*';
    if (fieldStatus == PLAYER_1 || fieldStatus == PLAYER_1_ON_BUSH)
        return '1';
    if (fieldStatus == PLAYER_2 || fieldStatus == PLAYER_2_ON_BUSH)
        return '2';
    if (fieldStatus == PLAYER_3 || fieldStatus == PLAYER_3_ON_BUSH)
        return '3';
    if (fieldStatus == PLAYER_4 || fieldStatus == PLAYER_4_ON_BUSH)
        return '4';

    return '?';
}

int checkIfConnectorExist(void) {
    errno = 0;

    int sharedBlockId = shmget(ftok(FILE_CONNECTOR, 0), sizeof(player_connector_t),
                               IPC_CREAT | IPC_EXCL);

    if (sharedBlockId == -1)
        if (errno == EEXIST)
            return 1;

    return 0;
}


void createAndDisplayStatistics(void) {
    roundNumber = 0;

    int i = 0;
    mvprintw(xCaptionStartLoc + i++, yCaptionStartLoc, "Server's PID: %d", serverProcessId);
    i++;  // mvprintw(xCaptionStartLoc + i++, yCaptionStartLoc, "Campsite X/Y: %d/%d", campsiteXCoordinate, campsiteYCoordinate);
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


int establishConnection(void) {
//    if (checkIfConnectorExist())
//        return -1;

    key_t key = ftok(FILE_CONNECTOR, 0);
    int sharedBlockId = shmget(key, sizeof(player_connector_t),
                               IPC_CREAT);

    playerConnector =
            (player_connector_t *) shmat(sharedBlockId, NULL, 0);

//    pthread_mutex_lock(&playerConnectionMutex);
    serverProcessId = playerConnector->serverPid;
    if (playerConnector->totalPlayerCount == MAX_PLAYER_COUNT) {
        puts("Too many players\nTry again later.");
    }
    else {
//        puts("Connected successfully");
        playerID = playerConnector->freeIndex;

        refresh();
    }
    playerConnector->playerConnected = 1;

    // Already locked! //
    connectToCommunicator(playerID);

//    printf("Currently %d players\n", playerConnector->totalPlayerCount + 1);

//    pthread_mutex_unlock(&playerConnectionMutex);

    return 0;
}

#define LABYRINTH_WIDTH  51
#define LABYRINTH_HEIGHT 25

int mapCleanerX = -1;
int mapCleanerY;

void printMapAround(void) {
    if (mapCleanerX != -1) {
        for (int i = 0; i < 5; i++) {
            for (int j = 0; j < 5; j++) {
                mvwprintw(map, mapCleanerX, mapCleanerY, " ");
            }
        }
    }

    mapCleanerX = playerCommunicator->currentlyAtX - 2;
    mapCleanerY = playerCommunicator->currentlyAtY - 2;


    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            mvwprintw(
                    map, playerCommunicator->currentlyAtX - 2 + i, playerCommunicator->currentlyAtY - 2 + j,
                    "%c", getCharFromStatus(playerCommunicator->mapAround.aroundPlayers[i][j])
                      );
        }
    }

    wrefresh(map);
    refresh();
}

int connectToCommunicator(int playerConnectionIndex) {
    key_t key = ftok(FILE_COMMUNICATOR, playerConnectionIndex);

    // mvprintw(3, 3, "PlayerID: %d", playerConnectionIndex);
    refresh();

    int sharedBlockId = shmget(key, sizeof(struct communicator_t),
                               IPC_CREAT);

    playerCommunicator =
            (struct communicator_t *) shmat(sharedBlockId, NULL, 0);

    return 0;
}

void *getInputThread(void *ptr) {
    return NULL;
}

void createMapWindow(void) {
    map = newwin(LABYRINTH_HEIGHT, LABYRINTH_WIDTH, 0, 0);
    refresh();
    wrefresh(messagesWindow);
}

void createMessageWindow(void) {
    messagesWindow = newwin(16, 36, 15, 60);
    refresh();
    box(messagesWindow, 0, 0);
    wrefresh(messagesWindow);
}

int debug = 1;

int getDirection(int input) {
    if (input == 65)
        return KEY_UP;
    if (input == 66)
        return KEY_DOWN;
    if (input == 67)
        return KEY_RIGHT;

    return KEY_LEFT;
}

void *gameMove(void *ptr) {
    e:

    while (playerCommunicator->currentlyMoving == 0);

    mvwprintw(messagesWindow, debug++, 1, "Give input");
    wrefresh(messagesWindow);
    refresh();


    // UP DOWN LEFT RIGHT
    int isq = getch();
    if (isq == 'Q' || isq == 'q'){
        // TODO finalize(); //
    }
    getch();
    int input = getch(); // getch();//getRandomInputDebug();

    input = getDirection(input);

    mvwprintw(messagesWindow, debug++, 1, "Your input: %d", input);
    wrefresh(messagesWindow);
    refresh();

    playerCommunicator->playerInput = input;
    while (1) {
//        printf("%d ", playerCommunicator->currentlyMoving);

        if (playerCommunicator->currentlyMoving == 0)
            break;
    }
    printMapAround();

    mvwprintw(messagesWindow, debug++, 2, "Wait for your turn");
    wrefresh(messagesWindow);
    refresh();

    goto e;

    return NULL;
}

int getRandomInputDebug(void) {
//    srand(time(NULL));

    int dir;
    dir = rand() % 4;

    if (dir == 0)
        return KEY_UP;
    if (dir == 1)
        return KEY_DOWN;
    if (dir == 2)
        return KEY_LEFT;

    return KEY_RIGHT;
}

int main(void) {
    srand(time(NULL));
    initscr();
    refresh();

    // noecho();
    createMessageWindow();
    createMapWindow();

    if (establishConnection() == -1)
        return puts("Failed to connect.\nServer is yet to start"), 1;
    createAndDisplayStatistics();

    pthread_create(&keyListenerThread, NULL, &gameMove, NULL);

    sleep(120u);
//    pthread_join(keyListenerThread, NULL);
//    setenv("TERMINFO","/usr/share/terminfo", 1);


    endwin();
    return 0;
}
