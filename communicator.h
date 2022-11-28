//
// Created by root on 11/24/22.
//

#ifndef LABYRINTHGAME_COMMUNICATOR_H
#define LABYRINTHGAME_COMMUNICATOR_H

#include "utils.h"
#include <semaphore.h>
#define MAX_PLAYERS MAX_PLAYER_COUNT

#define RANGE_OF_VIEW 5
#define ROUND_DURATION_SECONDS 1
#define ROUND_DURATION_MILLI_SECONDS (1000 * ROUND_DURATION_SECONDS)

typedef struct {
    field_status_t aroundPlayers[RANGE_OF_VIEW][RANGE_OF_VIEW];
    int campsiteX;
    int campsiteY;
} map_around_t;

struct communicator_t {
//    sem_t semaphore;

    char playerInput[MAX_PLAYERS]; // If not given -> same as previous
    // If wrong -> nothing will happen

    _Bool isConnected[MAX_PLAYERS];
    _Bool currentlyMoving[MAX_PLAYERS];
    map_around_t mapAround[MAX_PLAYERS];
    int coinsPicked[MAX_PLAYERS];
    int deaths[MAX_PLAYERS];

    int currentlyMovingPlayerIndex;
};

typedef struct {
    int x;
    int y;
    int coins;
} dropped_treasure_t;

#endif //LABYRINTHGAME_COMMUNICATOR_H
