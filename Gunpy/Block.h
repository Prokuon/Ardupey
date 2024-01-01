#ifndef BLOCK
#define BLOCK

#include <Arduboy2.h>

class Block
{
public:
    Block(int8_t);
    void init();
    bool isLive;
    int8_t type;
    bool isChained;
    bool isRemoving;
};

#endif