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

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path streaming_free_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void streaming_free_test::run_tests()
  {
    read_modify_write(working_materials() /= "Foo.txt", [](std::string& s) { capitalize(s);  });

    check(equivalence, report(""), working_materials() /= "Foo.txt", predictive_materials() /= "Foo.txt");
  }
}
