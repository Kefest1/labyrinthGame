#include <stdio.h>
#include <pthread.h>
#include <ncurses.h>
#include "utils.h"
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

player_connector_t *playerSharedConnector;
struct players_t *players;

void readMap(void);

void prepareServer(void) {
    readMap();
    pthread_mutex_init(&playerConnectionMutex, NULL);

    players = calloc(1, sizeof(struct players_t));

    int sharedBlockId = shmget(ftok(FILE_MEM_SHARE, 0), sizeof(player_connector_t), 0644 | IPC_CREAT);
    playerSharedConnector = (player_connector_t *) shmat(sharedBlockId, NULL, 0);

    playerSharedConnector->totalPlayers = 0;
    playerSharedConnector->playerConnected = 0;
    playerSharedConnector->justConnectedIndex = 0;
    for (int i = 0; i < MAX_PLAYER_COUNT; i++)
        playerSharedConnector->playerStatus[i] = NOT_CONNECTED;
}

int findFreeIndex(void);

_Noreturn void *playerConnector(void *ptr) {

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


            printf("Player %d has connected\n", index);
            printf("Total players = %d\n", players->totalPlayers);

            // TODO randomise positions
        }
        pthread_mutex_unlock(&playerConnectionMutex);
    }

}

void displayMap(void) {
    initscr();

    WINDOW *win = newwin(LABYRINTH_HEIGHT, LABYRINTH_WIDTH, 1, 1);
    // box(win, 0, 0);
    refresh();

    for (int i = 0; i < LABYRINTH_HEIGHT; i++) {
        for (int j = 0; j < LABYRINTH_WIDTH; j++) {
            field_status_t buffer = fieldStatus[i][j];
            if (buffer == WALL)
                wprintw(win, "%c", WALL_CHAR_REPLACED);
            else if (buffer == LARGE_TREASURE)
                wprintw(win, "%c", 'T');
            else if (buffer == TREASURE)
                wprintw(win, "%c", 't');
            else if (buffer == ONE_COIN)
                wprintw(win, "%c", 'c');
            else if (buffer == BUSHES)
                wprintw(win, "%c", '#');
            else if (buffer == CAMPSITE)
                wprintw(win, "%c", 'A');
            else if (buffer == DROPPED_TREASURE)
                wprintw(win, "%c", 'D');
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
//    puts("\n\nServer has started");
    pthread_t playerListenerThread;
    pthread_create(&playerListenerThread, NULL, playerConnector, NULL);

    // pthread_join(playerListenerThread, NULL);
    sleep(50);
    shmdt(playerSharedConnector);
    free(players);

    endwin();

    return 0;
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
                fieldStatus[i][j] = CAMPSITE;
            if (buffer == 'D')
                fieldStatus[i][j] = DROPPED_TREASURE;
        }

        fgetc(fp);  // "\n"
    }

    fclose(fp);
}
