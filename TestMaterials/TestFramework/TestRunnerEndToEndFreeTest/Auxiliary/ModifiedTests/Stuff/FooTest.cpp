////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2021.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "FooTest.hpp"

#include "sequoia/TestFramework/FileEditors.hpp"
#include "sequoia/TextProcessing/Substitutions.hpp"

namespace generatedProject::testing
{
  [[nodiscard]]
  std::filesystem::path foo_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void foo_test::run_tests()
  {
    namespace fs = std::filesystem;
    using namespace sequoia;
    for(auto& e : fs::recursive_directory_iterator(working_materials()))
    {
      if(fs::is_regular_file(e))
      {
        if(auto contents{read_to_string(e.path())})
        {
          auto& text{contents.value()};
          if(!text.empty())
          {
            replace_all(text, "Old", "Updated");
            write_to_file(e.path(), text);
          }
        }
      }
    }

    const fs::path folder{"RepresentativeCases"};
    check(equivalence, "", working_materials() / folder, predictive_materials() / folder);
    check_exception_thrown<std::runtime_error>("", [](){ throw std::runtime_error{"A happy runtime error!"}; });
  }
}
