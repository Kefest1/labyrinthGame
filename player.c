//
// Created by root on 11/6/22.
//
#include <stdio.h>
#include <signal.h>
#include "utils.h"
#include <sys/ipc.h>
#include <sys/shm.h>

void establishConnection(void) {
    int sharedBlockId = shmget(ftok(FILE_MEM_SHARE, 0), sizeof(player_connector_t), 0644 | IPC_CREAT);
    player_connector_t *playerConnector = (player_connector_t *) shmat(sharedBlockId, NULL, 0);

    if (playerConnector->totalPlayers == MAX_PLAYER_COUNT)
        puts("Too many players\nTry again later.");
    else {
        pthread_mutex_lock(&playerConnectionMutex);
        playerConnector->playerConnected = 1;
//        pthread_mutex_unlock(&playerConnectionMutex);
    }
}

int main(void) {

    return 0;
}
