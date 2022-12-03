//
// Created by root on 11/10/22.
//

#ifndef LABYRINTHGAME_UTILS_H
#define LABYRINTHGAME_UTILS_H

#define FILE_CONNECTOR "player.c"
#define FILE_COMMUNICATOR "server.c"
#include <stdlib.h>

#define MAX_PLAYER_COUNT 4
#define MAX_BEAST_COUNT 8

#define LARGE_TREASURE_COINS 50
#define TREASURE_COINS 10
#define ONE_COIN_COINS 1

#include <pthread.h>

//pthread_mutex playerConnectionMutex;
//pthread_mutex playerControllerMutex;

typedef enum {
    NOT_CONNECTED = 0,
    CONNECTED
} player_status;

// Shared between player processes //
typedef struct {
    pthread_mutex_t *pthreadMutex;

    _Bool playerConnected;
    int totalPlayerCount;
    int freeIndex; // Doesn't matter if max //
    __pid_t serverPid;
} player_connector_t;



typedef struct {
    int xPosition;
    int yPosition;

    int xStartPosition;
    int yStartPosition;

    int player_id;
    int coinsCarried;
    int coinsBrought;
    int deaths;


    int locked;
} player_t;

typedef struct {
    int xPosition;
    int yPosition;
    int isOnBushes;
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
    PLAYER_4,
    PLAYER_1_ON_BUSH,
    PLAYER_2_ON_BUSH,
    PLAYER_3_ON_BUSH,
    PLAYER_4_ON_BUSH
} field_status_t;
// field_status_t fieldStatus[LABYRINTH_HEIGHT][LABYRINTH_WIDTH];

struct players_t {
    wild_beast_t wildBeast[MAX_BEAST_COUNT];
    player_t players[MAX_PLAYER_COUNT];
    int totalPlayers;
};

typedef enum {
    MOVE_LEFT,
    MOVE_RIGHT,
    MOVE_UP,
    MOVE_DOWN,

    PLAYER_QUIT
} player_move_dir;

typedef struct {
    int x;
    int y;
} campsite_t;

typedef enum {
    CPU,
    HUMAN
} player_type_t;

#endif //LABYRINTHGAME_UTILS_H
