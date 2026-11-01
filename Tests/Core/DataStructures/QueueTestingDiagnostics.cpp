////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "QueueTestingDiagnostics.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path queue_false_negative_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void queue_false_negative_test::run_tests()
  {
    using queue_t = data_structures::queue<int>;

    const queue_t empty{};
    const auto one{make_by_pushing<queue_t>({1})},
               two{make_by_pushing<queue_t>({2})},
               oneTwo{make_by_pushing<queue_t>({1, 2})};

    check(equality, "Empty against one element", empty, one);
    check(equality, "Differing elements", one, two);
    check(equality, "Differing sizes, same front", one, oneTwo);
    check(equality,
          "Differing middle element, same front, back and size",
          make_by_pushing<queue_t>({1, 2, 3}),
          make_by_pushing<queue_t>({1, 5, 3}));
    check(equivalence, "Wrong element", one, std::vector<int>{2});
    check(equivalence, "Wrong size", one, std::vector<int>{1, 2});
    check(equivalence, "Elements back first", oneTwo, std::vector<int>{2, 1});

    auto popped{make_by_pushing<queue_t>({1, 2, 3})};
    popped.pop();
    check(equality,
          "A queue reached by popping against one holding the popped element",
          popped,
          make_by_pushing<queue_t>({1, 2, 3}));
  }
}
