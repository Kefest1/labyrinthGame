#include <stdio.h>
#include <pthread.h>

#include "utils.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>


#define LABYRINTH_WIDTH  51
#define LABYRINTH_HEIGHT 25
#define MAP_FILENAME "map.txt"
#define WALL_CHAR '█'
#define WALL_CHAR_REPLACED 'O'  // Since c doesn't like █ to be a char
#define BUSH_CHAR '#'

field_status_t fieldStatus[LABYRINTH_WIDTH][LABYRINTH_HEIGHT];

_Bool isDone = 0;
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
    playerSharedConnector->okToConnect = 1;
    playerSharedConnector->playerConnected = 0;
}

int findFreeIndex(void) {
    int freePos = -1;
    for (int i = 0; i < 4; i++) {
        if (players->playerStatus[i] == NOT_CONNECTED) {
            freePos = i;
            break;
        }
    }

    return freePos;
}

_Noreturn void *playerConnector(void *ptr) {
    playerSharedConnector->okToConnect = 1;

    while (1) {
        pthread_mutex_lock(&playerConnectionMutex);
        if (playerSharedConnector->playerConnected == 1) {
            playerSharedConnector->playerConnected = 0;
            playerSharedConnector->justConnectedIndex = findFreeIndex();
            int index = playerSharedConnector->justConnectedIndex;

            if (index == -1) {
                puts("Player couldn't connect (game is full)");
                playerSharedConnector->okToConnect = 1;
                pthread_mutex_unlock(&playerConnectionMutex);
                continue;
            }

            players->playerStatus[index] = CONNECTED;
            players->players[index].player_id = index;
            players->players[index].deaths = 0;
            players->players[index].coinsCarried = 0;
            players->players[index].coinsBrought = 0;
            players->totalPlayers++;

            playerSharedConnector->totalPlayers++;
            printf("Player %d has connected\n", index);
            printf("Total players = %d\n", players->totalPlayers);
            playerSharedConnector->okToConnect = 1;

            // TODO randomise positions
        }
        pthread_mutex_unlock(&playerConnectionMutex);
    }

}

int main(void) {
    prepareServer();
    puts("Server has started");
    pthread_t playerListenerThread;
    pthread_create(&playerListenerThread, NULL, playerConnector, NULL);

    isDone = 1;

    pthread_join(playerListenerThread, NULL);

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
