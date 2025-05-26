////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "StreamingFreeTest.hpp"
#include "sequoia/Streaming/Streaming.hpp"
#include "sequoia/TextProcessing/Substitutions.hpp"

#include <fstream>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  [[nodiscard]]
  std::filesystem::path streaming_free_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void streaming_free_test::run_tests()
  {
    using namespace std::string_literals;

    check(equality, "", read_to_string(working_materials() /= "Foo.txt"), std::optional{"hello, World"s});
    check(equality, "", read_to_string(working_materials() /= "Bar.txt"), std::optional<std::string>{});

    check_exception_thrown<std::runtime_error>(
      reporter{""},
      [this]() { read_modify_write(working_materials() /= "Bar.txt", [](std::string& s) { capitalize(s);  }); });

    check_exception_thrown<std::runtime_error>(
      reporter{""},
      [this]() { write_to_file(working_materials() /= "Baz.txt", "Hello!", std::ios_base::noreplace); });

    read_modify_write(working_materials() /= "Foo.txt", [](std::string& s) { capitalize(s);  });
    check(equivalence, "", working_materials() /= "Foo.txt", predictive_materials() /= "Foo.txt");
  }
}
