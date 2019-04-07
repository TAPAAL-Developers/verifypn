/*
 *  Copyright Peter G. Jensen, all rights reserved.
 */

/* 
 * File:   SearhStrategies.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on April 6, 2019, 12:27 PM
 */

#ifndef SEARHSTRATEGIES_H
#define SEARHSTRATEGIES_H
namespace Utils {
    namespace SearchStrategies {

        enum Strategy {
            BFS,
            DFS,
            HEUR,
            RDFS,
            OverApprox,
            DEFAULT
        };
    }
};
#endif /* SEARHSTRATEGIES_H */

