////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "StaticQueueTestingDiagnostics.hpp"
#include "StaticQueueTestingUtilities.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path test_static_queue_false_positives::source_file() const
  {
    return std::source_location::current().file_name();
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

    check("Empty queue must be empty", !s.empty());
    check(equality, "Empty queue must have size zero", s.size(), 1_sz);
  }

  void test_static_queue_false_positives::check_depth_1()
  {
    using namespace data_structures;

    static_queue<int, 1> s{}, t{};
    t.push(1);

    check(equality, "Empty queue versus populated queue", s, t);

    s.push(2);
    check(equality, "Differing elements", s, t);

    s.pop();
    check(equality, "Empty queue versus populated queue", s, t);
  }
}
