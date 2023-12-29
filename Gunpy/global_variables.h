#ifndef GLOBAL_VARIABLES_H
#define GLOBAL_VARIABLES_H

#include <Arduboy2.h>

namespace Blc {
  extern const int8_t TYPE_1; // ＼
  extern const int8_t TYPE_2; // ／
  extern const int8_t TYPE_3; // ＼／
  extern const int8_t TYPE_4; // ／＼
}

namespace Setting {

  extern const int8_t SCENE_TITLE;
  extern const int8_t SCENE_PLAY;
  extern const int8_t SCENE_PAUSE;
  extern const int8_t SCENE_GAMEOVER;
  extern const int8_t SCENE_HELP;
  extern const int8_t SCENE_HELP2;

  extern const int8_t FPS;

  extern const int8_t MESSAGE_ANIM_CNT; // 0->FPS=view, FPS->+10=Fadeout

  extern const int8_t MSG_HIT;
  extern const int8_t MSG_HIT_ADD;
  extern const int8_t MSG_HIT_SP; // 未使用欠番
  extern const int8_t MSG_LEVEL_UP;
  extern const int8_t MSG_ALL_CLEAR;


}

#endif
