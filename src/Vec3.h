#pragma once
// Vec3 - core logic only, no Android, no GPU, no file IO
struct Vec3 {
  float x, y, z;
  Vec3 add(const Vec3& other) const;
  float dot(const Vec3& other) const;
  float length() const;
};
