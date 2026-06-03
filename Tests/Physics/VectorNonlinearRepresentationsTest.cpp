////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "VectorNonlinearRepresentationsTest.hpp"
#include "sequoia/Physics/PhysicalValues.hpp"

namespace sequoia::testing
{
  using namespace physics;
  
  namespace
  {
    template<weak_commutative_ring T, auto Bounds=no_bounds<T>>
    struct physical_polar_representation : polar_representation<T, Bounds>
    {
      using radius_type = physical_value<radius_space<T, implicit_common_arena>, si::units::metre_t>;
      using angle_type = physical_value<angular_space<T, implicit_common_arena>, si::units::radian_t>;
      
      using coordinates_type = std::tuple<radius_type, angle_type>;
      
      template<auto OtherBounds>
      using rebind_type = physical_polar_representation<T, OtherBounds>;
    };
  }

  [[nodiscard]]
  std::filesystem::path vector_nonlinear_representations_free_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void vector_nonlinear_representations_free_test::run_tests()
  {
    using namespace si::units;

    using space_t = euclidean_vector_space<float, 2>;
    using unit_t  = metre_t;

    {
      using vec_t
        = physical_value<
            space_t,
            unit_t,
            unit_defined_right_handed_basis<free_module_type_of_t<space_t>, unit_t>,
            to_origin_type_t<space_t>,
            canonical_representation<no_bounds<float>>,
            throwing_validator
          >;

      STATIC_CHECK(!std::constructible_from<vec_t, length<float>, length<float>>);
    }

    {
      using rep_t = physical_polar_representation<float, no_bounds<float>>;
      using vec_t
        = physical_value<
            space_t,
            unit_t,
            unit_defined_right_handed_basis<free_module_type_of_t<space_t>, unit_t>,
            to_origin_type_t<space_t>,
            rep_t,
            throwing_validator
          >;

      using radius_t = rep_t::radius_type;

      vec_t v{radius_t{1.0f, metre}, physical_value{1.0f, radian}};
    }
  }
}
