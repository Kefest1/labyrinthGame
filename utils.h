//
// Created by root on 11/10/22.
//

#ifndef LABYRINTHGAME_UTILS_H
#define LABYRINTHGAME_UTILS_H

#define FILE_MEM_SHARE "player.c"
#define MAX_PLAYER_COUNT 4

#include <pthread.h>

pthread_mutex_t playerConnectionMutex;

typedef struct {
    _Bool playerConnected;
    _Bool okToConnect;
    int justConnectedIndex;
    int totalPlayers;
} player_connector_t;

typedef struct {
    int xPosition;
    int yPosition;

    int player_id;
    int coinsCarried;
    int coinsBrought;
    int deaths;
} player_t;

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

typedef enum {
    CONNECTED,
    NOT_CONNECTED
} player_status;

struct players_t {
    player_t players[MAX_PLAYER_COUNT];
    player_status playerStatus[MAX_PLAYER_COUNT];
    int totalPlayers;
};


#endif //LABYRINTHGAME_UTILS_H
