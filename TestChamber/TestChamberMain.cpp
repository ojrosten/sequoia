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
  try
  {
    using namespace sequoia;
    using namespace testing;
    using namespace object;
    using namespace std::literals::chrono_literals;

    test_runner runner{argc,
                       argv,
                       "Oliver J. Rosten",
                       "  ",
                       {.main_cpp{"TestChamber/TestChamberMain.cpp"}, .common_includes{"TestCommon/TestIncludes.hpp"}}};

    runner.add_test_suite(
      "Geometry",
      suite{
        "Spaces",        
        spaces_meta_free_test{"Spaces Meta Free Test"}
      },
      suite{
        "Vector Coordinates",
        vector_coordinates_false_negative_test{"False negative Test"},
        vector_coordinates_test{"Unit Test"}
      },
      suite{
        "Affine Coordinates",
        affine_coordinates_false_negative_test{"False negative Test"},
        affine_coordinates_test{"Unit Test"}
      }
    );

    runner.add_test_suite(
      "Physical Values",
      physical_value_meta_free_test{"Physical Value Meta Free Test"},
      physical_value_false_negative_test{"False Negative Test"},
      absolute_physical_value_test{"Absolute Physical Value Test"},
      unsafe_absolute_physical_value_test{"Unsafe Absolute Physical Value Test"},
      affine_physical_value_test{"Affine Physical Value Test"},
      convex_physical_value_test{"Convex Physical Value Test"},
      vector_physical_value_test{"Vector Physical Value Test"},
      mixed_physical_value_test{"Mixed Physical Value Test"},
      integral_physical_value_test{"Integral Physical Value Test"}
    );

    runner.execute(timer_resolution{1ms});
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

