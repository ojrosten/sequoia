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

      //template<class T, class C> struct thing {};
      
      //template<class T, template<class> class C> struct thing2 {};      

      template<class T> using alias = component<T>;

      template<class T>
      class wrapper
      {
        T& m_Ref; // Behaves with reference_wrapper
        //std::reference_wrapper<T> m_Ref;
      public:
        wrapper(T& ref) : m_Ref{ref} {}
        
        wrapper(const wrapper&)            = default;
        wrapper(wrapper&&)                 noexcept = default;
        wrapper& operator=(const wrapper&) noexcept(false){} // required to reveal bug; 
        //removing false fixes compilation
        wrapper& operator=(wrapper&&)      noexcept = default;

      };

      template<class T, class W>
      class thing
      {
      private:
        W m_Member{};
      public:
        thing(T& ref) : m_Member{ref} {}

        thing(const thing&) = default;
        thing(thing&&)      = default;

        thing& operator=(const thing&)     = default;
        thing& operator=(thing&&) noexcept = default;
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
