////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2025.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "SpacesMetaFreeTest.hpp"
#include "sequoia/Maths/Geometry/Spaces.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path spaces_meta_free_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void spaces_meta_free_test::run_tests()
  {
    using namespace maths;

    STATIC_CHECK(addable_v<int>);
    STATIC_CHECK(subtractable_v<int>);
    STATIC_CHECK(multiplicable_v<int>);
    STATIC_CHECK(divisible_v<int>);

    STATIC_CHECK(weakly_abelian_group_under_addition_v<int>);
    STATIC_CHECK(weakly_abelian_group_under_addition_v<double>);
    STATIC_CHECK(!weakly_abelian_group_under_multiplication_v<int>);
    STATIC_CHECK(weakly_abelian_group_under_multiplication_v<double>);
  }
}
