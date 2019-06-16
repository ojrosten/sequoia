////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "UnitTestCore.hpp"

namespace sequoia::unit_testing
{
  class iterator_test : public unit_test
  {
  public:
    using unit_test::unit_test;
  private:
    void run_tests();
    
    template<
      class CustomIter,
      class Iter,
      class... Args,
      class Pointer=typename CustomIter::pointer
    >
    void basic_checks(Iter begin, Iter end, Pointer pBegin, std::string_view message, Args... args);

    void test_iterator();
    void test_const_iterator();
    void test_reverse_iterator();
    void test_const_reverse_iterator();

    void test_const_scaling_iterator();
    void test_const_reverse_scaling_iterator();
  };

  template<class Iterator>
  class scaling_dereference_policy
  {
  public:    
    using value_type = typename std::iterator_traits<Iterator>::value_type;
    using proxy      = value_type;
    using pointer    = typename std::iterator_traits<Iterator>::pointer;
    using reference  = typename std::iterator_traits<Iterator>::reference;

    constexpr scaling_dereference_policy(const value_type scale) : m_Scale{scale} {}
    constexpr scaling_dereference_policy(const scaling_dereference_policy&) = default;
    
    [[nodiscard]]
    constexpr proxy get(typename std::iterator_traits<Iterator>::reference ref) const noexcept
    {
      return ref*m_Scale;
    }

    [[nodiscard]]
    static constexpr pointer get_ptr(reference ref) noexcept { return &ref; }

    [[nodiscard]]
    constexpr value_type scale() const noexcept { return m_Scale; }
  protected:    
    constexpr scaling_dereference_policy(scaling_dereference_policy&&)      = default;

    ~scaling_dereference_policy() = default;

    constexpr scaling_dereference_policy& operator=(const scaling_dereference_policy&) = default;
    constexpr scaling_dereference_policy& operator=(scaling_dereference_policy&&)      = default;

  private:
    value_type m_Scale{1};
  };

  template<class DerefPolicy, class=std::void_t<>>
  struct is_scaling : std::false_type
  {};

  template<class DerefPolicy>
  struct is_scaling<DerefPolicy, std::void_t<decltype(std::declval<DerefPolicy>().scale())>> : std::true_type
  {};

  template<class DerefPolicy>
  constexpr bool is_scaling_v{is_scaling<DerefPolicy>::value};
}
