////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/** \file */

#include "IntegralPhysicalValueTest.hpp"

#include "../Maths/Geometry/GeometryTestingUtilities.hpp"

namespace sequoia::testing
{ 
  using namespace physics;

  namespace graphics
  { 
    struct texture_arena {};

    namespace units
    {
      struct texel
      {
        using is_unit = std::true_type;
      };
    }
  }

  [[nodiscard]]
  std::filesystem::path integral_physical_value_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void integral_physical_value_test::run_tests()
  {
    test_integral_quantity<basic_quantity<length_space<graphics::texture_arena>, graphics::units::texel, unsigned, identity_validator>>();
  }

  template<class Quantity>
  void integral_physical_value_test::test_integral_quantity()
  {
    using quantity_t = Quantity;
    using delta_q_t  = quantity_t::displacement_type;
    using space_type = quantity_t::space_type;
    using value_type = quantity_t::value_type;

    check_static<(std::is_unsigned_v<value_type>)>();
    check_static<(std::is_unsigned_v<typename quantity_t::representation_type::value_type>)>();
    check_static<(convex_space<space_type>)>();
    check_static<(free_module<free_module_type_of_t<space_type>>)>();
    check_static<(can_multiply<quantity_t, value_type>)>();
    check_static<(!can_divide<quantity_t, value_type>)>();
    check_static<(!can_divide<quantity_t, quantity_t>)>();
    check_static<(!can_divide<quantity_t, delta_q_t>)>();
    check_static<(!can_divide<delta_q_t, quantity_t>)>();
    check_static<(!can_divide<delta_q_t, delta_q_t>)>();
    check_static<(can_add<quantity_t, quantity_t>)>();
    check_static<(can_add<quantity_t, delta_q_t>)>();
    check_static<(can_subtract<quantity_t, quantity_t>)>();
    check_static<(can_subtract<quantity_t, delta_q_t>)>();
    check_static<(has_unary_plus<quantity_t>)>();
    check_static<(!has_unary_minus<quantity_t>)>();
    
    coordinates_operations<Quantity>{*this}.execute();
  }
}
