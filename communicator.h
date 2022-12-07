//
// Created by root on 11/24/22.
//

#ifndef LABYRINTHGAME_COMMUNICATOR_H
#define LABYRINTHGAME_COMMUNICATOR_H

#include "utils.h"
#include <semaphore.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define MAX_PLAYERS MAX_PLAYER_COUNT

#define RANGE_OF_VIEW 5
#define ROUND_DURATION_SECONDS 4u
#define ROUND_DURATION_TENTH_SECONDS (10u * ROUND_DURATION_SECONDS)

#define BETWEEN_ROUNDS_SLEEP 1u
#define DEBUG_SLEEP 120u

typedef struct {
    field_status_t aroundPlayers[RANGE_OF_VIEW][RANGE_OF_VIEW];
    int campsiteX;
    int campsiteY;
} map_around_t;


// Shared between one player and server //

struct communicator_t {
    sem_t communicatorSemaphore1;
    sem_t communicatorSemaphore2;
    sem_t communicatorSemaphore3;

    sem_t updateSemaphore;



    pthread_mutex_t mutexDataSafety;
    int playerInput;
    // If not given -> same as previous
    // If wrong -> nothing will happen

    player_status playerStatus;
    _Bool currentlyMoving;
    map_around_t mapAround;

    int coinsPicked;
    int coinsBrought;
    int deaths;

    _Bool isCollision;

    int playerIndex;

    int locked;

    int currentlyAtX;
    int currentlyAtY;

    _Bool hasJustDisconnected;

    __pid_t playerProcessID;
};

typedef struct {
    int x;
    int y;
    int coins;
} dropped_treasure_t;

#endif //LABYRINTHGAME_COMMUNICATOR_H
