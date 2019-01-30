////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "UnitTestUtils.hpp"

namespace sequoia::unit_testing
{
  class iterator_test : public unit_test
  {
  public:
    using unit_test::unit_test;
  private:
    void run_tests();
    
    template<class CustomIter, class Iter, class Pointer=typename CustomIter::pointer>
    void basic_checks(Iter begin, Iter end, Pointer pBegin, std::string_view message);

    void test_iterator();
    void test_const_iterator();
    void test_reverse_iterator();
    void test_const_reverse_iterator();
  };

  template<class Iterator>
  struct scaling_dereference_policy
  {
    using value_type = typename std::iterator_traits<Iterator>::value_type;
    using reference = typename std::iterator_traits<Iterator>::reference;
    using pointer = typename std::iterator_traits<Iterator>::pointer;

    [[nodiscard]]
    static constexpr reference get(reference ref) noexcept { return ref; }
  };
}
