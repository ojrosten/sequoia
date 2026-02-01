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
  
  using namespace maths;

  namespace
  {
    struct distinguished_origin_space {
      using set_type             = sets::R<1>;
      using free_module_type     = euclidean_vector_space<float, 1>;
      using is_convex_space      = std::true_type;
      using distinguished_origin = std::true_type;
    };

    struct half_line_space {
      using set_type         = sets::R<1>;
      using free_module_type = euclidean_vector_space<float, 1>;
      using is_convex_space  = std::true_type;
      using half_line        = std::true_type;
    };

    struct unremarkable_space {
      using set_type         = sets::R<1>;
      using free_module_type = euclidean_vector_space<float, 1>;
      using is_convex_space  = std::true_type;
    };
  }
  
  [[nodiscard]]
  std::filesystem::path spaces_meta_free_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void spaces_meta_free_test::run_tests()
  {
    STATIC_CHECK(is_addable_v<int>);
    STATIC_CHECK(is_subtractable_v<int>);
    STATIC_CHECK(is_multiplicable_v<int>);
    STATIC_CHECK(is_divisible_v<int>);

    STATIC_CHECK(weakly_abelian_group_under_addition_v<int>);
    STATIC_CHECK(weakly_abelian_group_under_addition_v<std::size_t>);
    STATIC_CHECK(weakly_abelian_group_under_addition_v<float>);
    STATIC_CHECK(weakly_abelian_group_under_addition_v<double>);
    STATIC_CHECK(weakly_abelian_group_under_addition_v<std::complex<float>>);
    STATIC_CHECK(weakly_abelian_group_under_addition_v<std::complex<double>>);
    
    STATIC_CHECK(!weakly_abelian_group_under_multiplication_v<int>);
    STATIC_CHECK(!weakly_abelian_group_under_multiplication_v<std::size_t>);
    STATIC_CHECK(weakly_abelian_group_under_multiplication_v<float>);
    STATIC_CHECK(weakly_abelian_group_under_multiplication_v<double>);
    STATIC_CHECK(weakly_abelian_group_under_multiplication_v<std::complex<float>>);
    STATIC_CHECK(weakly_abelian_group_under_multiplication_v<std::complex<double>>);

    STATIC_CHECK(has_distinguished_origin_v<distinguished_origin_space>);
    STATIC_CHECK(!is_half_line_v<distinguished_origin_space>);

    STATIC_CHECK(has_distinguished_origin_v<half_line_space>);
    STATIC_CHECK(is_half_line_v<half_line_space>);

    STATIC_CHECK(!has_distinguished_origin_v<unremarkable_space>);
    STATIC_CHECK(!is_half_line_v<unremarkable_space>);
  }
}
