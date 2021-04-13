////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "IndentFreeTest.hpp"
#include "sequoia/TextProcessing/Indent.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view indent_free_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void indent_free_test::run_tests()
  {
    test_indent();
    test_append_indented();
  }

  void indent_free_test::test_indent()
  {
    using namespace std::string_literals;
    check_equality(LINE("Null indent of empty string"), indent("", indentation{"  "}), ""s);
    check_equality(LINE("Indent a letter"), indent("a", indentation{"  "}), "  a"s);
    check_equality(LINE("Indent a sententce"), indent("The quick brown fox...", indentation{"  "}), "  The quick brown fox..."s);
  }

  void indent_free_test::test_append_indented()
  {
  }
}
