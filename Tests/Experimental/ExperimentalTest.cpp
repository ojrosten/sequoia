////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "ExperimentalTest.hpp"

#include <array>

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view experimental_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void experimental_test::run_tests()
  {
    {
      using array_t = std::array<int, 3>;
    
      constexpr auto a{
        [](){
          array_t a{42, 1, 4};
          experimental::stable_sort(a.begin(), a.end(), [](int lhs, int rhs){
            return lhs < rhs;
          });

          return a;
        }()
      };


      check(equality, LINE(""), a, array_t{1, 4, 42});
    }

    {
      using pair_t = std::pair<int, int>;
      using array_t = std::array<pair_t, 4>;
      
      constexpr auto a{
        [](){
          array_t a{pair_t{42,42}, pair_t{42, 1}, pair_t{42, 4}, pair_t{1, 100}};
          experimental::stable_sort(a.begin(), a.end(), [](const pair_t& lhs, const pair_t& rhs){
            return lhs.first < rhs.first;
            });

          return a;
        }()
      };


      check(equality, LINE(""), a, array_t{pair_t{1, 100}, pair_t{42,42}, pair_t{42, 1}, pair_t{42, 4}});
    }
  }
}
