////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "StaticPriorityQueueTestingDiagnostics.hpp"
#include "StaticPriorityQueueTestingUtilities.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path test_static_priority_queue_false_positives::source_file() const noexcept
  {
    return std::source_location::current().file_name();
  }

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

    check(equality, report_line("Empty queue versus populated queue"), s, t);

    s.push(2);
    check(equality, report_line("Differing elements"), s, t);

    s.pop();
    check(equality, report_line("Empty queue versus populated queue"), s, t);
  }

  void test_static_priority_queue_false_positives::check_depth_2()
  {
    using namespace data_structures;

    static_priority_queue<int, 2> s{1, 2}, t{1};
    check(equality, report_line("Two element queue versus one element queue"), s, t);
  }
}
