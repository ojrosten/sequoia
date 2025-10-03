////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "AbsolutePhysicalValueTest.hpp"

#include "../Maths/Geometry/GeometryTestingUtilities.hpp"

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

  [[nodiscard]]
  std::filesystem::path absolute_physical_value_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void absolute_physical_value_test::run_tests()
  {
    test_absolute_quantity<si::mass<float>>();
    test_absolute_quantity<si::mass<double>>();
    test_absolute_quantity<si::length<float>>();
    test_absolute_quantity<si::time_interval<float>>();
    test_absolute_quantity<si::temperature<double>>();

    test_mass_conversions();
    test_length_conversions();
    test_area_conversions();
  }

  template<class Quantity>
  void absolute_physical_value_test::test_absolute_quantity()
  {
    using quantity_t = Quantity;
    using delta_q_t  = quantity_t::displacement_type;
    using space_type = quantity_t::space_type;
    using value_type = quantity_t::value_type;
    using units_type = quantity_t::units_type;

    STATIC_CHECK(convex_space<space_type>);
    STATIC_CHECK(vector_space<free_module_type_of_t<space_type>>);
    STATIC_CHECK(can_multiply<quantity_t, value_type>);
    STATIC_CHECK(can_divide<quantity_t, value_type>);
    STATIC_CHECK(can_divide<quantity_t, quantity_t>);
    STATIC_CHECK(can_divide<quantity_t, delta_q_t>);
    STATIC_CHECK(can_divide<delta_q_t, quantity_t>);
    STATIC_CHECK(can_divide<delta_q_t, delta_q_t>);
    STATIC_CHECK(can_add<quantity_t, quantity_t>);
    STATIC_CHECK(can_add<quantity_t, delta_q_t>);
    STATIC_CHECK(can_subtract<quantity_t, quantity_t>);
    STATIC_CHECK(can_subtract<quantity_t, delta_q_t>);
    STATIC_CHECK(has_unary_plus<quantity_t>);
    STATIC_CHECK(!has_unary_minus<quantity_t>);

    check_exception_thrown<std::domain_error>("Negative quantity", [](){ return quantity_t{-1.0, units_type{}}; });

    coordinates_operations<quantity_t>{*this}.execute();

    using inv_unit_t = dual<units_type>;

    using inv_quantity_t = quantity<dual<units_type>, value_type>;
    coordinates_operations<inv_quantity_t>{*this}.execute();

    test_compositions<quantity_t>();

    check(equality, "", (inv_quantity_t{2.0, inv_unit_t{}} * inv_quantity_t{3.0, inv_unit_t{}}) * quantity_t{2.0, units_type{}}, inv_quantity_t{12.0, inv_unit_t{}});

    using euc_vec_space_qty = euclidean_1d_vector_quantity<value_type>;

    check(equality, "", (quantity_t{4.0, units_type{}} *  quantity_t{3.0, units_type{}}  / delta_q_t{-2.0, units_type{}})  / quantity_t{2.0, units_type{}},  euc_vec_space_qty{-3.0, no_unit});
    check(equality, "", (quantity_t{4.0, units_type{}} *  quantity_t{3.0, units_type{}}) / (delta_q_t{-2.0, units_type{}}  * quantity_t{2.0, units_type{}}),  euc_vec_space_qty{-3.0, no_unit});
    check(equality, "", (quantity_t{4.0, units_type{}} *  quantity_t{3.0, units_type{}}) * ((1.0 / delta_q_t{-2.0, units_type{}})  * (1.0 / quantity_t{2.0, units_type{}})),  euc_vec_space_qty{-3.0, no_unit});      

    check(equality, "", (delta_q_t{4.0, units_type{}} *  delta_q_t{-3.0, units_type{}}  /  quantity_t{2.0, units_type{}})  / quantity_t{2.0, units_type{}},   euc_vec_space_qty{-3.0, no_unit});
    check(equality, "", (delta_q_t{4.0, units_type{}} *  delta_q_t{-3.0, units_type{}}) / (quantity_t{2.0, units_type{}}   * quantity_t{2.0, units_type{}}),  euc_vec_space_qty{-3.0, no_unit});
    check(equality, "",  delta_q_t{4.0, units_type{}} * (delta_q_t{-3.0, units_type{}}  / (quantity_t{2.0, units_type{}}   * quantity_t{2.0, units_type{}})), euc_vec_space_qty{-3.0, no_unit});
  }

  template<class Quantity>
  void absolute_physical_value_test::test_compositions()
  {
    using qty_t             = Quantity;
    using delta_qty_t       = qty_t::displacement_type;
    using value_t           = qty_t::value_type;
    using units_t           = qty_t::units_type;
    using inv_units_t       = dual<units_t>;
    using inv_qty_t         = quantity<inv_units_t, value_t>;
    using delta_inv_qty_t   = inv_qty_t::displacement_type;
    using euc_half_line_qty = euclidean_half_line_quantity<value_t>;
    using euc_vec_space_qty = euclidean_1d_vector_quantity<value_t>;
    using unsafe_qty_t      = quantity<units_t, value_t, std::identity>;
    using unsafe_inv_qty_t  = quantity<inv_units_t, value_t, std::identity>;
    using q2_t              = decltype(physical_value{value_t{}, units_t{} * units_t{}});
    using q3_t              = decltype(physical_value{value_t{}, units_t{} *units_t{} * units_t{}});

    using variant_t
      = std::variant<
          qty_t,
          delta_qty_t,
          inv_qty_t,
          delta_inv_qty_t,
          euc_half_line_qty,
          euc_vec_space_qty,
          unsafe_qty_t,
          unsafe_inv_qty_t,
          q2_t,
          q3_t
        >;
    using graph_type = transition_checker<variant_t>::transition_graph;
    using edge_t     = transition_checker<variant_t>::edge;

    enum qty_label { qty, dq, inv, dinvq, euc_half, euc_vec, unsafe, unsafe_inv, q2, q3 };
    
    graph_type g{
      {
        {
          // Start qty
          edge_t{
            qty_label::qty,
            this->report("Quantity left unaffected by attempting to turn it negative"),
            [this](variant_t v) -> variant_t {
              check_exception_thrown<std::domain_error>("Negative quantity", [&v](){ return std::get<qty_t>(v) += delta_qty_t{-2.0, units_t{}}; });
              return v;
            },
            std::weak_ordering::equivalent
          },
          edge_t{
            qty_label::inv,
            this->report("0.5 / qty"),
            [](variant_t v) -> variant_t { return value_t{2.0} / std::get<qty_t>(v); },
            std::weak_ordering::less
          },
          edge_t{
            qty_label::inv,
            this->report("qty / qty^2"),
            [](variant_t v) -> variant_t { return std::get<qty_t>(v) / physical_value{value_t{0.5}, units_t{} * units_t{}}; },
            std::weak_ordering::less
          },
          edge_t{
            qty_label::dinvq,
            this->report("qty / dq^2"),
            [](variant_t v) -> variant_t { return std::get<qty_t>(v) / (delta_qty_t{2.0, units_t{}} * delta_qty_t{2.0, units_t{}}); },
            std::weak_ordering::less
          },
          edge_t{
            qty_label::euc_half,
            this->report("qty / qty"),
            [](variant_t v) -> variant_t { return std::get<qty_t>(v) / qty_t{2.0, units_t{}}; },
            std::weak_ordering::less
          },
          edge_t{
            qty_label::euc_vec,
            this->report("qty / d_qty"),
            [](variant_t v) -> variant_t { return std::get<qty_t>(v) / delta_qty_t{2.0, units_t{}}; },
            std::weak_ordering::less
          },
          edge_t{
            qty_label::euc_vec,
            this->report("d_qty / qty"),
            [](variant_t v) -> variant_t { return delta_qty_t{0.5, units_t{}} / std::get<qty_t>(v); },
            std::weak_ordering::less
          },
          edge_t{
            qty_label::euc_half,
            this->report("qty * inv_qty"),
            [](variant_t v) -> variant_t { return std::get<qty_t>(v) * inv_qty_t{0.5, inv_units_t{}}; },
            std::weak_ordering::less
          },
          edge_t{
            qty_label::euc_half,
            this->report("inv_qty * qty"),
            [](variant_t v) -> variant_t { return inv_qty_t{0.5, inv_units_t{}} * std::get<qty_t>(v); },
            std::weak_ordering::less
          },
          edge_t{
            qty_label::euc_vec,
            this->report("qty * d_inv_qty"),
            [](variant_t v) -> variant_t { return std::get<qty_t>(v) * delta_inv_qty_t{0.5, inv_units_t{}}; },
            std::weak_ordering::less
          },
          edge_t{
            qty_label::euc_vec,
            this->report("d_inv_qty * qty"),
            [](variant_t v) -> variant_t { return delta_inv_qty_t{0.5, inv_units_t{}} * std::get<qty_t>(v); },
            std::weak_ordering::less
          },
          edge_t{
            qty_label::q2,
            this->report("qty * qty"),
            [](variant_t v) -> variant_t { return std::get<qty_t>(v) * qty_t{4.0, units_t{}}; },
            std::weak_ordering::less
          }
          // End qty
        },
        {
          // Start dq
          edge_t{
            qty_label::dinvq,
            this->report("0.125 / d_q"),
            [](variant_t v) -> variant_t { return value_t{0.125} / std::get<delta_qty_t>(v); },
            std::weak_ordering::less
          },
          edge_t{
            qty_label::euc_vec,
            this->report("d_qty / d_qty"),
            [](variant_t v) -> variant_t { return std::get<delta_qty_t>(v) / delta_qty_t{1.0, units_t{}}; },
            std::weak_ordering::less
          },
          edge_t{
            qty_label::euc_vec,
            this->report("d_qty * d_inv_qty"),
            [](variant_t v) -> variant_t { return std::get<delta_qty_t>(v) * delta_inv_qty_t{1.0, inv_units_t{}}; },
            std::weak_ordering::less
          },
          edge_t{
            qty_label::euc_vec,
            this->report(" d_inv_qty * d_qty"),
            [](variant_t v) -> variant_t { return delta_inv_qty_t{1.0, inv_units_t{}} * std::get<delta_qty_t>(v); },
            std::weak_ordering::less
          }
          // End dq
        },
        {
          // Start inv_q
          edge_t{
            qty_label::inv,
            this->report("Inv quantity left unaffected by attempting to turn it negative"),
            [this](variant_t v) -> variant_t {
              check_exception_thrown<std::domain_error>("Negative quantity", [&v](){ return std::get<inv_qty_t>(v) += delta_inv_qty_t{-3.0, inv_units_t{}}; });
              return v;
            },
            std::weak_ordering::equivalent
          },
          edge_t{
            qty_label::qty,
            this->report("2.0 / invq"),
            [](variant_t v) -> variant_t { return value_t{2.0} / std::get<inv_qty_t>(v); },
            std::weak_ordering::greater
          },
          edge_t{
            qty_label::euc_half,
            this->report("inv_qty / inv_qty"),
            [](variant_t v) -> variant_t { return std::get<inv_qty_t>(v) / inv_qty_t{4.0, inv_units_t{}}; },
            std::weak_ordering::less
          },
          edge_t{
            qty_label::euc_vec,
            this->report("inv_qty / d_inv_qty"),
            [](variant_t v) -> variant_t { return std::get<inv_qty_t>(v) / delta_inv_qty_t{4.0, inv_units_t{}}; },
            std::weak_ordering::less
          },
          edge_t{
            qty_label::euc_vec,
            this->report("d_inv_qty / inv_qty"),
            [](variant_t v) -> variant_t { return delta_inv_qty_t{1.0, inv_units_t{}} / std::get<inv_qty_t>(v); },
            std::weak_ordering::less
          },
          // End inv_q
        },
        { // Start d_inv_q
          edge_t{
            qty_label::dq,
            this->report("0.125 / dinvq"),
            [](variant_t v) -> variant_t { return value_t{0.125} / std::get<delta_inv_qty_t>(v); },
            std::weak_ordering::greater
          },
          edge_t{
            qty_label::euc_vec,
            this->report("d_inv_qty / d_inv_qty"),
            [](variant_t v) -> variant_t { return std::get<delta_inv_qty_t>(v) / delta_inv_qty_t{0.5, inv_units_t{}}; },
            std::weak_ordering::less
          },
          // Start d_inv_q
        },
        { // Start euc_half_line
          edge_t{
            qty_label::qty,
            this->report("euc_half * qty"),
            [](variant_t v) -> variant_t { return std::get<euc_half_line_qty>(v) * qty_t{2.0, units_t{}}; },
            std::weak_ordering::greater
          },
          edge_t{
            qty_label::qty,
            this->report("qty * euc_half"),
            [](variant_t v) -> variant_t { return qty_t{2.0, units_t{}} * std::get<euc_half_line_qty>(v); },
            std::weak_ordering::greater
          },
          edge_t{
            qty_label::inv,
            this->report("euc_half * inv_qty"),
            [](variant_t v) -> variant_t { return std::get<euc_half_line_qty>(v) * inv_qty_t{4.0, inv_units_t{}}; },
            std::weak_ordering::greater
          },
          edge_t{
            qty_label::inv,
            this->report("inv_qty * euc_half"),
            [](variant_t v) -> variant_t { return inv_qty_t{4.0, inv_units_t{}} * std::get<euc_half_line_qty>(v); },
            std::weak_ordering::greater
          }
          // End euc_half_line
        },
        {
          // Start euc_vec
          edge_t{
            qty_label::dq,
            this->report("euc_vec * d_qty"),
            [](variant_t v) -> variant_t { return std::get<euc_vec_space_qty>(v) * delta_qty_t{1.0, units_t{}}; },
            std::weak_ordering::greater
          },
          edge_t{
            qty_label::dq,
            this->report("d_qty * euc_vec"),
            [](variant_t v) -> variant_t { return delta_qty_t{1.0, units_t{}} * std::get<euc_vec_space_qty>(v); },
            std::weak_ordering::greater
          },
          edge_t{
            qty_label::dinvq,
            this->report("euc_vec * d_inv_qty"),
            [](variant_t v) -> variant_t { return std::get<euc_vec_space_qty>(v) * delta_inv_qty_t{0.5, inv_units_t{}}; },
            std::weak_ordering::greater
          },
          edge_t{
            qty_label::dinvq,
            this->report("d_inv_qty * euc_vec"),
            [](variant_t v) -> variant_t { return delta_inv_qty_t{0.5, inv_units_t{}} * std::get<euc_vec_space_qty>(v); },
            std::weak_ordering::greater
          },
          edge_t{
            qty_label::unsafe,
            this->report("euc_vec * qty"),
            [](variant_t v) -> variant_t { return -std::get<euc_vec_space_qty>(v) * qty_t{2.0, units_t{}}; },
            std::weak_ordering::less
          },
          edge_t{
            qty_label::unsafe_inv,
            this->report("euc_vec * inv_qty"),
            [](variant_t v) -> variant_t { return -std::get<euc_vec_space_qty>(v) * inv_qty_t{4.0, inv_units_t{}}; },
            std::weak_ordering::less
          },
          // End euc_vec
        },
        {
          // Start unsafe q
          // End unsafe q
        },
        {
          // Start unsafe inv q
          // End unsafe inv q
        },
        {
          // Start q^2
          edge_t{
            qty_label::qty,
            this->report("qty^2 / qty"),
            [](variant_t v) -> variant_t { return std::get<q2>(v) / qty_t{4.0, units_t{}}; },
            std::weak_ordering::greater
          },
          edge_t{
            qty_label::qty,
            this->report("qty^2 * inv_qty"),
            [](variant_t v) -> variant_t { return std::get<q2>(v) * inv_qty_t{0.25, inv_units_t{}}; },
            std::weak_ordering::greater
          },
          edge_t{
            qty_label::euc_half,
            this->report("qty^2 / qty^2"),
            [](variant_t v) -> variant_t { return std::get<q2>(v) / physical_value{value_t{8.0}, units_t{} * units_t{}}; },
            std::weak_ordering::greater
          },
          edge_t{
            qty_label::inv,
            this->report("qty^2 / qty^3"),
            [](variant_t v) -> variant_t { return std::get<q2>(v) / physical_value{value_t{2.0}, units_t{} * units_t{} * units_t{}}; },
            std::weak_ordering::greater
          },
          edge_t{
            qty_label::q3,
            this->report("qty^2 * qty"),
            [](variant_t v) -> variant_t { return std::get<q2>(v) * qty_t{2.0, units_t{}}; },
            std::weak_ordering::less
          },
          edge_t{
            qty_label::q3,
            this->report("qty * qty^2"),
            [](variant_t v) -> variant_t { return qty_t{2.0, units_t{}} * std::get<q2>(v); },
            std::weak_ordering::less
          },
          edge_t{
            qty_label::dq,
            this->report("qty^2 / dq"),
            [](variant_t v) -> variant_t { return std::get<q2>(v) / delta_qty_t{8.0, units_t{}}; },
            std::weak_ordering::greater
          },
          edge_t{
            qty_label::euc_vec,
            this->report("qty^2 / dq^2"),
            [](variant_t v) -> variant_t { return std::get<q2>(v) / (delta_qty_t{2.0, units_t{}} * delta_qty_t{4.0, units_t{}}); },
            std::weak_ordering::greater
          },
          // End q^2
        },
        {
          // Start q^3
          edge_t{
            qty_label::q2,
            this->report("qty^3 / qty"),
            [](variant_t v) -> variant_t { return std::get<q3>(v) / qty_t{2.0, units_t{}}; },
            std::weak_ordering::greater
          }
          // End q^3
        }
      },
      {
        variant_t{            qty_t{1.0, units_t{}}},                                 // qty
        variant_t{      delta_qty_t{0.5, units_t{}}},                                 // dq
        variant_t{        inv_qty_t{2.0, inv_units_t{}}},                             // inv
        variant_t{  delta_inv_qty_t{0.25, inv_units_t{}}},                            // dinvq
        variant_t{euc_half_line_qty{0.5, no_unit}},                                   // euc_half
        variant_t{euc_vec_space_qty{0.5, no_unit}},                                   // euc_vec 
        variant_t{     unsafe_qty_t{-1.0, units_t{}}},                                // unsafe
        variant_t{ unsafe_inv_qty_t{-2.0, inv_units_t{}}},                            // unsafe_inv
        variant_t{   physical_value{value_t{4.0}, units_t{} * units_t{}}},            // q2
        variant_t{   physical_value{value_t{8.0}, units_t{} * units_t{} * units_t{}}} // q3
      }
    };

    auto checker{
      [this](std::string_view description, const variant_t& obtained, const variant_t& prediction) {
        this->check(equality, description, obtained, prediction);
      }
    };
    
    transition_checker<variant_t>::check("", g, checker);
  }
  

  void absolute_physical_value_test::test_mass_conversions()
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

  void absolute_physical_value_test::test_length_conversions()
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

  void absolute_physical_value_test::test_area_conversions()
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
}
