////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

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
        using validator_type = half_line_validator;
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
    test_integral_quantity<physical_value<length_space<std::size_t, graphics::texture_arena>, graphics::units::texel>>();
  }

  template<class Quantity>
  void integral_physical_value_test::test_integral_quantity()
  {
    coordinates_operations<Quantity>{*this}.execute();
  }
}
