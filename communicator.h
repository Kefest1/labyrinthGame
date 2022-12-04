//
// Created by root on 11/24/22.
//

#ifndef LABYRINTHGAME_COMMUNICATOR_H
#define LABYRINTHGAME_COMMUNICATOR_H

#include "utils.h"
#include <semaphore.h>

#define MAX_PLAYERS MAX_PLAYER_COUNT

#define RANGE_OF_VIEW 5
#define ROUND_DURATION_SECONDS 3u
#define ROUND_DURATION_MILLI_SECONDS (1000u * ROUND_DURATION_SECONDS)

#define DEBUG_SLEEP 60u

typedef struct {
    field_status_t aroundPlayers[RANGE_OF_VIEW][RANGE_OF_VIEW];
    int campsiteX;
    int campsiteY;
} map_around_t;


// Shared between one player and server //
struct player_server_communicator_t {
    player_move_dir playerMoveDir;

    int playerIndex;
    int playerProcessID;

    int coinsBrought;
    int coinsCarried;
};

struct communicator_t {
    pthread_mutex_t connectorMutex;
//    sem_t semaphore;

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
};

typedef struct {
    int x;
    int y;
    int coins;
} dropped_treasure_t;

#endif //LABYRINTHGAME_COMMUNICATOR_H
