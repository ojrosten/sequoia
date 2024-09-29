////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "FileSystemUtilitiesFreeTest.hpp"
#include "sequoia/TestFramework/FileSystemUtilities.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path file_system_utilities_free_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void file_system_utilities_free_test::run_tests()
  {
    test_find_in_tree();
    test_rebase_from();
  }

  void file_system_utilities_free_test::test_find_in_tree()
  {
    namespace fs = std::filesystem;

    const auto root{working_materials()}, fooPath{root / "Foo"};

    auto postprocessor{
      [](std::string mess) ->std::string {
        constexpr auto npos{std::string::npos};
        if (const auto end{mess.find("output")}; end != npos)
        {
          mess.erase(0, end);
        }

        return mess;
      }
    };

    check_exception_thrown<std::runtime_error>("",
                                               [&fooPath](){ return find_in_tree(fooPath / "Bar" / "baz.txt", "baz.txt"); },
                                               postprocessor);
    check_exception_thrown<std::runtime_error>("",
                                               [&fooPath](){ return find_in_tree(fooPath / "Stuff", "baz.txt"); },
                                               postprocessor);

    check(equality, "Not found", find_in_tree(root, "thing.txt"), fs::path{});
    check(equality, "Not found - empty", find_in_tree(root, ""), fs::path{});
    check(equality, "Unique file", find_in_tree(root, "plurgh.txt"), fooPath / "Bar" / "plurgh.txt");
    check(equality, "Unique directory", find_in_tree(root, "Bar"), fooPath / "Bar");
    check(equality, "Partial path", find_in_tree(root, "Bar/plurgh.txt"), fooPath / "Bar" / "plurgh.txt");
    check(equality, "Absolute path", find_in_tree(root, fooPath), fooPath);
    check(equality, "Absolute path: not found", find_in_tree(root, fooPath / "Baz"), fs::path{});
  }

  void file_system_utilities_free_test::test_rebase_from()
  {
    namespace fs = std::filesystem;

    check_exception_thrown<std::logic_error>("Attempt to rebase from file",
                    [this]() { return rebase_from("Foo/Bar", working_materials() /= "Foo/baz.txt"); });

    check(equality, "Non-existant path", rebase_from("Foo/Bar", "Baz"), fs::path{"Foo/Bar"});
    check(equality, "Rebase absolute", rebase_from(working_materials() /= "Foo", working_materials()), fs::path{"Foo"});
    check(equality, "No overlap", rebase_from(fs::path{"Things/Stuff.txt"}, working_materials()), fs::path{"Things/Stuff.txt"});
    check(equality, "Overlap", rebase_from(fs::path{"Foo/Stuff.txt"}, working_materials() /= "Foo"), fs::path{"Stuff.txt"});
    check(equality, "Relative", rebase_from(fs::path{"../Stuff.txt"}, working_materials()), fs::path{"Stuff.txt"});
    check(equality, "Double overlap", rebase_from(fs::path{"Foo/Bar/Stuff.txt"}, working_materials() /= "Foo/Bar"), fs::path{"Stuff.txt"});
  }
}
