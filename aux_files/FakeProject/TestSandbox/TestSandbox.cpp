////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "ShareIncludes.hpp"

#include <iostream>

int main(int argc, char** argv)
{
  using namespace sequoia;
  using namespace testing;
  namespace fs = std::filesystem;

  try
  {    
    test_runner runner{argc,
                       argv,
                       "Oliver J. Rosten",
                       working_path().append("TestChamberMain.cpp"),
                       sibling_path("TestCommon").append("TestIncludes.hpp"),
                       sibling_path("Tests"),
                       sibling_path("Source")
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

