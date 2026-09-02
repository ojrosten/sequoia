////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/** \file */

#include "FileEditorsFreeTest.hpp"
#include "sequoia/TestFramework/FileEditors.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path file_editors_free_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void file_editors_free_test::run_tests()
  {
    test_add_include_without_an_existing_block();
    test_add_include_to_an_existing_block();
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
}
