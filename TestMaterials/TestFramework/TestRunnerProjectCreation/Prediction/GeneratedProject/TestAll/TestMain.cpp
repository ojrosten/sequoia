////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2021.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/TestFramework/TestRunner.hpp"

#include <iostream>

int main(int argc, char** argv)
{
  using namespace sequoia;
  using namespace testing;

  try
  {
    const auto root{project_root(argc, argv)};
    test_runner runner{argc,
                       argv,
                       "Oliver Jacob Rosten",
                       root/"TestAll"/"TestMain.cpp",
                       root/"TestAll"/"TestMain.cpp",
                       project_paths{root}
    };
 
    runner.execute();
  }
  catch(const std::exception& e)
  {
    std::cout << e.what();
  }
  catch(...)
  {
    std::cout << "Unrecognized error\n"; 
  }

  return 0;
}

