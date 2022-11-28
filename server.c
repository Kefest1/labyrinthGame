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
#define WALL_CHAR_REPLACED 'O'  // Since c doesn't like █ to be a char
#define BUSH_CHAR '#'

field_status_t fieldStatus[LABYRINTH_HEIGHT][LABYRINTH_WIDTH];

struct communicator_t *playerCommunicator;
player_connector_t *playerSharedConnector;
struct players_t *players; // Not for players processes

WINDOW *win;


// <Statistics here:> //

int xCaptionStartLoc = 1;
int yCaptionStartLoc = LABYRINTH_WIDTH + 1 + 3;

__pid_t serverProcessId;

int campsiteXCoordinate;
int campsiteYCoordinate;

int roundNumber;

// </Statistics here:> //

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
    mvprintw(xCaptionStartLoc + i++, yCaptionStartLoc, "%s", "Type");
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
    readMap();
    pthread_mutex_init(&playerConnectionMutex, NULL);

    players = calloc(1, sizeof(struct players_t));

    int sharedBlockId = shmget(ftok(FILE_CONNECTOR, 0), sizeof(player_connector_t), 0644 | IPC_CREAT);
    playerSharedConnector = (player_connector_t *) shmat(sharedBlockId, NULL, 0);

    playerSharedConnector->totalPlayers = 0;
    playerSharedConnector->playerConnected = 0;
    playerSharedConnector->justConnectedIndex = 0;
    for (int i = 0; i < MAX_PLAYER_COUNT; i++)
        playerSharedConnector->playerStatus[i] = NOT_CONNECTED;

}


_Noreturn void *playerConnector(__attribute__((unused)) void *ptr) {

    while (1) {
        pthread_mutex_lock(&playerConnectionMutex);
        if (playerSharedConnector->playerConnected == 1) {
            playerSharedConnector->playerConnected = 0;
            playerSharedConnector->justConnectedIndex = findFreeIndex();
            int index = playerSharedConnector->justConnectedIndex;

            if (index == -1) {
                puts("Player couldn't connect (game is full)");

                pthread_mutex_unlock(&playerConnectionMutex);
                continue;
            }

            playerSharedConnector->playerStatus[index] = CONNECTED;
            players->players[index].player_id = index;
            players->players[index].deaths = 0;
            players->players[index].coinsCarried = 0;
            players->players[index].coinsBrought = 0;
            players->totalPlayers++;

            int *tempArr = getRandomFreePosition();

            players->players->xPosition = *tempArr;
            players->players->yPosition = *(tempArr + 1);
            paintPlayer(index, *(tempArr + 0), *(tempArr + 1));
            fieldStatus[*tempArr][*(tempArr + 1)] = getStatusFromIndex(index);

            wmove(win, *tempArr, *(tempArr + 1));
            wprintw(win, "%c", (char) (index + '0' + 1));
            refresh();
            wrefresh(win);

            free(tempArr);
            // printf("Player %d has connected\n", index);
            // printf("Total players = %d\n", players->totalPlayers);
        }
        pthread_mutex_unlock(&playerConnectionMutex);
    }

}

void createCommunicator(void) {

    key_t key = ftok(FILE_CONTROLLER, 0);
    int sharedBlockId = shmget(key, sizeof(struct communicator_t),
                               IPC_CREAT);

    playerCommunicator =
            (struct communicator_t *) shmat(sharedBlockId, NULL, 0);

   pthread_mutex_init(&playerControllerMutex, NULL);


//   pthread_mutex_lock(&playerCommunicator->mutex);

//   pthread_mutex_unlock(&playerCommunicator->mutex);

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
    prepareServer();
    displayMap();
    createAndDisplayServerStatistics();
    createCommunicator();
    pthread_t playerListenerThread;
    pthread_create(&playerListenerThread, NULL, playerConnector, NULL);

    // pthread_join(playerListenerThread, NULL);
    sleep(3);
    shmdt(playerSharedConnector);
    free(players);

    endwin();

    return 0;
}

int movePlayer(int index, player_move_dir playerMoveDir) {
    int xFrom = players->players[index].xPosition, xTo;
    int yFrom = players->players[index].yPosition, yTo;
    field_status_t fieldStatusFrom = fieldStatus[xFrom][yFrom];

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

    field_status_t fieldStatusTo = fieldStatus[xTo][yTo];

    if (fieldStatusTo == FREE_BLOCK)
        return 0;
    if (fieldStatusTo == WALL)
        return 1;

    if (fieldStatusTo == LARGE_TREASURE) {
        players->players[index].coinsCarried = LARGE_TREASURE_COINS;
        players->players[index].xPosition = xTo;
        players->players[index].yPosition = yTo;
    }

    if (fieldStatusTo == LARGE_TREASURE) {
        players->players[index].coinsCarried = LARGE_TREASURE_COINS;
        players->players[index].xPosition = xTo;
        players->players[index].yPosition = yTo;
        fieldStatus[xTo][yTo] = getStatusFromIndex(index);
    }

}


void paintPlayer(int index, int x, int y) {
    wmove(win, x, y);
    wprintw(win, "%c", (char) (index + '0' + 1));
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
    int freePos = -1;
    for (int i = 0; i < 4; i++) {
        if (playerSharedConnector->playerStatus[i] == NOT_CONNECTED) {
            freePos = i;
            break;
        }
    }

    return freePos;
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
