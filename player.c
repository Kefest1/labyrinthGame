#include <stdio.h>
#include <signal.h>
#include "utils.h"
#include <sys/ipc.h>
#include <sys/shm.h>

int playerID;

int establishConnection(void) {
    int sharedBlockId = shmget(ftok(FILE_MEM_SHARE, 0),
                               sizeof(player_connector_t),
                               IPC_EXCL);

    if (sharedBlockId < 0) return -1;

    player_connector_t *playerConnector = (player_connector_t *) shmat(sharedBlockId, NULL, 0);

    pthread_mutex_lock(&playerConnectionMutex);

    if (playerConnector->totalPlayers == MAX_PLAYER_COUNT) {
        puts("Too many players\nTry again later.");
    }
    else {
        puts("Connected successfully");
        for (int i = 0; i < MAX_PLAYER_COUNT; i++)
            if (playerConnector->playerStatus[i] == NOT_CONNECTED)
                playerID = i;
        playerConnector->totalPlayers++;
    }
    playerConnector->playerConnected = 1;

    printf("Currently %d players\n", playerConnector->totalPlayers);

    pthread_mutex_unlock(&playerConnectionMutex);

    return 0;
}

int main(void) {
    if (establishConnection() == -1)
        return puts("Failed to connect."), 1;

    getchar();
    return 0;
}
