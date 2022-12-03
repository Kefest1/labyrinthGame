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


void createStatistics(void) {

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

    printf("Currently %d players\n", playerConnector->totalPlayerCount + 1);

//    pthread_mutex_unlock(&playerConnectionMutex);

    return 0;
}

#define LABYRINTH_WIDTH  51
#define LABYRINTH_HEIGHT 25

int connectToCommunicator(int playerConnectionIndex) {
    key_t key = ftok(FILE_COMMUNICATOR, playerConnectionIndex);

    mvprintw(3, 3, "PlayerID: %d", playerConnectionIndex);
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

void createMessageWindow(void) {
    messagesWindow = newwin(16, 36, 15, 60);
    refresh();
    box(messagesWindow, 0, 0);
    wrefresh(messagesWindow);
}

int debug = 1;

void *gameMove(void *ptr) {
    e:

    while (playerCommunicator->currentlyMoving == 0);

    mvwprintw(messagesWindow, debug++, 1, "Give input");
    wrefresh(messagesWindow);
    refresh();

    //sleep(ROUND_DURATION_SECONDS);
    int input = getRandomInputDebug();  //KEY_LEFT; // getch();

    mvwprintw(messagesWindow, debug++, 1, "Your input: %d", input);
    wrefresh(messagesWindow);
    refresh();

    playerCommunicator->playerInput = input;
    while (1) {
//        printf("%d ", playerCommunicator->currentlyMoving);

        if (playerCommunicator->currentlyMoving == 0)
            break;
    }


    mvwprintw(messagesWindow, debug++, 2, "Wait for your turn");
    wrefresh(messagesWindow);
    refresh();

    goto e;

    return NULL;
}

int getRandomInputDebug(void) {
    srand(time(NULL));
    int x = rand() % 4;

    if (x == 0)
        return KEY_UP;
    if (x == 1)
        return KEY_DOWN;
    if (x == 2)
        return KEY_LEFT;
    return KEY_RIGHT;
}

int main(void) {
    initscr();
    refresh();

    // noecho();
    createMessageWindow();

    if (establishConnection() == -1)
        return puts("Failed to connect.\nServer is yet to start"), 1;

    pthread_create(&keyListenerThread, NULL, &gameMove, NULL);

    sleep(16u);
//    pthread_join(keyListenerThread, NULL);
//    setenv("TERMINFO","/usr/share/terminfo", 1);


    endwin();
    return 0;
}
