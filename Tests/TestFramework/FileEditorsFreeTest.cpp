////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "FileEditorsFreeTest.hpp"
#include "sequoia/TestFramework/FileEditors.hpp"
#include "Utilities/TestUtilities.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path file_editors_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void file_editors_free_test::run_tests()
  {
    test_add_include_without_an_existing_block();
    test_add_include_to_an_existing_block();
    test_comparison_of_file_contents();
    test_blank_lines_of_a_seqpat();
    test_line_numbers_of_a_seqpat();
  }

  /// A file with no `#include` anywhere has no block to extend. The include must
  /// still be placed ahead of the first `import`, for the reason given in
  /// add_include: a textual include after an import re-parses what the module
  /// already carried. Appending at the end of the file satisfies neither, and
  /// leaves whatever the header declares undeclared at every point of use.
  void file_editors_free_test::test_add_include_without_an_existing_block()
  {
    const auto file{working_materials() /= "NoBlock/Main.cpp"};
    add_include(file, "Stuff/FooTest.hpp");

    check(equivalence, "Include added to a file with no include block", file, predictive_materials() /= "NoBlock/Main.cpp");
  }

  /// The established behaviour, pinned so the case above cannot be fixed by
  /// breaking it: an existing block is replaced by the sorted union, angled
  /// before quoted.
  void file_editors_free_test::test_add_include_to_an_existing_block()
  {
    const auto file{working_materials() /= "ExistingBlock/Main.cpp"};
    add_include(file, "Stuff/FooTest.hpp");

    check(equivalence, "Include added to an existing include block", file, predictive_materials() /= "ExistingBlock/Main.cpp");
  }

  /** The 0x1A checks are aimed at MSVC's text mode, which stops reading at that byte; POSIX text
      mode is binary mode, so there they cannot fail.
   */
  void file_editors_free_test::test_comparison_of_file_contents()
  {
    using namespace std::string_view_literals;

    check("Text differing only in its line endings",   compares_equivalent("alpha\r\nbeta\r\n", "alpha\nbeta\n"));
    check("Text differing in more than that",         !compares_equivalent("alpha\r\nbeta\r\n", "alpha\nbeta\ngamma\n"));
    check("Text which is identical",                   compares_equivalent("alpha\nbeta\n", "alpha\nbeta\n"));

    check("A file with a null byte is compared byte for byte, line endings included",
          !compares_equivalent("\0alpha\r\n"sv, "\0alpha\n"sv));
    check("Two identical files with a null byte compare equivalent",
          compares_equivalent("\0alpha\r\n"sv, "\0alpha\r\n"sv));

    check("Two files differing only after a 0x1A byte",
          !compares_equivalent("head\x1A" "tail", "head\x1A" "different"));
    check("The same, for a file which is not text",
          !compares_equivalent("\0head\x1A" "tail"sv, "\0head\x1A" "different"sv));
  }

  void file_editors_free_test::test_blank_lines_of_a_seqpat()
  {
    const transient_file patterns{
      working_materials() /= "ContentsUnderComparison.seqpat",
      "alpha[0-9]\n\nbeta[0-9]\n  \ngamma[0-9]\n\t\ndelta[0-9]\n"
    };

    check("A pattern after an empty line, a line of spaces or a line holding only a tab is applied",
           compares_equivalent("alpha1 beta1 gamma1 delta1\n", "alpha2 beta2 gamma2 delta2\n"));
    check("A line of spaces masks no difference in indentation",
          !compares_equivalent("    indented\n", "  indented\n"));
    check("A line holding only a tab masks no difference in indentation",
          !compares_equivalent("\tindented\n", "indented\n"));
  }

  void file_editors_free_test::test_line_numbers_of_a_seqpat()
  {
    const transient_file patterns{working_materials() /= "ContentsUnderComparison.seqpat", "alpha[0-9]\n\n  \n(\n"};

    check_exception_thrown<std::runtime_error>(
      "An invalid pattern is reported by its line in the file, blank lines included",
      [this]() { return compares_equivalent("alpha1\n", "alpha2\n"); }
    );
  }

  [[nodiscard]]
  bool file_editors_free_test::compares_equivalent(std::string_view working, std::string_view prediction) const
  {
    const auto dir{working_materials()};
    const transient_file workingFile{dir / "ContentsUnderComparison.working", working},
                         predictionFile{dir / "ContentsUnderComparison.prediction", prediction};

    const auto contents{get_reduced_file_content(workingFile.path(), predictionFile.path())};

    return contents.working.value() == contents.prediction.value();
  }
}
