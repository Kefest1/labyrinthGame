//
// Created by root on 11/10/22.
//

#ifndef LABYRINTHGAME_UTILS_H
#define LABYRINTHGAME_UTILS_H

#define FILE_MEM_SHARE "player.c"
#define MAX_PLAYER_COUNT 4
#define MAX_BEAST_COUNT 8

#include <pthread.h>

pthread_mutex_t playerConnectionMutex;

typedef enum {
    NOT_CONNECTED = 0,
    CONNECTED
} player_status;

typedef struct {
    _Bool playerConnected;
    int justConnectedIndex;
    int totalPlayers;
    player_status playerStatus[MAX_PLAYER_COUNT];

} player_connector_t;

typedef struct {
    int xPosition;
    int yPosition;

    int player_id;
    int coinsCarried;
    int coinsBrought;
    int deaths;
} player_t;

typedef struct {
    int xPosition;
    int yPosition;
} wild_beast_t;

typedef enum {
    WALL,
    FREE_BLOCK,
    LARGE_TREASURE,
    TREASURE,
    ONE_COIN,
    BUSHES,
    CAMPSITE,
    DROPPED_TREASURE,
    WILD_BEAST,
    PLAYER_1,
    PLAYER_2,
    PLAYER_3,
    PLAYER_4
} field_status_t;
// field_status_t fieldStatus[LABYRINTH_HEIGHT][LABYRINTH_WIDTH];

struct players_t {
    wild_beast_t wildBeast[MAX_BEAST_COUNT];
    player_t players[MAX_PLAYER_COUNT];
    int totalPlayers;
};


#endif //LABYRINTHGAME_UTILS_H
