#include "Vec3.h"
#include <cmath>

Vec3 Vec3::add(const Vec3& other) const {
  return {x + other.x, y + other.y, z + other.z};
}

float Vec3::dot(const Vec3& other) const {
  return x * other.x + y * other.y + z * other.z;
}

float Vec3::length() const {
  return std::sqrt(dot(*this));
}
