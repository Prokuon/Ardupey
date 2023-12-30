#include "Player.h"

Player::Player() {
  init();
}

void Player::focusPause() {
  isFocusPause = true;
  set( -1, -1 );
}

void Player::defocusPause() {
  isFocusPause = false;
  set( oldX, oldY );
}

void Player::init() {
  x = 0;
  y = 0;
  // oldX = 0;
  // oldY = 0;
  isFocusPause = false;
}

void Player::set(int8_t newX, int8_t newY) {
  oldX = x;
  oldY = y;
  x = newX;
  y = newY;
}
