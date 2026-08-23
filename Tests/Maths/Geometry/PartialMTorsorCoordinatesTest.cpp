////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "PartialMTorsorCoordinatesTest.hpp"

namespace sequoia::testing
{
  using namespace maths;

  namespace
  {
    /*! The non-negative orthant of a free module over the integers. The action of
        the module is only partial - subtracting a large enough displacement leaves
        the orthant - so this sits at the root of the DAG and nowhere below it. Note
        in particular that it is not convex: interpolation requires an ordered field,
        and the integers are merely an ordered ring.
     */
    template<class Set, class Ring, std::size_t D>
    struct my_partial_m_torsor
    {
      constexpr static std::size_t dimension{D};
      using set_type              = Set;
      using free_module_type      = my_module<Set, Ring, D>;
      using structure             = partial_m_torsor_tag_t;
      using distinguished_origin  = std::true_type;
      using non_negative_orthant  = std::true_type;
    };
  }

  [[nodiscard]]
  std::filesystem::path partial_m_torsor_coordinates_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void partial_m_torsor_coordinates_test::run_tests()
  {
    test_partial_m_torsor<sets::N_0<1>, commutative_rings::integers<1>, unsigned, 1>();
  }

  template<class Set, class Ring, class SetRep, std::size_t D>
  void partial_m_torsor_coordinates_test::test_partial_m_torsor()
  {
    using space_t  = my_partial_m_torsor<Set, Ring, D>;
    using basis_t  = general_basis<free_module_type_of_t<space_t>>;
    using coords_t = coordinates<space_t, basis_t, canonical_representation<SetRep, half_line_bounds<SetRep>>, throwing_validator>;

    STATIC_CHECK( partial_m_torsor<space_t>);
    STATIC_CHECK(!convex_space<space_t>);
    STATIC_CHECK(!m_affine_space<space_t>);
    STATIC_CHECK(!affine_space<space_t>);
    STATIC_CHECK(!free_module<space_t>);
    STATIC_CHECK( has_distinguished_origin_v<space_t>);
    STATIC_CHECK( is_non_negative_orthant_v<space_t>);

    using displacement_value_t = coords_t::displacement_coordinates_type::value_type;
    STATIC_CHECK(maths::weak_representation_for<displacement_value_t, Ring>);

    // A distinguished origin makes points addable, but the ring is not a field,
    // so neither points nor displacements may be divided by a scalar.
    operator_checks<coords_t, operator_expectations{
        .point_plus_point               = admits::yes,
        .point_plus_displacement        = admits::yes,
        .point_minus_point              = admits::yes,
        .point_minus_displacement       = admits::yes,
        .point_unary_plus               = admits::yes,
        .point_unary_minus              = admits::no,
        .point_times_scalar             = admits::yes,
        .point_over_scalar              = admits::no,
        .point_over_point               = admits::no,
        .point_over_displacement        = admits::no,
        .displacement_over_point        = admits::no,
        .displacement_times_scalar      = admits::yes,
        .displacement_over_scalar       = admits::no,
        .displacement_over_displacement = admits::no,
        .displacement_unary_minus       = admits::yes
      }
    >{*this}.execute();

    coordinates_operations<coords_t>{*this}.execute();
  }
}
