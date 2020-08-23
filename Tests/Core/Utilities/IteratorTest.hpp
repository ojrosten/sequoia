////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "RegularTestCore.hpp"

namespace sequoia::testing
{
  class iterator_test final : public regular_test
  {
  public:
    using regular_test::regular_test;

    [[nodiscard]]
    std::string_view source_file() const noexcept final;
  private:
    void run_tests() final;
    
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
    constexpr value_type scale() const noexcept { return m_Scale; }

    [[nodiscard]]
    friend constexpr bool operator==(const scaling_dereference_policy&, const scaling_dereference_policy&) noexcept = default;

    [[nodiscard]]
    friend constexpr bool operator!=(const scaling_dereference_policy&, const scaling_dereference_policy&) noexcept = default;
  protected:
    constexpr scaling_dereference_policy(scaling_dereference_policy&&) = default;

    ~scaling_dereference_policy() = default;

    constexpr scaling_dereference_policy& operator=(const scaling_dereference_policy&) = default;
    constexpr scaling_dereference_policy& operator=(scaling_dereference_policy&&)      = default;
    
    [[nodiscard]]
    constexpr proxy get(typename std::iterator_traits<Iterator>::reference ref) const noexcept
    {
      return ref*m_Scale;
    }

    [[nodiscard]]
    static constexpr pointer get_ptr(reference ref) noexcept { return &ref; }
  private:
    value_type m_Scale{1};
  };
}
