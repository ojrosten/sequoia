////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/** \file */

#include "AffinePhysicalValueTest.hpp"

#include "../Maths/Geometry/GeometryTestingUtilities.hpp"

namespace sequoia::testing
{
  using namespace physics;

  namespace
  {
    struct tina_arena{};
  }

  [[nodiscard]]
  std::filesystem::path affine_physical_value_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void affine_physical_value_test::run_tests()
  {
    test_affine_quantity<si::time<float>>();
    test_affine_quantity<si::time<double>>();
    test_affine_quantity<si::position<float, 1>>();
    test_affine_quantity<si::position<float, 2>>();
    test_affine_quantity<si::position<float, 2, tina_arena, alice>>();
  }

  template<class Quantity>
  void affine_physical_value_test::test_affine_quantity()
  {
    using quantity_t  = Quantity;
    using delta_q_t   = quantity_t::displacement_type;
    using space_type  = quantity_t::space_type;
    using repr_t      = quantity_t::representation_type;

    STATIC_CHECK(affine_space<space_type>);
    STATIC_CHECK(vector_space<free_module_type_of_t<space_type>>);
    STATIC_CHECK(!can_multiply<quantity_t, float>);
    STATIC_CHECK(!can_divide<quantity_t, float>);
    STATIC_CHECK(!can_divide<quantity_t, quantity_t>);
    STATIC_CHECK(!can_divide<quantity_t, delta_q_t>);
    STATIC_CHECK(!can_divide<delta_q_t, quantity_t>);
    STATIC_CHECK(has_unary_plus<quantity_t>);
    STATIC_CHECK(!has_unary_minus<quantity_t>);

    if constexpr(quantity_t::dimension == 1)
    {
      STATIC_CHECK(can_divide<delta_q_t, delta_q_t>);
    }
    STATIC_CHECK(!can_add<quantity_t, quantity_t>);
    STATIC_CHECK(can_add<quantity_t, delta_q_t>);
    STATIC_CHECK(can_subtract<quantity_t, quantity_t>);
    STATIC_CHECK(can_subtract<quantity_t, delta_q_t>);

    coordinates_operations<quantity_t>{*this}.execute();

    using units_type      = quantity_t::units_type;
    using origin_type     = quantity_t::origin_type;
    using basis_data_type = quantity_t::basis_data_type;
    using validator_type  = quantity_t::validator_type;

    // Why the dual is inadmissible, stated directly rather than left to be
    // inferred from a failure to compile.
    STATIC_CHECK(!has_distinguished_origin_v<space_type>);
    STATIC_CHECK(!permissible_value_space_v<dual<space_type>>);

    // Positive control. Without it the negative check below would pass just as
    // readily if one of its arguments were merely wrong, rather than the space
    // being inadmissible.
    STATIC_CHECK(
      defines_physical_value_v<space_type, units_type, basis_data_type, repr_t, origin_type, validator_type>);

    STATIC_CHECK(
      !defines_physical_value_v<
        dual<space_type>,
        dual<units_type>,
        unit_defined_basis_data_for<dual<space_type>, dual<units_type>>,
        repr_t,
        dual<origin_type>,
        validator_type
      >
    );
  }
}
