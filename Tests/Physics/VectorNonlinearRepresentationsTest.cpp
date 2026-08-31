////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/** \file */

#include "VectorNonlinearRepresentationsTest.hpp"
#include "sequoia/Physics/PhysicalValues.hpp"

#include "../Maths/Geometry/GeometryTestingUtilities.hpp"

namespace sequoia::testing
{
  using namespace physics;
  
  namespace
  {
    template<
      std::floating_point T,
      physical_unit UnitOfLength = si::units::metre_t,
      physical_unit UnitOfAngle  = si::units::radian_t
      // TO DO: separate bounds for radius and angle, which are propagated
    >
    struct physical_polar_representation
    {
      constexpr static auto bounds_v{no_bounds<T>};
      using length_unit_type = UnitOfLength;
      using angle_unit_type  = UnitOfAngle;

      using value_type = T;
      
      using radius_type = basic_quantity< radius_space<implicit_common_arena>, length_unit_type, value_type, throwing_validator>;
      using angle_type  = basic_quantity<angular_space<implicit_common_arena>, angle_unit_type , value_type, throwing_validator>;
      
      using coordinates_type = std::tuple<radius_type, angle_type>;

      using free_module_representation = physical_polar_representation;

      [[nodiscard]]
      constexpr static std::array<T, 2> to_underlying(std::span<const T, 2> polar)
      {
        const angle_type theta{polar[1], angle_unit_type{}};
        return {polar[0] * cos(theta), polar[0] * sin(theta)};
      }

      [[nodiscard]]
      constexpr static std::array<T, 2> from_underlying(std::span<const T, 2> cartesian)
      {
        angle<T> theta{(!cartesian[0] && !cartesian[1]) ? angle<T>{} : angle<T>{std::atan2(cartesian[1], cartesian[0]), si::units::radian}};
        if(theta < angle<T>{})
        {
          theta += T{2} * std::numbers::pi_v<T> * si::units::radian;
        }
        
        return {std::sqrt(cartesian[0] * cartesian[0] + cartesian[1] * cartesian[1]), theta.convert_to(angle_unit_type{}).value()};
      }
    };
  }

  [[nodiscard]]
  std::filesystem::path vector_nonlinear_representations_free_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void vector_nonlinear_representations_free_test::run_tests()
  {
    test_canonical_rep_meta();
    test_polar<float, si::units::radian_t>();
    test_polar<double, non_si::units::degree_t>();
  }

  void vector_nonlinear_representations_free_test::test_canonical_rep_meta()
  {
    using space_t = euclidean_vector_space<2>;
    
    using rep_t = canonical_representation<float, no_bounds<float>>;
    using vec_t
      = physical_value<
          space_t,
          si::units::metre_t,
          unit_defined_basis_data_for<space_t, si::units::metre_t>,
          rep_t,
          to_origin_type_t<space_t>,
          throwing_validator
        >;

    STATIC_CHECK(!has_heterogeneous_representation_v<rep_t>);
    STATIC_CHECK(!std::constructible_from<vec_t, length<float>, length<float>>);
  }
  
  template<std::floating_point T, physics::physical_unit AngleUnit>
  void vector_nonlinear_representations_free_test::test_polar()  
  {
    using space_t = euclidean_vector_space<2>;
    using rep_t = physical_polar_representation<T, si::units::metre_t, AngleUnit>;

    using vec_t
      = physical_value<
          space_t,
          si::units::metre_t,
          unit_defined_basis_data_for<space_t, si::units::metre_t>,
          rep_t,
          to_origin_type_t<space_t>,
          throwing_validator
        >;

    using delta_t = vec_t::displacement_type;
    using value_t = vec_t::value_type;

    STATIC_CHECK(free_module<free_module_type_of_t<space_t>>);
    STATIC_CHECK(has_coordinates_type_v<rep_t>);
    STATIC_CHECK(has_heterogeneous_representation_v<rep_t>);
    STATIC_CHECK(can_multiply<vec_t, value_t>);
    STATIC_CHECK(can_divide<vec_t, value_t>);
    STATIC_CHECK(!can_divide<vec_t, vec_t>);
    STATIC_CHECK(!can_divide<vec_t, delta_t>);
    STATIC_CHECK(!can_divide<delta_t, vec_t>);
    STATIC_CHECK(!can_divide<delta_t, delta_t>);
    STATIC_CHECK(can_add<vec_t, vec_t>);
    STATIC_CHECK(can_add<vec_t, delta_t>);
    STATIC_CHECK(can_subtract<vec_t, vec_t>);
    STATIC_CHECK(can_subtract<vec_t, delta_t>);
    STATIC_CHECK(has_unary_plus<vec_t>);
    STATIC_CHECK(has_unary_minus<vec_t>);

    using radius_t = rep_t::radius_type;
    using angle_t  = rep_t::angle_type;
    STATIC_CHECK(!std::constructible_from<vec_t, radius_t, radius_t>);
    STATIC_CHECK( std::constructible_from<vec_t, radius_t, angle_t>);

    coordinates_operations<vec_t>{*this}.execute();
  }
}
