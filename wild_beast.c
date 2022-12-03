//
// Created by root on 11/7/22.
//

#include "utils.h"
#include <stdio.h>
#include <ncurses.h>

int main(void) {
    initscr();
    refresh();
    int x = -2137;
    x = getch();
    printf("%d ", x);
    x = getch();
    printf("%d ", x);
    x = getch();
    printf("%d ", x);

    endwin();
    return 0;
}

