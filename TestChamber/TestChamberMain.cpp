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
  auto code{sequoia::testing::return_code::incomplete_run};

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
        ratio_free_test{}
      }
    );
    
    runner.add_test_suite(
      "Geometry",
      suite{
        "Spaces",        
        numeric_rings_meta_free_test{},
        spaces_meta_free_test{}
      },
      suite{
        "Bounds",
        bounds_free_test{}
      },
      suite{
        "Validators",
        validators_free_test{}
      },
      suite{
        "Vector Coordinates",
        vector_coordinates_false_negative_test{},
        vector_coordinates_test{},
        complex_vector_coordinates_test{},
        vector_polar_coordinates_test{}
      },
      suite{
        "Affine Coordinates",
        affine_coordinates_false_negative_test{},
        affine_coordinates_test{}
      },
      suite{
        "M-Affine Coordinates",
        m_affine_coordinates_test{}
      },
      suite{
        "Free Module Coordinates",
        free_module_coordinates_test{}
      },
      suite{
        "Partial M-Torsor Coordinates",
        partial_m_torsor_coordinates_test{}
      },
      suite{
        "Absolute Coordinates",
        absolute_coordinates_false_negative_test{},
        absolute_coordinates_test{},
        absolute_logarithmic_coordinates_test{}
      }
    );

    runner.add_test_suite(
      "Physical Values",
      space_ordering_meta_free_test{},
      physical_value_meta_free_test{},
      physical_value_false_negative_test{},
      physical_value_conversions_free_test{},
      absolute_physical_value_test{},
      absolute_physical_value_compositions_test{},
      unsafe_absolute_physical_value_test{},
      unsafe_absolute_physical_value_compositions_test{},
      affine_physical_value_test{},
      convex_physical_value_test{},
      vector_physical_value_test{},
      vector_physical_value_compositions_test{},
      mixed_physical_value_test{},
      integral_physical_value_test{}
    );

    runner.add_test_suite(
      "Saturating Arithmetic",
      saturating_mul_free_test{},
      saturating_add_free_test{}
    );

    runner.add_test_suite(
      "Vector_nonlinear_representations",
      vector_nonlinear_representations_free_test{}
    );

    code = runner.execute(timer_resolution{1ms});
  }
  catch(const std::exception& e)
  {
    std::cout << e.what();
  }
  catch(...)
  {
    std::cout << "Unrecognized error\n"; 
  }
  
  return sequoia::testing::to_exit_code(code);
}

