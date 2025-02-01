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
      "Angle",
      angle_false_negative_test{"False negative Test"},
      angle_test{"Unit Test"}
    );

    runner.add_test_suite(
      "Geometry",
      suite{
        "Vec",
        vec_false_negative_test{"False negative Test"},
        vec_test{"Unit Test"}
      },
      suite{
        "Affine Coordinates",
        affine_coordinates_false_negative_test{"False negative Test"},
        affine_coordinates_test{"Unit Test"}
      }
    );

    runner.add_test_suite(
      "Quantity",
      quantity_meta_free_test{"Quantity Meta Free Test"},
      quantity_false_negative_test{"False Negative Test"},
      quantity_test{"Unit Test"}
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

