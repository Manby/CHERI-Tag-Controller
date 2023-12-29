//
// Created by kofi on 28/12/23.
//

#ifndef SIMULATOR_CACHE_H
#define SIMULATOR_CACHE_H


class Cache {

/* TODO: Should this class be inside Controller?
 * This way, I think we can have it so only the Controller class can access Cache's methods, which is nice hiding.
 */
public:
    Cache();
    void read();
    void write();
};


#endif //SIMULATOR_CACHE_H
