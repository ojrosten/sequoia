////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2025.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "RatioFreeTest.hpp"
#include "sequoia/Maths/Algebra/Ratio.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path ratio_free_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void ratio_free_test::run_tests()
  {
    STATIC_CHECK(maths::ratio<1, 2>::num == 1);
    STATIC_CHECK(maths::ratio<1, 2>::den == 2);

    STATIC_CHECK(maths::ratio<2, 4>::num == 1);
    STATIC_CHECK(maths::ratio<2, 4>::den == 2);

    STATIC_CHECK(maths::ratio<4, 2>::num == 2);
    STATIC_CHECK(maths::ratio<4, 2>::den == 1);
    
    STATIC_CHECK(maths::ratio<1.1L, 2>::num == 1.1L);
    STATIC_CHECK(maths::ratio<1.1L, 2>::den == 2);

    STATIC_CHECK(maths::ratio<2, 1.1L>::num == 2);
    STATIC_CHECK(maths::ratio<2, 1.1L>::den == 1.1L);
    
    STATIC_CHECK(maths::ratio<1.1L, 2.1L>::num == 1.1L);
    STATIC_CHECK(maths::ratio<1.1L, 2.1L>::den == 2.1L);

    STATIC_CHECK(maths::ratio<1.1L, 1.1L>::num == 1.0L);
    STATIC_CHECK(maths::ratio<1.1L, 1.1L>::den == 1.0L);
    
    STATIC_CHECK(!std::same_as<maths::ratio<1, 1>, maths::ratio<1L, 1>>,
                 "This is an unfortunate consequence of ratio<intmax_t, intmax_t> being a specialization");

    STATIC_CHECK(maths::defines_ratio_v<maths::ratio<1, 2>>);
    STATIC_CHECK(maths::defines_ratio_v<std::ratio<1, 2>>);
    STATIC_CHECK(maths::defines_ratio_v<maths::ratio<1.1L, 2>>);
  }
}
