////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "ArrayUtilitiesTest.hpp"
#include "Utilities/TestUtilities.hpp"

#include "sequoia/Core/ContainerUtilities/ArrayUtilities.hpp"

namespace sequoia::testing
{  
  using namespace utilities;

  [[nodiscard]]
  std::filesystem::path array_utilities_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void array_utilities_test::run_tests()
  {
    test_to_array();
    test_make_array();
  }

  void array_utilities_test::test_to_array()
  {
    {
      check_exception_thrown<std::logic_error>("", [](){ return to_array<int,0>({1});});

      constexpr auto a{to_array<int, 0>({})};
      check(equality, "", a, {});

    }

    {
      check_exception_thrown<std::logic_error>("", [](){ return to_array<int,5>({0,1,2,3,4,5});});

      constexpr auto a{to_array<int, 5>({3, 6, 9, 1, 0})};
      check(equality, "", a, {3, 6, 9, 1, 0});
    }

    {
      check_exception_thrown<std::logic_error>("", [](){ return to_array<int,3>({0,1,2,3});});

      constexpr auto a{to_array<int, 3>({1, 2, 3}, [](const int e) { return e * 3; })};
      check(equality, "", a, {3, 6, 9});
    }

    {
      using ndc_t = no_default_constructor;

      auto converter{
        [](int i){ return no_default_constructor{i}; }
      };

      check_exception_thrown<std::logic_error>("", [converter](){ return to_array<ndc_t,2>({0,1,2}, converter);});

      constexpr std::array<no_default_constructor, 2>
        a{to_array<ndc_t, 2>({2, 3}, converter)};

      check(equality, "", a, {ndc_t{2}, ndc_t{3}});
    }
  }

  void array_utilities_test::test_make_array()
  {
    check(equality, "", make_array<int, 0>(std::identity{}), std::array<int, 0>{});
    check(equality, "", make_array<int, 1>([](std::size_t) { return 42;}), std::array<int, 1>{42});
    check(equality, "", make_array<int, 2>([](std::size_t i) { return 42*i;}), std::array<int, 2>{0, 42});
  }
}
