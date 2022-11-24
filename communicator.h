//
// Created by root on 11/24/22.
//

#ifndef LABYRINTHGAME_COMMUNICATOR_H
#define LABYRINTHGAME_COMMUNICATOR_H

#define MAX_PLAYERS 4

struct communicator_t {
    char playerInput[MAX_PLAYERS]; // If not given -> same as previous
    // If wrong -> nothing will happen

};

#endif //LABYRINTHGAME_COMMUNICATOR_H
