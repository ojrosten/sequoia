////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "StaticQueueTestingDiagnostics.hpp"
#include "StaticQueueTestingUtilities.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view test_static_queue_false_positives::source_file() const noexcept
  {
    return __FILE__;
  }

  void test_static_queue_false_positives::run_tests()
  {
    check_depth_0();
    check_depth_1();
  }

  void test_static_queue_false_positives::check_depth_0()
  {
    using namespace data_structures;

    static_queue<int, 0> s{};

    check(LINE("Empty queue must be empty"), !s.empty());
    check_equality(LINE("Empty queue must have size zero"), s.size(), 1ul);
  }
  
  void test_static_queue_false_positives::check_depth_1()
  {
    using namespace data_structures;

    static_queue<int, 1> s{}, t{};
    t.push(1);

    check_equality(LINE("Empty queue versus populated queue"), s, t);

    s.push(2);
    check_equality(LINE("Differing elements"), s, t);

    s.pop();
    check_equality(LINE("Empty queue versus populated queue"), s, t);
  }
}
