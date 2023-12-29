#ifndef PLAYER
#define PLAYER

#include <Arduboy2.h>

class Player {
public:
  Player();
  void init();
  int8_t x, y;
  int8_t oldX, oldY;
  bool isFocusPause;
  void set( int8_t, int8_t );
  void focusPause();
  void defocusPause();

};

#endif