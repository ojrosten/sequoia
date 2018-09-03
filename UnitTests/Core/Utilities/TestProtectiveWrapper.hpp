#pragma once

#include "UnitTestUtils.hpp"

namespace sequoia
{
  namespace unit_testing
  {
    class test_protective_wrapper : public unit_test
    {
    public:
      using unit_test::unit_test;

    private:
      void run_tests();

      template<class T>
      static constexpr T invert(const T& in)
      {
        T val;
        val.set(-in.get());
        return val;
      }
    };

    template <class T> class point
    {
    public:
      constexpr point() {};
      constexpr point(const T x, const T y) : _x(x), _y(y) {}

      constexpr T x() const { return _x; }
      constexpr T y() const { return _y; }
      
      constexpr point& operator+=(const T translation)
      {
        _x += translation;
        _y += translation;

        return *this;
      }
      
      constexpr point& operator*=(const T scale)
      {
        _x *= scale;
        _y *= scale;

        return *this;
      }
    private:
      T _x{0},_y{0};
    };

    template<class T, class S> constexpr T scale(const T& wrappedPt, const S scale)
    {
      T out{};
      auto pt = wrappedPt.get();
      out.set(pt *= scale);
      return out;
    }
  }
}
