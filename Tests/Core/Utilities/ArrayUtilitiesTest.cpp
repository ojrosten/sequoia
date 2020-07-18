////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestUtilities.hpp"
#include "ArrayUtilitiesTest.hpp"

#include "ArrayUtilities.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view array_utilities_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void array_utilities_test::run_tests()
  {
    using namespace utilities;

    {
      check_exception_thrown<std::logic_error>(LINE(""), [](){ return to_array<int,0>({1});});

      constexpr auto a{to_array<int, 0>({})};
      check_equality(LINE(""), a, {});

    }
    
    {
      check_exception_thrown<std::logic_error>(LINE(""), [](){ return to_array<int,5>({0,1,2,3,4,5});});

      constexpr auto a{to_array<int, 5>({3, 6, 9, 1, 0})};
      check_equality(LINE(""), a, {3, 6, 9, 1, 0});
    }

    {
      check_exception_thrown<std::logic_error>(LINE(""), [](){ return to_array<int,3>({0,1,2,3});});

      constexpr auto a{to_array<int, 3>({1, 2, 3}, [](const int e) { return e * 3; })};
      check_equality(LINE(""), a, {3, 6, 9});
    }

    {
      using ndc_t = no_default_constructor;

      auto converter{
        [](int i){ return no_default_constructor{i}; }
      };

      check_exception_thrown<std::logic_error>(LINE(""), [converter](){ return to_array<ndc_t,2>({0,1,2}, converter);});

      constexpr std::array<no_default_constructor, 2>
        a{to_array<ndc_t, 2>({2, 3}, converter)};

      check_equality(LINE(""), a, {ndc_t{2}, ndc_t{3}});
    }
  }
}
