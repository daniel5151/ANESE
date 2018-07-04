#pragma once

#include "common/util.h"

// idk what i'm doing, but wikipedia is nice enough to give some sample
// code, so I think these work alright

typedef uint hertz;

class FirstOrderFilter {
protected:
  const double RC, dt;
public:
  virtual double process(double sample) = 0;

  virtual ~FirstOrderFilter() = default;
  FirstOrderFilter(hertz f, hertz sample_rate)
  : RC { 1.0 / (2 * 3.1415928535 * double(f)) }
  , dt { 1.0 / double(sample_rate) }
  {}
};

// https://en.wikipedia.org/wiki/Low-pass_filter
class LoPassFilter final : public FirstOrderFilter {
private:
  const double a;
  struct { double x, y; } prev;
public:
  virtual ~LoPassFilter() = default;
  LoPassFilter(hertz f, hertz sample_rate)
  : FirstOrderFilter(f, sample_rate)
  , a { this->dt / (this->RC + this->dt) }
  , prev { 0, 0 }
  {}

  double process(double x) override {
    double y = this->a * x + (1 - this->a) * this->prev.y;
    this->prev = { x, y };
    return y;
  }
};

// https://en.wikipedia.org/wiki/High-pass_filter
class HiPassFilter final : public FirstOrderFilter {
private:
  const double a;
  struct { double x, y; } prev;
public:
  virtual ~HiPassFilter() = default;
  HiPassFilter(hertz f, hertz sample_rate)
  : FirstOrderFilter(f, sample_rate)
  , a { this->RC / (this->RC + this->dt) }
  , prev { 0, 0 }
  {}

  double process(double x) override {
    double y = this->a * this->prev.y + this->a * (x - this->prev.x);
    this->prev = { x, y };
    return y;
  }
};
