#pragma once

#include "UnitTestUtils.hpp"

namespace sequoia
{
  namespace unit_testing
  {
    
    constexpr double modf(double x, double* intpart)
    {
      *intpart = 1.0;
      return x;
    }

    constexpr double frexp(double x, int* exp)
    {
      *exp = 1;
      return x;
    }
    
    class test_experimental : public unit_test
    {      
    public:
      using unit_test::unit_test;
    private:
      void run_tests();

      template<class T> struct component {};

      template<class T, class C> struct thing {};
      
      template<class T, template<class> class C> struct thing2 {};      

      template<class T> using alias = component<T>;

      template<class T>
      struct wrapper
      {
        wrapper(T& ref) : m_Ref{ref} {}
        
        wrapper(const wrapper&)            noexcept = default;
        wrapper(wrapper&&)                 noexcept = default;
        wrapper& operator=(const wrapper&) noexcept(false){} // required to reveal bug; removing false fixes compilation
        wrapper& operator=(wrapper&&)      noexcept = default;

        private:
          T& m_Ref; // Behaves with reference_wrapper
      };

      template<class T, class W>
      class thing3
      {
      public:
        thing3(T& ref) : m_Member{ref} {}

        thing3(const thing3&) noexcept = default;
        thing3(thing3&&)      noexcept = default;

        thing3& operator=(const thing3&)               = default;
        constexpr thing3& operator=(thing3&&) noexcept = default;
      private:
        W m_Member{};
      };
      
      static constexpr double divby10(double x)
      {
        return x/10.0;
      }

      static double divby10B(double x)
      {
        return x/10.0;
      }
    };
  }
}
