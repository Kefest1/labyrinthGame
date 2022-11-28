#include <stdio.h>

#include "communicator.h"
#include "player.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
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

    pthread_mutex_lock(&playerConnectionMutex);

    if (playerConnector->totalPlayers == MAX_PLAYER_COUNT) {
        puts("Too many players\nTry again later.");
    }
    else {
        puts("Connected successfully");
        for (int i = 0; i < MAX_PLAYER_COUNT; i++) {
            if (playerConnector->playerStatus[i] == NOT_CONNECTED) {
                playerID = i;
                break;
            }
        }
        playerConnector->totalPlayers++;
    }
    playerConnector->playerConnected = 1;

    // Already locked! //
    //connectToCommunicator(playerID);

    printf("Currently %d players\n", playerConnector->totalPlayers);

    pthread_mutex_unlock(&playerConnectionMutex);

    return 0;
}

int connectToCommunicator(int playerConnectionIndex) {
    key_t key = ftok(FILE_CONTROLLER, 0);
    int sharedBlockId = shmget(key, sizeof(struct communicator_t),
                               IPC_CREAT);

    playerCommunicator =
            (struct communicator_t *) shmat(sharedBlockId, NULL, 0);

    pthread_mutex_lock(&playerConnectionMutex);

    playerCommunicator->isConnected[playerConnectionIndex] = 1;

    pthread_mutex_unlock(&playerConnectionMutex);

    return 0;
}

void *gameMove(void *ptr) {
    // print "your turn"
    char input;



}

int main(void) {
    int x = getchar();
    printf("\"%c\"\n", x);
//    if (establishConnection() == -1)
//        return puts("Failed to connect.\nServer is yet to start"), 1;

    return 0;
}
