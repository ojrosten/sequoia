////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "PriorityQueueTestingDiagnostics.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path priority_queue_false_negative_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void priority_queue_false_negative_test::run_tests()
  {
    using queue_t = observed_priority_queue<int>;

    const queue_t empty{};
    const auto one{make_by_pushing<queue_t>({1})},
               two{make_by_pushing<queue_t>({2})},
               oneTwo{make_by_pushing<queue_t>({1, 2})};

    check(equality, "Empty against one element", empty, one);
    check(equality, "Differing elements", one, two);
    check(equality, "Differing sizes, same top", two, oneTwo);
    check(equality,
          "Differing least element, same top and size",
          oneTwo,
          make_by_pushing<queue_t>({0, 2}));
    check(equivalence, "Wrong element", one, std::vector<int>{2});
    check(equivalence, "Wrong size", one, std::vector<int>{1, 2});
    check(equivalence, "Elements in the order pushed", oneTwo, std::vector<int>{1, 2});
  }
}
