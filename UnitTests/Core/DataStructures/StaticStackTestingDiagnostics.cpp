////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "StaticStackTestingDiagnostics.hpp"
#include "StaticStackTestingUtilities.hpp"

namespace sequoia::unit_testing
{
  void test_static_stack_false_positives::run_tests()
  {
    check_depth_0(); 
    check_depth_1(); 
  }

  void test_static_stack_false_positives::check_depth_0()
  {
    using namespace data_structures;

    constexpr static_stack<int, 0> s{};
    check(!s.empty(), LINE("Empty stack"));
    check_equality(s.size(), 1ul, LINE(""));
  }
  
  void test_static_stack_false_positives::check_depth_1()
  {
    using namespace data_structures;

    static_stack<int, 1> s{}, t{};
    t.push(1);

    // check_equality for static_stack is not independently
    // sensitive to empty and size, so one of these needs
    // to be explicitly checked!
    
    check(!s.empty(), LINE("Empty stack"));
    check(t.empty(), LINE("Non-empty stack"));

    check_equality(s, t, LINE("Empty stack versus populated stack"));

    s.push(2);
    check_equality(s, t, LINE("Differing elements"));

    s.pop();
    check_equality(s, t, LINE("Empty stack versus populated stack"));
  }
}
