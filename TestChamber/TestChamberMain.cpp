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
                       {.main_cpp{"TestChamber/TestChamberMain.cpp"}, .ancillary_main_cpps{{"TestAll/TestMain.cpp"}}, .common_includes{"TestCommon/TestIncludes.hpp"}}};

    runner.add_test_suite(
      "Algebra",
      suite{
        "Ratio",
        ratio_free_test{"Ratio Free Test"}
      }
    );
    
    runner.add_test_suite(
      "Geometry",
      suite{
        "Spaces",        
        spaces_meta_free_test{"Spaces Meta Free Test"}
      },
      suite{
        "Bounds",
        bounds_free_test{"Bounds Free Test"}
      },
      suite{
        "Vector Coordinates",
        vector_coordinates_false_negative_test{"Vector Coordinates False negative Test"},
        vector_coordinates_test{"Vector Coordinates Test"},
        vector_polar_coordinates_test{"Vector Polar Coordinates Test"}
      },
      suite{
        "Affine Coordinates",
        affine_coordinates_false_negative_test{"Affine Coordinates False negative Test"},
        affine_coordinates_test{"Affine Coordinates Test"}
      },
      suite{
        "Free Module Coordinates",
        free_module_coordinates_test{"Free Module Test"}
      },
      suite{
        "Absolute Coordinates",
        absolute_coordinates_false_negative_test{"Absolute Coordinates False negative Test"},
        absolute_coordinates_test{"Absolute Coordinates Test"},
        absolute_logarithmic_coordinates_test{"Absolute Logarithmic Coordinates Test"}
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

    runner.add_test_suite(
      "Saturating Arithmetic",
      saturating_mul_free_test{"Saturating Mul Free Test"},
      saturating_add_free_test{"Saturating Add Free Test"}
    );

    runner.add_test_suite(
      "Vector_nonlinear_representations",
      vector_nonlinear_representations_free_test{"Vector Nonlinear Representations Free Test"}
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

