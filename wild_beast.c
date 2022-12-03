//
// Created by root on 11/7/22.
//

#include <stdio.h>
#include <pthread.h>
#include <ncurses.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
    sysconf(_SC_THREAD_PROCESS_SHARED);
    return 0;
}

