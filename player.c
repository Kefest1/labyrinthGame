//
// Created by root on 11/6/22.
//
#include <stdio.h>
#include <signal.h>
#include "utils.h"
#include <sys/ipc.h>
#include <sys/shm.h>

void establishConnection(void) {
    int sharedBlockId = shmget(ftok(FILE_MEM_SHARE, 0), sizeof(struct players_t), 0644 | IPC_CREAT);
    struct players_t *players = (struct players_t *) shmat(sharedBlockId, NULL, 0);

    if (players->playerNumber == MAX_PLAYER_COUNT)
        puts("Too many players\nTry again later.");
    else {
        for (int i = 0; i < 4; i++) {
            if (players->playerStatus[i] == NOT_CONNECTED) {
                pthread_mutex_lock(&playerConnectionMutex);
                players->players[i].coinsBrought = 0;
                players->players[i].coinsCarried = 0;
                players->players[i].deaths = 0;
                players->players[i].player_id = i;
                players->playerConnected = 1;
                players->justConnectedIndex = i;
                pthread_mutex_unlock(&playerConnectionMutex);
            }
        }
    }
}

int main(void) {

    return 0;
}
