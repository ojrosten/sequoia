////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "StaticQueueTestingDiagnostics.hpp"
#include "StaticQueueTestingUtilities.hpp"

namespace sequoia::unit_testing
{
  void test_static_queue_false_positives::run_tests()
  {
    check_depth_0();
    check_depth_1();
  }

  void test_static_queue_false_positives::check_depth_0()
  {
    using namespace data_structures;

    static_queue<int, 0> s{};

    check(!s.empty(), LINE("Empty queue must be empty"));
    check_equality(s.size(), 1ul, LINE("Empty queue must have size zero"));
  }
  
  void test_static_queue_false_positives::check_depth_1()
  {
    using namespace data_structures;

    static_queue<int, 1> s{}, t{};
    t.push(1);

    check_equality(s, t, LINE("Empty queue versus populated queue"));

    s.push(2);
    check_equality(s, t, LINE("Differing elements"));

    s.pop();
    check_equality(s, t, LINE("Empty queue versus populated queue"));
  }
}
