#include <stdio.h>

#include "communicator.h"
#include "player.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <stdlib.h>
#include <ncurses.h>
#include <unistd.h>
#include <signal.h>

#define QUIT_EXIT_CODE 1

#define TO_MANY_PLAYERS_ERROR -1
#define SERVER_NOT_STARTED -2

int getRandomInputDebug(void);
void printMapAround(void);
// TODO FAIL IF DOESN'T EXIST

int input1;
int input3;
int inputReadSuccess;

player_connector_t *playerConnector;
struct communicator_t *playerCommunicator;

pthread_t keyListenerThread;
pthread_t roundUpdaterThread;
pthread_t serverListenerThread;

WINDOW *messagesWindow;
WINDOW *map;

#define LABYRINTH_WIDTH  51

#define LABYRINTH_HEIGHT 25

int mapCleanerX = -1;
int mapCleanerY;

// <Statistics> //
int xCaptionStartLoc = 1;
int yCaptionStartLoc = LABYRINTH_WIDTH + 1 + 3;

__pid_t serverProcessId;
int playerID;

int roundNumber = 0;

// </Statistics //

void displayPlayerNumber(void) {
    mvprintw(xCaptionStartLoc + 5, yCaptionStartLoc + 7, "%d  ", playerID);
    refresh();
}

void updatePlayerCur(void) {
    mvprintw(xCaptionStartLoc + 6, yCaptionStartLoc + 9, "%2d/%2d",
             playerCommunicator->currentlyAtX, playerCommunicator->currentlyAtY);
    refresh();
}

void updatePlayerDeaths(void) {
    mvprintw(xCaptionStartLoc + 7, yCaptionStartLoc + 7, "%d   ", playerCommunicator->deaths);
    refresh();
}

void updatePlayerCarriedCoins(void) {
    mvprintw(xCaptionStartLoc + 10, yCaptionStartLoc + 10, "%d   ", playerCommunicator->coinsPicked);
    refresh();
}

void updatePlayerBroughtCoins(void) {
    mvprintw(xCaptionStartLoc + 11, yCaptionStartLoc + 10, "%d   ", playerCommunicator->coinsBrought);
    refresh();
}

void updateStatistics(void) {
    updatePlayerDeaths();
    updatePlayerCur();
    updatePlayerBroughtCoins();
    updatePlayerCarriedCoins();
}

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

void *updateRound(void *ptr) {
    inf_loop:

    sem_wait(&playerCommunicator->updateSemaphore);

    if (playerConnector->rounds != roundNumber) {
        mvprintw(xCaptionStartLoc + 2, yCaptionStartLoc, "Round number: %d", playerConnector->rounds);
        roundNumber = playerConnector->rounds;

        printMapAround();

        if (roundNumber)
            updateStatistics();
    }

    refresh();

//

    goto inf_loop;
}

void createAndDisplayStatistics(void) {
    roundNumber = 0;

    int i = 0;
    mvprintw(xCaptionStartLoc + i++, yCaptionStartLoc, "Server's PID: %d", serverProcessId);
    i++;  // mvprintw(xCaptionStartLoc + i++, yCaptionStartLoc, "Campsite X/Y: %d/%d", campsiteXCoordinate, campsiteYCoordinate);
    mvprintw(xCaptionStartLoc + i++, yCaptionStartLoc, "Round number: %d", roundNumber);
    i++;
    mvprintw(xCaptionStartLoc + i, yCaptionStartLoc, "%s", "Player:");
    i++;
    mvprintw(xCaptionStartLoc + i++, yCaptionStartLoc, "%s", "Number");
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

int establishConnection(void) {
    errno = 0;

    key_t key = ftok(FILE_CONNECTOR, 0);
    int sharedBlockId = shmget(key, sizeof(player_connector_t), 0);

    if (errno == ENOENT || sharedBlockId < 0) {
        return SERVER_NOT_STARTED;
    }

    playerConnector =
            (player_connector_t *) shmat(sharedBlockId, NULL, 0);


    pthread_create(&roundUpdaterThread, NULL, &updateRound, NULL);
    serverProcessId = playerConnector->serverPid;


    if (playerConnector->totalPlayerCount == MAX_PLAYER_COUNT) {
        return TO_MANY_PLAYERS_ERROR;
    }
    else {
        playerID = playerConnector->freeIndex;
        refresh();
    }

    sem_post(&playerConnector->connectorSemaphore1);


    // Already locked! //
    connectToCommunicator(playerID);

    if (playerConnector->totalPlayerCount == 1)
        sem_post(&playerConnector->isGameRunnningSemaphore);

    return 0;
}

void printMapAround(void) {

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


    int sharedBlockId = shmget(key, sizeof(struct communicator_t),
                               IPC_CREAT);

    playerCommunicator =
            (struct communicator_t *) shmat(sharedBlockId, NULL, 0);


    playerCommunicator->playerProcessID = getpid();


    return 0;
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

int isPrintable = 0;
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

void *serverListener(void *ptr) {
    sem_wait(&playerConnector->isServerUpSemaphore);

    finalize();

    pthread_kill(keyListenerThread, SIGKILL);
    return NULL;
}

void *gameMove(void *ptr) {
    pthread_create(&serverListenerThread, NULL, &serverListener, NULL);

    halfdelay(ROUND_DURATION_TENTH_SECONDS - 2u);
    inf_loop:

    sem_wait(&playerCommunicator->communicatorSemaphore1);

    if (!isPrintable)
        printMapAround(), isPrintable = 1;

    mvwprintw(messagesWindow, debug, 1, "Give input        ");
    wrefresh(messagesWindow);
    refresh();

    int isq = getch();
    cbreak();
    if (isq == 'Q' || isq == 'q') {
        mapCleanerX = -1;
        playerCommunicator->hasJustDisconnected = 1;
        finalize();
        return NULL;
    }

    getch();
    int input = getch();

    input = getDirection(input);

    playerCommunicator->playerInput = input;


    mvwprintw(messagesWindow, debug, 1, "Your input: %d   ", input);
    wrefresh(messagesWindow);
    refresh();


    sem_wait(&playerCommunicator->communicatorSemaphore2);

    printMapAround();

    mvwprintw(messagesWindow, debug, 1, "Wait for your turn");
    wrefresh(messagesWindow);
    refresh();

    goto inf_loop;

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

void setGameUp(void) {
    initscr();
    refresh();
    noecho();
    refresh();
}

void finalize(void) {
    werase(map);
    werase(messagesWindow);
    endwin();
}

int main(void) {

    int x = establishConnection();
    if (x == TO_MANY_PLAYERS_ERROR) {
        werase(messagesWindow), werase(map), endwin();
        puts("To many players");
        return TO_MANY_PLAYERS_ERROR * -1;
    }
    if (x == SERVER_NOT_STARTED) {
        werase(messagesWindow), werase(map), endwin();
        puts("Server has not started yet");
        return SERVER_NOT_STARTED * -1;
    }

    setGameUp();
    createMessageWindow();
    createMapWindow();

    createAndDisplayStatistics();

    pthread_create(&keyListenerThread, NULL, &gameMove, NULL);

    pthread_join(keyListenerThread, NULL);

    finalize();
    return 0;
}
