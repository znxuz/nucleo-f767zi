#pragma once

#include <cmath>
#include <cstdio>
#include <limits>

constexpr static inline double inf = std::numeric_limits<double>::infinity();
constexpr static inline double eps = std::numeric_limits<double>::epsilon();

/* Heading of a mobile robot in the range of -pi ... +pi */
template <typename T>
class Heading {
 private:
  T th{};

  T maxPI(T theta) const {
    while (theta > T(M_PI)) theta -= T(2 * M_PI);
    while (theta < T(-M_PI)) theta += T(2 * M_PI);
    return theta;
  }

 public:
  Heading() = default;

  Heading(T theta) { th = maxPI(theta); }

  Heading& operator=(const T& theta) {
    th = maxPI(theta);
    return *this;
  }

  Heading operator+(const T& d) const {
    Heading t = maxPI(th + d);
    return t;
  }

  Heading& operator+=(const T& d) {
    th = maxPI(th + d);
    return *this;
  }

  Heading operator-(const T& d) const {
    Heading t = maxPI(th - d);
    return t;
  }

  Heading& operator-=(const T& d) {
    th = maxPI(th - d);
    return *this;
  }

  operator T() const { return th; }
};

template <typename T>
class Pose;

template <typename T>
class vPose {
 public:
  T vx{};
  T vy{};
  T omega{};

  vPose() = default;

  vPose(T vx, T vy, T omega) : vx{vx}, vy{vy}, omega{omega} {}

  bool operator==(const vPose<T>& rhs) const {
    return this->vx == rhs.vx && this->vy == rhs.vy && this->omega == rhs.omega;
  }

  vPose& operator+=(const vPose<T>& rhs) {
    this->vx += rhs.vx;
    this->vy += rhs.vy;
    this->omega += rhs.omega;
    return *this;
  }

  // FIXME: do not convert here, just multiply; create ctor for conversion
  friend Pose<T> operator*(const vPose<T>& lhs, const T factor) {
    return Pose<T>{lhs.vx * factor, lhs.vy * factor, lhs.omega * factor};
  }

  friend vPose<T> operator+(const vPose<T>& lhs, const vPose<T>& rhs) {
    return vPose<T>{lhs.vx + rhs.vx, lhs.vy + rhs.vy, lhs.omega + rhs.omega};
  }

  friend vPose<T> operator-(const vPose<T>& lhs, const vPose<T>& rhs) {
    return vPose<T>{lhs.vx - rhs.vx, lhs.vy - rhs.vy, lhs.omega - rhs.omega};
  }

  /* transformation of velocities from robot frame into world frame  */
  friend vPose<T> vRF2vWF(vPose<T> vRF, T theta) {
    return vPose<T>{vRF.vx * cos(theta) - vRF.vy * sin(theta),
                    vRF.vx * sin(theta) + vRF.vy * cos(theta), vRF.omega};
  }

  /* transformation of velocities from world frame into robot frame  */
  friend vPose<T> vWF2vRF(vPose<T> vWF, T theta) {
    return vPose<T>{vWF.vx * cos(theta) + vWF.vy * sin(theta),
                    -vWF.vx * sin(theta) + vWF.vy * cos(theta), vWF.omega};
  }
};

template <typename T>
class Pose {
 public:
  T x{};
  T y{};
  Heading<T> theta{};

  Pose() = default;

  Pose(T x, T y, Heading<T> theta) : x{x}, y{y}, theta{theta} {}

  bool operator==(const Pose<T>& rhs) const {
    return this->x == rhs.x && this->y == rhs.y && this->theta == rhs.theta;
  }

  Pose& operator+=(const Pose<T>& rhs) {
    this->x += rhs.x;
    this->y += rhs.y;
    this->theta += rhs.theta;
    return *this;
  }

  friend Pose<T> operator+(const Pose<T>& lhs, const Pose<T>& rhs) {
    return Pose<T>{lhs.x + rhs.x, lhs.y + rhs.y, lhs.theta + rhs.theta};
  }

  friend Pose<T> operator-(const Pose<T>& lhs, const Pose<T>& rhs) {
    return Pose<T>{lhs.x - rhs.x, lhs.y - rhs.y, lhs.theta - rhs.theta};
  }

  // FIXME: do not convert here, just multiply; create ctor for conversion
  friend vPose<T> operator/(const Pose<T>& p1, const T divisor) {
    if (divisor == 0.0) {
      std::perror("division by zero!");
      exit(1);
    }
    return vPose<T>{p1.x / divisor, p1.y / divisor, p1.theta / divisor};
  }

  /* transformation of small movements from robot frame into world frame  */
  friend Pose<T> pRF2pWF(const Pose<T>& pose, T th) {
    return Pose<T>{pose.x * cos(th) - pose.y * sin(th),
                   pose.x * sin(th) + pose.y * cos(th), pose.theta};
  }

  /* transformation of small movements from world frame into robot frame  */
  friend Pose<T> pWF2pRF(const Pose<T>& d_pose, T th) {
    return Pose<T>{d_pose.x * cos(th) + d_pose.y * sin(th),
                   -d_pose.x * sin(th) + d_pose.y * cos(th), d_pose.theta};
  }
};

