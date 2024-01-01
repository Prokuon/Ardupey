#ifndef MESSAGE
#define MESSAGE

#include "global_variables.h"
#include <Arduboy2.h>


class Message
{
public:
    Message(int8_t, int8_t, int16_t);
    int8_t type;
    int8_t anim;
    int8_t doAnim();

    bool isLive;
    int8_t index;
    int8_t blockNum;
    int16_t score;
};

#endif