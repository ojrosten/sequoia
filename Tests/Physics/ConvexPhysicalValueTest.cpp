////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "ConvexPhysicalValueTest.hpp"

#include "../Maths/Geometry/GeometryTestingUtilities.hpp"

#include <charconv>

namespace sequoia::testing
{ 
  using namespace physics;

  [[nodiscard]]
  std::filesystem::path convex_physical_value_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void convex_physical_value_test::run_tests()
  {
    test_exceptions<float>();
    test_exceptions<double>();

    test_convex_quantity<si::temperature_celsius<float>>();
    test_convex_quantity<si::temperature_celsius<double>>();
    test_convex_quantity<non_si::temperature_farenheight<float>>();
    test_convex_quantity<non_si::temperature_farenheight<double>>();

    test_celsius_conversions<float>();
    test_celsius_conversions<double>();

    test_farenheight_conversions<float>();
    test_farenheight_conversions<double>();
  }

  template<std::floating_point T>
  void convex_physical_value_test::test_exceptions()
  {
    check_exception_thrown<std::domain_error>(
      "",
      [](){ return si::temperature_celsius<T>{T(-273.16), si::units::celsius}; }
    );

    check_exception_thrown<std::domain_error>(
      "",
      [](){ return non_si::temperature_farenheight<T>{T(-460), non_si::units::farenheight}; },
      [](const project_paths&, std::string message) {
        if constexpr(std::same_as<T, double> && (sizeof(double) == sizeof(long double)))
        {
          if(message.size() < 2)
            return message;

          if(const auto pos{message.rfind(' ', message.size() - 2)}; pos != std::string::npos)
          {
            /* Homebrew issue means this doesn't link for llvm 21.1.3
            double val{};
            const auto[ptr,ec]{std::from_chars(message.data() + pos, message.data() + message.size(), val)};
            if(ec != std::errc{})
              throw std::runtime_error{"Unable to extract final double from error message:" +  message};
            */

            char* end{};
            const auto val{std::strtod(message.data() + pos, &end)};
            message.replace(pos + 1, message.size() - pos - 2, std::format("{:.2f}", val));
          }
        }

        return message;
      }
    );
  }

  template<class Quantity>
  void convex_physical_value_test::test_convex_quantity()
  {
    using quantity_t = Quantity;
    using delta_q_t  = quantity_t::displacement_type;
    using space_t    = quantity_t::space_type;
    using units_t    = quantity_t::units_type;

    STATIC_CHECK(convex_space<space_t>);
    STATIC_CHECK(vector_space<free_module_type_of_t<space_t>>);
    STATIC_CHECK(!can_multiply<quantity_t, float>);
    STATIC_CHECK(!can_multiply<quantity_t, quantity_t>);
    STATIC_CHECK(!can_divide<quantity_t, float>);
    STATIC_CHECK(!can_divide<quantity_t, quantity_t>);
    STATIC_CHECK(!can_divide<quantity_t, delta_q_t>);
    STATIC_CHECK(!can_divide<delta_q_t, quantity_t>);
    STATIC_CHECK(can_divide<delta_q_t, delta_q_t>);
    STATIC_CHECK(!can_add<quantity_t, quantity_t>);
    STATIC_CHECK(can_add<quantity_t, delta_q_t>);
    STATIC_CHECK(can_subtract<quantity_t, quantity_t>);
    STATIC_CHECK(can_subtract<quantity_t, delta_q_t>);
    STATIC_CHECK(has_unary_plus<quantity_t>);
    STATIC_CHECK(!has_unary_minus<quantity_t>);
    STATIC_CHECK(
      !defines_physical_value_v<
        dual<space_t>,
        dual<units_t>,
        canonical_right_handed_basis<free_module_type_of_t<dual<space_t>>>,
        to_origin_type_t<dual<space_t>,
        dual<units_t>>,
        typename dual<units_t>::validator_type
      >
    );

    coordinates_operations<quantity_t>{*this}.execute();
  }

  template<std::floating_point T>
  void convex_physical_value_test::test_celsius_conversions()
  {
    using quantity_t = si::temperature_celsius<T>;
    using delta_q_t  = quantity_t::displacement_type;
    using value_t    = T;
    
    STATIC_CHECK(has_quantity_conversion_v<si::temperature<value_t>, quantity_t>);
    STATIC_CHECK(has_quantity_conversion_v<quantity_t, si::temperature<value_t>>);
    STATIC_CHECK(
      std::same_as<
        root_transform_t<si::units::celsius_t>,
        coordinate_transform<si::units::kelvin_t, dilatation<std::ratio<1, 1>>, translation<-273.15L>>
      >
    );
    STATIC_CHECK(!has_quantity_conversion_v<quantity_t, si::mass<value_t>>);

    using absolute_temp_t       = si::temperature<value_t>;
    using delta_absolute_temp_t = absolute_temp_t::displacement_type;

    STATIC_CHECK(not noexcept(absolute_temp_t{}.convert_to(si::units::celsius)));
      
    check(
      equality,
      "",
      absolute_temp_t{}.convert_to(si::units::celsius),
      quantity_t{value_t(-273.15), si::units::celsius}
    );

    check(
      equality,
      "",
      delta_absolute_temp_t{}.convert_to(si::units::celsius),
      delta_q_t{0, si::units::celsius}
    );

    check(
      equality,
      "",
      quantity_t{}.convert_to(si::units::kelvin),
      absolute_temp_t{value_t(273.15), si::units::kelvin}
    );

    check(
      equality,
      "",
      delta_q_t{}.convert_to(si::units::kelvin),
      delta_absolute_temp_t{0, si::units::kelvin}
    );
  }

  template<std::floating_point T>
  void convex_physical_value_test::test_farenheight_conversions()
  {
    using farenheight_t = non_si::temperature_farenheight<T>;
    using delta_faren_t = farenheight_t::displacement_type;
    using value_t       = T;
    using farenheight_wrt_t = non_si::units::farenheight_t::with_respect_to_type;

    STATIC_CHECK(   derives_from_another_unit_v<non_si::units::farenheight_t>
                 && derives_from_another_unit_v<farenheight_wrt_t>);

    STATIC_CHECK(   derives_from_another_unit_v<si::units::celsius_t>
                 && !derives_from_another_unit_v<typename si::units::celsius_t::with_respect_to_type>);
    STATIC_CHECK(std::same_as<root_transform_unit_t<non_si::units::farenheight_t>, si::units::kelvin_t>);

    using farenheight_transform_t = non_si::units::farenheight_t::transform_type;
    using farenheight_nested_transform_t = typename root_transform<non_si::units::farenheight_t>::nested_transform_type;
      
    STATIC_CHECK(std::same_as<root_transform<non_si::units::farenheight_t>::wrt_type, si::units::celsius_t>);
    STATIC_CHECK(
      std::same_as<
        farenheight_transform_t,
        coordinate_transform<si::units::celsius_t, dilatation<std::ratio<9, 5>>, translation<32.0L>>
      >
    );
    STATIC_CHECK(
      std::same_as<
        farenheight_nested_transform_t,
        coordinate_transform<si::units::kelvin_t, dilatation<std::ratio<1,1>>, translation<-273.15L>>
      >
    );

    STATIC_CHECK(
      std::same_as<
        root_transform_t<non_si::units::farenheight_t>,
        coordinate_transform<si::units::kelvin_t, dilatation<maths::ratio<std::intmax_t{9}, std::intmax_t{5}>>, translation<32.0L - 273.15L*9/5>>
      >
    );

    STATIC_CHECK(
      std::same_as<
        inverse_t<root_transform_t<non_si::units::farenheight_t>>,
      coordinate_transform<si::units::kelvin_t, dilatation<maths::ratio<std::intmax_t{5}, std::intmax_t{9}>>, translation<(273.15L*9/5 - 32.0L)*5/9>>
      >
    );

    STATIC_CHECK(has_quantity_conversion_v<si::temperature<value_t>, farenheight_t>);
    STATIC_CHECK(has_quantity_conversion_v<farenheight_t, si::temperature<value_t>>);
    STATIC_CHECK(!has_quantity_conversion_v<farenheight_t, si::mass<value_t>>);

    using absolute_temp_t       = si::temperature<value_t>;
    using delta_absolute_temp_t = absolute_temp_t::displacement_type;
    using celsius_t             = si::temperature_celsius<T>;
    using delta_celsius_t       = celsius_t::displacement_type;

    STATIC_CHECK(not noexcept(absolute_temp_t{}.convert_to(non_si::units::farenheight)));
   
    check(
      equality,
      "",
      absolute_temp_t{}.convert_to(non_si::units::farenheight),
      farenheight_t{value_t(-273.15L * 9 / 5 + 32), non_si::units::farenheight}
    );

    check(
      equality,
      "",
      farenheight_t{}.convert_to(si::units::kelvin),
      absolute_temp_t{value_t(273.15L - 160.0L/9), si::units::kelvin}
    );

    check(
      equality,
      "",
      celsius_t{100, si::units::celsius}.convert_to(non_si::units::farenheight),
      farenheight_t{value_t(212), non_si::units::farenheight}
    );

    check(
      equality,
      "",
      farenheight_t{212, non_si::units::farenheight}.convert_to(si::units::celsius),
      celsius_t{value_t(100), si::units::celsius}
    );

    check(
      equality,
      "",
      delta_absolute_temp_t{}.convert_to(non_si::units::farenheight),
      delta_faren_t{}
    );

    check(
      equality,
      "",
      delta_absolute_temp_t{10, si::units::kelvin}.convert_to(non_si::units::farenheight),
      delta_faren_t{value_t(18), non_si::units::farenheight}
    );

    check(
      equality,
      "",
      delta_faren_t{9, non_si::units::farenheight}.convert_to(si::units::kelvin),
      delta_absolute_temp_t{value_t{5}, si::units::kelvin}
    );

    check(
      equality,
      "",
      delta_faren_t{9, non_si::units::farenheight}.convert_to(si::units::celsius),
      delta_celsius_t{value_t(5), si::units::celsius}
    );
  }
}
