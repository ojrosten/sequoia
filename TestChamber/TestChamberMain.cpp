////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestIncludes.hpp"

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
                       "Oliver J. Rosten",
                       root/"TestChamber"/"TestChamberMain.cpp",
                       root/"TestCommon" /"TestIncludes.hpp",
                       project_paths{root}
    };

    runner.add_test_family(
      "Experimental",
      experimental_test{"Unit Test"}
    );

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

