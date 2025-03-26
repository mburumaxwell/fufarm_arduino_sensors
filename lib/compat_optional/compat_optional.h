#pragma once

#if __cplusplus >= 201703L
#include <optional>
namespace compat
{
  using std::nullopt;
  using std::nullopt_t;
  using std::optional;
}
#else
// Simple fallback optional implementation
namespace compat
{
  struct nullopt_t
  {
    explicit constexpr nullopt_t(int) {}
  };
  constexpr nullopt_t nullopt{0};

  template <typename T>
  class optional
  {
    bool has_value_ = false;
    T value_;

  public:
    optional() = default;
    optional(nullopt_t) : has_value_(false) {}

    optional(const T &val) : has_value_(true), value_(val) {}
    optional(T &&val) : has_value_(true), value_(std::move(val)) {}

    optional &operator=(const T &val)
    {
      value_ = val;
      has_value_ = true;
      return *this;
    }

    optional &operator=(T &&val)
    {
      value_ = std::move(val);
      has_value_ = true;
      return *this;
    }

    optional &operator=(nullopt_t)
    {
      has_value_ = false;
      return *this;
    }

    bool has_value() const { return has_value_; }
    explicit operator bool() const { return has_value_; }

    T &value()
    {
      // In real impl, you'd throw or handle error
      return value_;
    }

    const T &value() const
    {
      return value_;
    }

    T value_or(const T &fallback) const
    {
      return has_value_ ? value_ : fallback;
    }

    void reset() { has_value_ = false; }
  };
}
#endif