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
                       {.source_folder{"sequoia"}, .main_cpp{"TestChamber/TestChamberMain.cpp"}, .ancillary_main_cpps{{"TestAll/TestMain.cpp"}}, .common_includes{"TestCommon/TestIncludes.hpp"}}};

    runner.register_test<ratio_free_test>();
    runner.register_test<numeric_rings_meta_free_test>();
    runner.register_test<spaces_meta_free_test>();
    runner.register_test<bounds_free_test>();
    runner.register_test<validators_free_test>();
    runner.register_test<vector_coordinates_false_negative_test>();
    runner.register_test<vector_coordinates_test>();
    runner.register_test<complex_vector_coordinates_test>();
    runner.register_test<vector_polar_coordinates_test>();
    runner.register_test<affine_coordinates_false_negative_test>();
    runner.register_test<affine_coordinates_test>();
    runner.register_test<m_affine_coordinates_test>();
    runner.register_test<free_module_coordinates_test>();
    runner.register_test<partial_m_torsor_coordinates_test>();
    runner.register_test<absolute_coordinates_false_negative_test>();
    runner.register_test<absolute_coordinates_test>();
    runner.register_test<absolute_logarithmic_coordinates_test>();
    runner.register_test<space_ordering_meta_free_test>();
    runner.register_test<physical_value_meta_free_test>();
    runner.register_test<physical_value_false_negative_test>();
    runner.register_test<physical_value_conversions_free_test>();
    runner.register_test<absolute_physical_value_test>();
    runner.register_test<absolute_physical_value_compositions_test>();
    runner.register_test<unsafe_absolute_physical_value_test>();
    runner.register_test<unsafe_absolute_physical_value_compositions_test>();
    runner.register_test<affine_physical_value_test>();
    runner.register_test<convex_physical_value_test>();
    runner.register_test<vector_physical_value_test>();
    runner.register_test<vector_physical_value_compositions_test>();
    runner.register_test<mixed_physical_value_test>();
    runner.register_test<integral_physical_value_test>();
    runner.register_test<saturating_mul_free_test>();
    runner.register_test<saturating_add_free_test>();
    runner.register_test<vector_nonlinear_representations_free_test>();
    runner.register_test<arithmetic_casts_free_test>();

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

