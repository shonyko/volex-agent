#ifndef UTILS_H
#define UTILS_H

#include <math.h>

int clamp(const int x, const int min, const int max) {
  return std::max(min, std::min(max, x));
}

#endif