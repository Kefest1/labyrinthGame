#include <stdio.h>
#include <signal.h>
#include "utils.h"
#include <sys/ipc.h>
#include <sys/shm.h>

int playerID;

void establishConnection(void) {
    int sharedBlockId = shmget(ftok(FILE_MEM_SHARE, 0), sizeof(player_connector_t), 0644 | IPC_CREAT);
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

}

int main(void) {
    establishConnection();
    getchar();
    return 0;
}
