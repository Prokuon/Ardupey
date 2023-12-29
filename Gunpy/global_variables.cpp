#include "global_variables.h"

namespace Blc {
const int8_t TYPE_1 = 0;
const int8_t TYPE_2 = 1;
const int8_t TYPE_3 = 2;
const int8_t TYPE_4 = 3;
}

namespace Setting {
  
const int8_t SCENE_TITLE = 0;
const int8_t SCENE_PLAY = 1;
const int8_t SCENE_PAUSE = 2;
const int8_t SCENE_GAMEOVER = 3;
const int8_t SCENE_HELP = 4;
const int8_t SCENE_HELP2 = 5;

const int8_t FPS = 30;

const int8_t MESSAGE_ANIM_CNT = ( FPS * 2 ) + 10; // 0->FPS=view, FPS->+10=Fadeout

const int8_t MSG_HIT = 1;
const int8_t MSG_HIT_ADD = 2;
const int8_t MSG_HIT_SP = 3;
const int8_t MSG_LEVEL_UP = 4;
const int8_t MSG_ALL_CLEAR = 5;

}