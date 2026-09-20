////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/** \file */

#include "QueueTestingDiagnostics.hpp"

namespace sequoia::testing
{
  using namespace data_structures;

  [[nodiscard]]
  std::filesystem::path queue_false_negative_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void queue_false_negative_test::run_tests()
  {
    const queue<int> empty{};
    const auto one{make_queue({1})}, two{make_queue({2})}, oneTwo{make_queue({1, 2})};

    check(equality, "Empty against one element", empty, one);
    check(equality, "Differing elements", one, two);
    check(equality, "Differing sizes, same front", one, oneTwo);
    check(equality, "Differing middle element, same front, back and size", make_queue({1, 2, 3}), make_queue({1, 5, 3}));
    check(equivalence, "Wrong element", one, std::vector<int>{2});
    check(equivalence, "Wrong size", one, std::vector<int>{1, 2});
    check(equivalence, "Wrong back element", oneTwo, std::vector<int>{1, 5});

    // Differing sizes, same back
    auto popped{make_queue({1, 2})};
    popped.pop();
    check(equality, "Popped queue against one holding its history", popped, oneTwo);
  }
}
