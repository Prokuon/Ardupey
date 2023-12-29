#include "Block.h"

Block::Block(int8_t tp) {
  type = tp;
  isLive = false;
  isChained = false;
  isRemoving = false;
}
