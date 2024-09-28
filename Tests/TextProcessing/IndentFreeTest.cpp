////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "IndentFreeTest.hpp"
#include "sequoia/TextProcessing/Indent.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path indent_free_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void indent_free_test::run_tests()
  {
    test_indent();
    test_append_indented();
  }

  void indent_free_test::test_indent()
  {
    using namespace std::string_literals;
    check(equality, report("Null indent of empty string"), indent("", indentation{"  "}), ""s);
    check(equality, report("Indent a letter"), indent("a", indentation{"  "}), "  a"s);
    check(equality, report("Indent a sententce"), indent("The quick brown fox...", indentation{"  "}), "  The quick brown fox..."s);

    check(equality, report("Indent multiple strings"), indent("a", "b", indentation{"  "}), "  a\n  b"s);
  }

  void indent_free_test::test_append_indented()
  {
    using namespace std::string_literals;
    check(equality, report("Append empty to empty"), append_indented("", "", indentation{" "}), ""s);
    check(equality, report("Append empty to non-empty"), append_indented("", "a", indentation{" "}), "a"s);
    check(equality, report("Append non-empty to empty"), append_indented("", "a", indentation{" "}), "a"s);
    check(equality, report("Append non-empty to non-empty"), append_indented("a", "b", indentation{" "}), "a\n b"s);

    check(equality, report("Append lines"), append_lines("a", "b"), "a\nb"s);
  }
}
