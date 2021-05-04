////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "FileSystemFreeTest.hpp"
#include "sequoia/TestFramework/FileSystem.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view file_system_free_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void file_system_free_test::run_tests()
  {
    test_find_in_tree();
  }

  void file_system_free_test::test_find_in_tree()
  {
    namespace fs = std::filesystem;

    const auto root{working_materials()}, fooPath{root / "Foo"};

    check_exception_thrown<std::runtime_error>(LINE(""),
                                               [&fooPath](){ return find_in_tree(fooPath / "Bar" / "baz.txt", "baz.txt"); });
    check_exception_thrown<std::runtime_error>(LINE(""),
                                               [&fooPath](){ return find_in_tree(fooPath / "Stuff", "baz.txt"); });

    check_equality(LINE("Not found"), find_in_tree(root, "thing.txt"), fs::path{});
    check_equality(LINE("Not found - empty"), find_in_tree(root, ""), fs::path{});
    check_equality(LINE("Unique file"), find_in_tree(root, "plurgh.txt"), fooPath / "Bar" / "plurgh.txt");
    check_equality(LINE("Unique directory"), find_in_tree(root, "Bar"), fooPath / "Bar");
    check_equality(LINE("Partial path"), find_in_tree(root, "Bar/plurgh.txt"), fooPath / "Bar" / "plurgh.txt");
    check_equality(LINE("Absolute path"), find_in_tree(root, fooPath), fooPath);
    check_equality(LINE("Absolute path: not found"), find_in_tree(root, fooPath / "Baz"), fs::path{});
  }
}
