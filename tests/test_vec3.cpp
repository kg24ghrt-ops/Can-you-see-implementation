#include <cassert>
#include <iostream>
#include "../src/Vec3.h"

int main() {
  Vec3 a{1, 2, 3};
  Vec3 b{4, 5, 6};
  Vec3 c = a.add(b);
  assert(c.x == 5 && c.y == 7 && c.z == 9);
  assert(a.dot(b) == 32);
  std::cout << "Vec3 OK\n";
  return 0;
}
