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
  template<class T, class U, template<class, class> class Compare>
  struct lower_bound;

  template<class T, class U, template<class, class> class Compare>
  inline constexpr auto lower_bound_v{lower_bound<T, U, Compare>::value};

  template<class... Ts, class U, template<class, class> class Compare>
  struct lower_bound<std::tuple<Ts...>, U, Compare>
  {
    constexpr static std::size_t N{sizeof...(Ts)};

    template<std::size_t Lower, std::size_t Upper>
      requires (Lower <= Upper) && (Upper <= N)
    constexpr static std::size_t get() noexcept {      
      if constexpr (Lower == Upper)
        return Lower;
      else
      {
        constexpr auto partition{(Lower + Upper) / 2};
        if constexpr(Compare<std::tuple_element_t<partition, std::tuple<Ts...>>, U>::value)
          return get<partition + 1, Upper>();
        else
          return get<Lower, partition>();
      }
    }

    constexpr static std::size_t value{get<0, N>()};
  };

  template<class T, class U, template<class, class> class Compare>
  struct merge;

  template<class T, class U, template<class, class> class Compare>
  using merge_t = merge<T, U, Compare>::type;

  template<class... Ts,  template<class, class> class Compare>
  struct merge<std::tuple<Ts...>, std::tuple<>, Compare>
  {
    using type = std::tuple<Ts...>;
  };

  template<class... Ts, template<class, class> class Compare>
    requires (sizeof...(Ts) > 0)
  struct merge<std::tuple<>, std::tuple<Ts...>, Compare>
  {
    using type = std::tuple<Ts...>;
  };

  template<class T, class U, template<class, class> class Compare>
  struct merge<std::tuple<T>, std::tuple<U>, Compare>
  {
    using type = std::tuple<U, T>;
  };

  template<class T, class U, template<class, class> class Compare>
  requires (Compare<T, U>::value)
  struct merge<std::tuple<T>, std::tuple<U>, Compare>
  {
    using type = std::tuple<T, U>;
  };

  namespace impl
  {
    template<class...>
    struct merge_one;

    template<class T, class... Us, std::size_t... Is, std::size_t... Js>
    struct merge_one<std::index_sequence<Is...>, std::index_sequence<Js...>, T, Us...>
    {
      using type = std::tuple<std::tuple_element_t<Is, std::tuple<Us...>>..., T, std::tuple_element_t<Js, std::tuple<Us...>>...>;
    };

    template<std::size_t N, class T>
    struct shift_sequence;

    template<std::size_t N, class T>
    using shift_sequence_t = shift_sequence<N, T>::type;

    template<std::size_t N, std::size_t... Is>
    struct shift_sequence<N, std::index_sequence<Is...>>
    {
      using type = std::index_sequence<N+Is...>;
    };
  }
  
  
  template<class T, class... Us, template<class, class> class Compare>
    requires (sizeof...(Us) > 1)
  struct merge<std::tuple<T>, std::tuple<Us...>, Compare>
  {
    constexpr static auto N{sizeof...(Us)};
    constexpr static auto Pos{lower_bound_v<std::tuple<Us...>, T, Compare>};
    using type = impl::merge_one<std::make_index_sequence<Pos>, impl::shift_sequence_t<Pos, std::make_index_sequence<N-Pos>>, T, Us...>::type;
  };

  template<class T, class... Ts, class... Us, template<class, class> class Compare>
    requires (sizeof...(Ts) > 0) && (sizeof...(Us) > 0)
  struct merge<std::tuple<T, Ts...>, std::tuple<Us...>, Compare>
  {
    // TO DO: make this more efficient
    using type = merge_t<std::tuple<Ts...>, merge_t<std::tuple<T>, std::tuple<Us...>, Compare>, Compare>;
  };
  
  template<class T, class U>
  struct comparator : std::bool_constant<sizeof(T) < sizeof(U)> {};  

  static_assert(lower_bound_v<std::tuple<>,     int, comparator> == 0);
  static_assert(lower_bound_v<std::tuple<int>,  int, comparator> == 0);
  static_assert(lower_bound_v<std::tuple<char>, int, comparator> == 1);
  static_assert(lower_bound_v<std::tuple<char, short, int>, char,   comparator> == 0);
  static_assert(lower_bound_v<std::tuple<char, short, int>, short,  comparator> == 1);
  static_assert(lower_bound_v<std::tuple<char, short, int>, int,    comparator> == 2);
  static_assert(lower_bound_v<std::tuple<char, short, int>, double, comparator> == 3);
  static_assert(lower_bound_v<std::tuple<char, char, short, short, int, int>, char,  comparator> == 0);
  static_assert(lower_bound_v<std::tuple<char, char, short, short, int, int>, short, comparator> == 2);
  static_assert(lower_bound_v<std::tuple<char, char, short, short, int, int>, int,   comparator> == 4);

  static_assert(std::is_same_v<merge_t<std::tuple<>, std::tuple<>, comparator>, std::tuple<>>);
  static_assert(std::is_same_v<merge_t<std::tuple<>, std::tuple<int>, comparator>, std::tuple<int>>);
  static_assert(std::is_same_v<merge_t<std::tuple<int>, std::tuple<>, comparator>, std::tuple<int>>);
  static_assert(std::is_same_v<merge_t<std::tuple<int>, std::tuple<int>,  comparator>, std::tuple<int, int>>);
  static_assert(std::is_same_v<merge_t<std::tuple<char>, std::tuple<int>, comparator>, std::tuple<char, int>>);
  static_assert(std::is_same_v<merge_t<std::tuple<int>, std::tuple<char>, comparator>, std::tuple<char, int>>);
  static_assert(std::is_same_v<merge_t<std::tuple<char>, std::tuple<char, int>, comparator>, std::tuple<char, char, int>>);
  static_assert(std::is_same_v<merge_t<std::tuple<short>, std::tuple<char, int>, comparator>, std::tuple<char, short, int>>);
  static_assert(std::is_same_v<merge_t<std::tuple<double>, std::tuple<char, int>, comparator>, std::tuple<char, int, double>>);
  static_assert(std::is_same_v<merge_t<std::tuple<char, int>, std::tuple<char>, comparator>, std::tuple<char, char, int>>);
  static_assert(std::is_same_v<merge_t<std::tuple<char, int>, std::tuple<char, short>, comparator>, std::tuple<char, char, short, int>>);

  static_assert(std::is_same_v<merge_t<std::tuple<short, int>, std::tuple<char, short, double>, comparator>, std::tuple<char, short, short, int, double>>);
  
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
      auto ml = mass_t{1.0, units::kilogram} * length_t{2.0, units::metre};
      check(equivalence, "", ml, 2.0f);
    }
  }
}
