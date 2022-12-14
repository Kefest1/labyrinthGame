//
// Created by root on 11/10/22.
//

#ifndef LABYRINTHGAME_SERVER_H
#define LABYRINTHGAME_SERVER_H

int findFreeIndex(void);
void readMap(void);
int *getRandomFreePosition(void);
void paintPlayer(int index, int x, int y);
void displayMap(void);
field_status_t getStatusFromIndex(int index);
field_status_t getStatusFromIndexBushed(int index);
int getDroppedTreasureCoins(int x, int y);
void debugging();
void testKeys(void);
void playDebugging(void);
int movePlayer(int index, player_move_dir playerMoveDir);
player_move_dir getMoveDirFromInput(int input);
void collision(int index1, int index2);
void finalize();
void fillSharedMap(int index);
int getSharedBlock(char *filename, size_t size, int index);
int getFreeTreasurePos(void);
int isPlayer(field_status_t fieldStatus);
int getIndexFromStatus(field_status_t fieldStatus);
void moveBeast(int index);
void *wildBeastFunction(void *ptr);
char getCharacterFromStatus(field_status_t fieldStatusChar);

#endif //LABYRINTHGAME_SERVER_H
