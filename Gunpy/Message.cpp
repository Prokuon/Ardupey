#include "Message.h"

Message::Message(int8_t tp, int8_t bNum, int16_t scr)
{
    isLive = false;
    type = tp;
    index = 0;
    blockNum = bNum;
    score = scr;
    anim = Setting::MESSAGE_ANIM_CNT;
}

int8_t Message::doAnim()
{
    if (anim <= 0)
    {
        return 0;
    }
    return anim--;
}
