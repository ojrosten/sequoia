////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "AffinePhysicalValueTest.hpp"

#include "../Maths/Geometry/GeometryTestingUtilities.hpp"

namespace sequoia::testing
{
  using namespace physics;

  namespace
  {
    struct tina_arena{};
    struct alice {};
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
    test_affine_quantity<si::position<float, 2, tina_arena, canonical_right_handed_basis<free_module_type_of_t<position_space<float, 2, tina_arena>>>, alice>>();
  }

  template<class Quantity>
  void affine_physical_value_test::test_affine_quantity()
  {
    using quantity_t  = Quantity;
    using delta_q_t   = quantity_t::displacement_type;
    using space_type  = quantity_t::space_type;

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

    using units_type  = quantity_t::units_type;
    using origin_type = quantity_t::origin_type;
    STATIC_CHECK(
      !defines_physical_value_v<
        dual<space_type>,
        dual<units_type>,
        canonical_right_handed_basis<free_module_type_of_t<dual<space_type>>>,
        dual<origin_type>,
        std::identity>
    );
  }
}
