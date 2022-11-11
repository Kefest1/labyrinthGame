#include <stdio.h>
#include <pthread.h>

#include "utils.h"
#include <sys/ipc.h>
#include <sys/shm.h>



#define LABYRINTH_WIDTH  51
#define LABYRINTH_HEIGHT 25
// #define MAP_FILENAME "/root/CLionProjects/labyrinthGame/utils/map.txt"
#define MAP_FILENAME "map.txt"
#define WALL_CHAR '█'
#define WALL_CHAR_REPLACED 'O'  // Since c doesn't like █ to be a char
#define BUSH_CHAR '#'

typedef enum {
    WALL,
    FREE_BLOCK,
    LARGE_TREASURE,
    TREASURE,
    ONE_COIN,
    BUSHES,
    CAMPSITE,
    DROPPED_TREASURE
} field_status_t;

field_status_t fieldStatus[LABYRINTH_WIDTH][LABYRINTH_HEIGHT];
struct players_t *players;

void readMap(void);

void prepareServer(void) {
    int sharedBlockId = shmget(ftok(FILE_MEM_SHARE, 0), sizeof(struct players_t), 0644 | IPC_CREAT);
    players = (struct players_t *) shmat(sharedBlockId, NULL, 0);

    pthread_mutex_init(&playerConnectionMutex, NULL);
    readMap();

}

void *playerConnector(void *ptr) {

    while (1) {
        pthread_mutex_lock(&playerConnectionMutex);
        if (players->playerConnected == 1) {
            players->playerConnected = 0;

        }
        pthread_mutex_unlock(&playerConnectionMutex);

    }

    return NULL;
}

int main(void) {
    pthread_t playerListener;
    pthread_create(&playerListener, NULL, playerConnector, NULL);



    return 0;
}



void readMap(void) {
    FILE *fp = fopen(MAP_FILENAME, "r");

    int buffer;

    for (int i = 0; i < LABYRINTH_HEIGHT; i++) {
        for (int j = 0; j < LABYRINTH_WIDTH; j++) {
            buffer = fgetc(fp);
            if (buffer == WALL_CHAR_REPLACED)
                fieldStatus[j][i] = WALL;
            if (buffer == ' ')
                fieldStatus[j][i] = FREE_BLOCK;
            if (buffer == BUSH_CHAR)
                fieldStatus[j][i] = BUSHES;
            if (buffer == 'c')
                fieldStatus[j][i] = ONE_COIN;
            if (buffer == 't')
                fieldStatus[j][i] = TREASURE;
            if (buffer == 'T')
                fieldStatus[j][i] = LARGE_TREASURE;
            if (buffer == 'A')
                fieldStatus[j][i] = CAMPSITE;
            if (buffer == 'D')
                fieldStatus[j][i] = DROPPED_TREASURE;
        }

        fgetc(fp);  // "\n"
    }

    fclose(fp);
}
