////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "StaticPriorityQueueTestingDiagnostics.hpp"
#include "StaticPriorityQueueTestingUtilities.hpp"

namespace sequoia::unit_testing
{
  void test_static_priority_queue_false_positives::run_tests()
  {
    check_depth_1();
    check_depth_2();
  }

  void test_static_priority_queue_false_positives::check_depth_1()
  {
    using namespace data_structures;

    static_priority_queue<int, 1> s{}, t{};
    t.push(1);

    check_equality(LINE("Empty queue versus populated queue"), s, t);

    s.push(2);
    check_equality(LINE("Differing elements"), s, t);

    s.pop();
    check_equality(LINE("Empty queue versus populated queue"), s, t);
  }

  void test_static_priority_queue_false_positives::check_depth_2()
  {
    using namespace data_structures;

    static_priority_queue<int, 2> s{1, 2}, t{1};
    check_equality(LINE("Two element queue versus one element queue"), s, t);
  }
}
