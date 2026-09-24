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
    test_add_test_registrations();
    test_comparison_of_file_contents();
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

  /// Registrations are placed on lines of their own after the last existing one, or just before the
  /// call to `runner.execute` if there is none, and take the indentation of that neighbour, tabs
  /// included. The cases vary the neighbourhood of the insertion, from which both the placement
  /// and the indentation are read.
  void file_editors_free_test::test_add_test_registrations()
  {
    auto check_registration{
      [this](std::string_view description, const std::filesystem::path& main, const std::vector<std::string>& tests) {
        const auto file{working_materials() /= "Registration" / main};
        add_test_registrations(file, tests);

        check(equivalence, description, file, predictive_materials() /= "Registration" / main);
      }
    };

    check_registration("After the last registration, which a blank line separates from the execution",
                       "BlankLineBeforeExecution/Main.cpp",
                       {"gamma_test"});

    check_registration("After the last registration, which the execution immediately follows",
                       "NoBlankLine/Main.cpp",
                       {"gamma_test"});

    check_registration("Before the execution, with no registration to follow",
                       "NoRegistrations/Main.cpp",
                       {"gamma_test"});

    check_registration("Indented with tabs, as the registration it follows",
                       "TabIndentation/Main.cpp",
                       {"gamma_test"});

    check_registration("Several tests at once, skipping one already registered",
                       "SeveralTests/Main.cpp",
                       {"beta_test", "alpha_test", "gamma_test"});
  }

  /** The 0x1A checks are aimed at MSVC's text mode, which stops reading at that byte; POSIX text
      mode is binary mode, so there they cannot fail.
   */
  void file_editors_free_test::test_comparison_of_file_contents()
  {
    auto compares_equivalent{
      [dir{working_materials()}](std::string_view lhs, std::string_view rhs) {
        const transient_file a{dir / "ContentsUnderComparison.working", lhs}, b{dir / "ContentsUnderComparison.prediction", rhs};

        const auto contents{get_reduced_file_content(a.path(), b.path())};

        return contents.working.value() == contents.prediction.value();
      }
    };

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
}
