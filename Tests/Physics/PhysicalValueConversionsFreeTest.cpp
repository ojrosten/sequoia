////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/** \file */

#include "PhysicalValueConversionsFreeTest.hpp"

import std;

namespace sequoia::testing
{
  using namespace physics;

  namespace
  {
    struct astronomical_unit_t : coordinate_transform<si::units::metre_t, dilatation<std::ratio<1, 149'597'870'700>>, translation<0>>
    {
      //constexpr static std::string_view symbol{"au"};
    };

    constexpr astronomical_unit_t astronomical_unit{};
  }

  namespace alternative
  {
    struct gradian_t : coordinate_transform<non_si::units::degree_t, dilatation<std::ratio<10, 9>>, translation<0>>
    {
      using validator_type = std::identity;
      constexpr static std::string_view symbol{"gon"};
    };

    constexpr gradian_t gradian{};
  }

  [[nodiscard]]
  std::filesystem::path physical_value_conversions_free_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void physical_value_conversions_free_test::run_tests()
  {
    test_mass_conversions();
    test_length_conversions();
    test_area_conversions();

    test_angle_conversions<float>();
    test_angle_conversions<double>();
    test_angle_conversions<long double>();
  }

  void physical_value_conversions_free_test::test_mass_conversions()
  {
    STATIC_CHECK(!noexcept(si::mass<float>{}.convert_to(si::units::tonne)));
    
    check(
      equality,
      "",
      si::mass<float>{1000.0, si::units::kilogram}.convert_to(si::units::tonne),
      physical_value{1.0f, si::units::tonne}
    );

    check(
      equality,
      "",
      si::mass<float>{1.0, si::units::kilogram}.convert_to(si::units::gram),
      physical_value{1000.0f, si::units::gram}
    );

    check(
      equality,
      "",
      physical_value{1.0f, si::units::tonne}.convert_to(si::units::kilogram),
      si::mass<float>{1000, si::units::kilogram}
    );

    check(
      equality,
      "",
      physical_value{1.0f, si::units::tonne}.convert_to(si::units::gram),
      physical_value{1'000'000.0f, si::units::gram}
    );

    check(
      equality,
      "",
      physical_value{1'000'000.0f, si::units::gram}.convert_to(si::units::tonne),
      physical_value{1.0f, si::units::tonne}
    );

    check(
      equality,
      "",
      si::mass<float>{1.0, si::units::kilogram}.convert_to(si::units::kilogram),
      si::mass<float>{1.0, si::units::kilogram}
    );

    check(
      equality,
      "",
      physical_value{1000.0f, si::units::tonne}.convert_to(si::units::kilotonne),
      physical_value{1.0f, si::units::kilotonne}
    );
  }

  void physical_value_conversions_free_test::test_length_conversions()
  {
    check(
      equality,
      "",
      si::length<float>{1.0, si::units::metre}.convert_to(non_si::units::foot),
      physical_value{3.2808399f, non_si::units::foot}
    );

    check(
      equality,
      "",
      physical_value{1.0f, non_si::units::foot}.convert_to(si::units::metre),
      si::length<float>{0.3048f, si::units::metre}
    );

    check(
      equality,
      "",
      physical_value{1.0, astronomical_unit}.convert_to(si::units::metre),
      si::length<double>{149'597'870'700.0, si::units::metre}
    );
  }

  void physical_value_conversions_free_test::test_area_conversions()
  {
    check(
      equality,
      "",
      physical_value{1.0f, si::units::metre * si::units::metre}.convert_to(non_si::units::foot * non_si::units::foot),
      physical_value{3.2808399f * 3.2808399f, non_si::units::foot * non_si::units::foot}
    );

    check(
      equality,
      "",
      physical_value{1.0, astronomical_unit * astronomical_unit}.convert_to(si::units::metre * si::units::metre),
      physical_value{149'597'870'700.0 * 149'597'870'700.0, si::units::metre * si::units::metre}
    );
  }

  template<std::floating_point T>
  void physical_value_conversions_free_test::test_angle_conversions()
  {
    STATIC_CHECK(std::same_as<root_transform_unit_t<alternative::gradian_t>, si::units::radian_t>);
    STATIC_CHECK(std::same_as<root_transform_unit_t<non_si::units::gradian_t>, si::units::radian_t>);
    STATIC_CHECK(std::same_as<root_transform_t<gradian_t>, coordinate_transform<si::units::radian_t, dilatation<ratio<std::intmax_t{200}, std::numbers::pi_v<long double>>>, translation<0>>>);
    STATIC_CHECK(std::same_as<root_transform_t<degree_t>, coordinate_transform<si::units::radian_t, dilatation<ratio<std::intmax_t{180}, std::numbers::pi_v<long double>>>, translation<0>>>);       
    STATIC_CHECK(std::same_as<root_transform_unit_t<milli<si::units::radian_t>>, si::units::radian_t>);
    STATIC_CHECK(std::same_as<root_transform_unit_t<milli<milli<si::units::radian_t>>>, si::units::radian_t>);

    using angle_t = si::angle<T>;
    using namespace si::units;
    constexpr auto pi{std::numbers::pi_v<long double>};

    check(
      equality,
      "Radians to Degrees",
      angle_t{1, si::units::radian}.convert_to(non_si::units::degree),
      physical_value{static_cast<T>(180 / pi), non_si::units::degree}
    );

    check(
      equality,
      "Degrees to Radians",
      physical_value{static_cast<T>(180 / pi), non_si::units::degree}.convert_to(si::units::radian),
      angle_t{1, si::units::radian}
    );

    check(
      equality,
      "Radians to Gradians",
      angle_t{1, si::units::radian}.convert_to(non_si::units::gradian),
      physical_value{static_cast<T>(std::intmax_t{200} / pi), non_si::units::gradian}
    );

    check(
      equality,
      "Gradians to Radians",
      physical_value{static_cast<T>(std::intmax_t{200} / pi), non_si::units::gradian}.convert_to(si::units::radian),
      physical_value{T(1), si::units::radian}
    );

    check(
      equality,
      "Degrees to Gradians",
      physical_value{T(360), non_si::units::degree}.convert_to(non_si::units::gradian),
      physical_value{T(400), non_si::units::gradian}  
    );

    check(
      equality,
      "Gradians to Degrees",
      physical_value{T(400), non_si::units::gradian}.convert_to(non_si::units::degree),
      physical_value{T(360), non_si::units::degree}  
    );

    check(
      equality,
      "Degrees to Gradians (not exactly representable as floating-point)",
      physical_value{T(1.1), non_si::units::degree}.convert_to(non_si::units::gradian),
      physical_value{T(1.1) * 10 / 9, non_si::units::gradian}  
    );

    check(
      equality,
      "Gradians to Degrees (not exactly representable as floating-point)",
      physical_value{T(1.1), non_si::units::gradian}.convert_to(non_si::units::degree),
      physical_value{T(1.1) * 9 / 10, non_si::units::degree}
    );

    check(
      equality,
      "Degrees to alternative::gradian",
      physical_value{T(360), non_si::units::degree}.convert_to(alternative::gradian),
      physical_value{T(400), alternative::gradian}  
    );

    check(
      equality,
      "alternative::Gradian to Degrees",
      physical_value{T(400), alternative::gradian}.convert_to(non_si::units::degree),
      physical_value{T(360), non_si::units::degree}  
    );
  }
}
