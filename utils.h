//
// Created by root on 11/10/22.
//

#ifndef LABYRINTHGAME_UTILS_H
#define LABYRINTHGAME_UTILS_H

#define FILE_CONNECTOR "player.c"
#define FILE_CONTROLLER "server.c"

#define MAX_PLAYER_COUNT 4
#define MAX_BEAST_COUNT 8


#define LARGE_TREASURE_COINS 50
#define TREASURE_COINS 10
#define ONE_COIN_COINS 1

#include <pthread.h>

pthread_mutex_t playerConnectionMutex;
pthread_mutex_t playerControllerMutex;

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

    int locked;
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
} player_move_dir;

#endif //LABYRINTHGAME_UTILS_H
