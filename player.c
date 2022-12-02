#include <stdio.h>

#include "communicator.h"
#include "player.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <stdlib.h>
#include <ncurses.h>
#include <unistd.h>


// TODO FAIL IF DOESN'T EXIST

int playerID;
int playerProcessID;
/*
typedef enum {
//    WALL,
//    FREE_BLOCK,
//    LARGE_TREASURE,
//    TREASURE,
//    ONE_COIN,
//    BUSHES,
//    CAMPSITE,
//    DROPPED_TREASURE,
    WILD_BEAST,
    PLAYER_1,
    PLAYER_2,
    PLAYER_3,
    PLAYER_4,
    PLAYER_1_ON_BUSH,
    PLAYER_2_ON_BUSH,
    PLAYER_3_ON_BUSH,
    PLAYER_4_ON_BUSH
} field_status_t;*/
player_connector_t *playerConnector;
struct communicator_t *playerCommunicator;

pthread_t keyListenerThread;

WINDOW *messagesWindow;
WINDOW *map;

int checkIfConnectorExist(void) {
    errno = 0;

    int sharedBlockId = shmget(ftok(FILE_CONNECTOR, 0), sizeof(player_connector_t),
                               IPC_CREAT | IPC_EXCL);

    if (sharedBlockId == -1)
        if (errno == EEXIST)
            return 1;

    return 0;
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
    int input = getch(); //KEY_LEFT; // getch();
//    if (input == 'w')
//        input = KEY_UP;
//    if (input == 'a')
//        input = KEY_LEFT;
//    if (input == 's')
//        input = KEY_DOWN;
//    if (input == 'd')
//        input = KEY_RIGHT;

    mvwprintw(messagesWindow, debug++, 1, "Your input: %d", input);
    wrefresh(messagesWindow);
    refresh();

    playerCommunicator->playerInput = input;
    while (1) {
//        printf("%d ", playerCommunicator->currentlyMoving);

        if (playerCommunicator->currentlyMoving == 0)
            break;
    }

    printf("Dupadupa");

    mvwprintw(messagesWindow, debug++, 2, "Wait for your turn");
    wrefresh(messagesWindow);
    refresh();

    goto e;

    return NULL;
}

int main(void) {
    initscr();
    refresh();

    // noecho();
    createMessageWindow();

    if (establishConnection() == -1)
        return puts("Failed to connect.\nServer is yet to start"), 1;

    pthread_create(&keyListenerThread, NULL, &gameMove, NULL);

    sleep(12);
//    pthread_join(keyListenerThread, NULL);
//    setenv("TERMINFO","/usr/share/terminfo", 1);

    endwin();
    return 0;
}
