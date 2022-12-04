/*void collision(int index1, int index2) {
    int posFrom1[2];
    posFrom1[0] = players->players[index1].xPosition;
    posFrom1[1] = players->players[index1].yPosition;

    int posFrom2[2]; // Also place of colision //
    posFrom2[0] = players->players[index2].xPosition;
    posFrom2[1] = players->players[index2].yPosition;

    int isOnBush1 = fieldStatus[posFrom1[0]][posFrom1[1]] == getStatusFromIndexBushed(index1);
    int isOnBush2 = fieldStatus[posFrom1[0]][posFrom1[1]] == getStatusFromIndexBushed(index2);


    int *posTo1;
    int *posTo2;
    do {
        posTo1 = getRandomFreePosition();
        posTo2 = getRandomFreePosition();
    } while (posTo1[0] == posTo2[0] && posTo1[1] == posTo2[1]);


    players->players[index1].xPosition = posTo1[0];
    players->players[index1].yPosition = posTo1[1];

    players->players[index2].xPosition = posTo2[0];
    players->players[index2].yPosition = posTo2[1];

    playerCommunicator[index1]->currentlyAtX = posTo1[0];
    playerCommunicator[index1]->currentlyAtY = posTo1[1];

    playerCommunicator[index2]->currentlyAtX = posTo2[0];
    playerCommunicator[index2]->currentlyAtY = posTo2[1];

    int droppedTreasureVal =
            players->players[index1].coinsCarried + players->players[index2].coinsCarried;

    players->players[index1].coinsCarried = 0;
    players->players[index2].coinsCarried = 0;

    playerCommunicator[index1]->coinsPicked = 0;
    playerCommunicator[index2]->coinsPicked = 0;

    if (isOnBush1) {
        fieldStatus[posFrom1[0]][posFrom1[1]] = BUSHES;
        mvwprintw(win, posFrom1[0], posFrom1[1], "#");
    }
    else {
        fieldStatus[posFrom1[0]][posFrom1[1]] = FREE_BLOCK;
        mvwprintw(win, posFrom1[0], posFrom1[1], " ");
    }

    if (isOnBush2) {
        fieldStatus[posFrom2[0]][posFrom2[1]] = BUSHES;
        mvwprintw(win, posFrom2[0], posFrom2[1], "#");
    }

    else {
        fieldStatus[posFrom2[0]][posFrom2[1]] = FREE_BLOCK;
        mvwprintw(win, posFrom2[0], posFrom2[1], " ");
    }

    fieldStatus[posFrom2[0]][posFrom2[1]] = DROPPED_TREASURE;


    wrefresh(win);
    refresh();

    free(posTo1);
    free(posTo2);
}*/
