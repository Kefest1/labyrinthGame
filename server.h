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

#endif //LABYRINTHGAME_SERVER_H
