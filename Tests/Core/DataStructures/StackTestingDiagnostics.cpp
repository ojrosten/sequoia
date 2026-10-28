////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "StackTestingDiagnostics.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path stack_false_negative_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void stack_false_negative_test::run_tests()
  {
    using stack_t = data_structures::stack<int>;

    const stack_t empty{};
    const auto one{make_by_pushing<stack_t>({1})},
               two{make_by_pushing<stack_t>({2})},
               oneTwo{make_by_pushing<stack_t>({1, 2})};

    check(equality, "Empty against one element", empty, one);
    check(equality, "Differing elements", one, two);
    check(equality, "Differing sizes, same top", two, oneTwo);
    check(equality, "Differing bottom element, same top and size", oneTwo, make_by_pushing<stack_t>({5, 2}));
    check(equivalence, "Wrong element", one, std::vector<int>{2});
    check(equivalence, "Wrong size", one, std::vector<int>{1, 2});
    check(equivalence, "Elements bottom first", oneTwo, std::vector<int>{1, 2});
  }
}
