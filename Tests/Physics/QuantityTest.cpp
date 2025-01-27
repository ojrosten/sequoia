////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "QuantityTest.hpp"

#include "../Maths/Geometry/GeometryTestingUtilities.hpp"

namespace sequoia::testing
{ 
  using namespace physics;

  namespace{
    template<class T>
    inline constexpr bool has_unary_minus{
      requires(T t){ { -t } -> std::same_as<T>; }
    };
  }

  [[nodiscard]]
  std::filesystem::path quantity_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void quantity_test::run_tests()
  {
    test_masses();
  }

  void quantity_test::test_masses()
  {
    static_assert(convex_space<mass_space<float>>);
    static_assert(!has_unary_minus<mass_space<float>>);

    {
      using mass_t    = si::mass<float>;
      using delta_m_t = mass_t::displacement_quantity_type;
      static_assert(vector_space<delta_m_t>);
      static_assert(mass_t::is_intrinsically_absolute);
            
      check_exception_thrown<std::domain_error>("Negative mass", [](){ return mass_t{-1.0, units::kilogram}; });

      coordinates_operations<mass_t>{*this}.execute();

      check(equality, "", mass_t{2.0, units::kilogram} / delta_m_t{1.0, units::kilogram}, 2.0f);
      check(equality, "", delta_m_t{2.0, units::kilogram} / mass_t{1.0, units::kilogram}, 2.0f);
    }

    {
      using unsafe_mass_t = quantity<mass_space<float>, units::kilogram_t, std::identity>;

      coordinates_operations<unsafe_mass_t>{*this}.execute();
    }

    {
      using mass_t   = si::mass<float>;
      using length_t = si::length<float>;

      static_assert(convex_space<mass_space<float>>);
      static_assert(convex_space<length_space<float>>);
      static_assert(convex_space<direct_product<mass_space<float>, length_space<float>>>);
      static_assert(vector_space<displacement_space<classical_quantity_sets::masses, float>>);
      static_assert(vector_space<displacement_space<classical_quantity_sets::lengths, float>>);
      static_assert(vector_space<direct_product<displacement_space<classical_quantity_sets::masses, float>, displacement_space<classical_quantity_sets::lengths, float>>>);
      static_assert(vector_space<reduction<direct_product<displacement_space<classical_quantity_sets::masses, float>, displacement_space<classical_quantity_sets::lengths, float>>>>);
      
      static_assert(convex_space<reduction<direct_product<mass_space<float>, length_space<float>>>>);
      auto ml = mass_t{1.0, units::kilogram} * length_t{2.0, units::metre},
           lm = length_t{2.0, units::metre} * mass_t{1.0, units::kilogram};
      check(equivalence, "", ml, 2.0f);
      check(equivalence, "", lm, 2.0f);
    }
  }
}
