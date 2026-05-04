////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "BoundsFreeTest.hpp"
#include "sequoia/Maths/Geometry/Spaces.hpp"

namespace sequoia::testing
{
  using namespace maths;
  template<arithmetic T>
  struct coordinate_bounds
  {
    using value_type = T;

    constexpr static T greatest_upper_bound{
      std::numeric_limits<T>::has_infinity ? std::numeric_limits<T>::infinity() : std::numeric_limits<T>::max()
    };

    constexpr static T least_lower_bound{
      std::numeric_limits<T>::has_infinity ? -std::numeric_limits<T>::infinity() : std::numeric_limits<T>::lowest()
    };

    T lower{}, upper{greatest_upper_bound};

    template<arithmetic U>
      requires initializable_from<T, U>
    [[nodiscard]]
    constexpr bool operator()(U val) const noexcept
    {
      if(lower > least_lower_bound)
      {
        if(const U uLower{static_cast<U>(lower)}; val < uLower)
          return false;
      }

      if(upper < greatest_upper_bound)
      {
        if(const U uUpper{static_cast<U>(upper)}; val > uUpper)
          return false;
      }

      return true;
    }

    template<arithmetic U, std::size_t D>
      requires initializable_from<T, U>
    [[nodiscard]]
    constexpr bool operator()(const std::array<U, D>& vals) const noexcept
    {
      auto v{std::views::transform(vals, [this](const U val) { return this->operator()(val); })};
      return !std::ranges::contains(v, false);
    }

    template<arithmetic U>
      requires initializable_from<T, U>
    [[nodiscard]]
    std::string format_input(const U val) const
    {
      return std::format("{}", val);
    }

    template<arithmetic U, std::size_t D>
      requires initializable_from<T, U>
    [[nodiscard]]
    std::string format_input(const std::array<U, D>& vals) const
    {
      return std::format("{} has at least one value", vals);
    }
  };

  template<std::floating_point T>
  struct annulus_bounds
  {
    using value_type = T;
    
    T lower{}, upper{};

    template<arithmetic U>
      requires initializable_from<T, U>
    [[nodiscard]]
    constexpr bool operator()(const std::array<U, 2>& vals) const noexcept
    {
      const T r{vals[0] * vals[0] + vals[1] * vals[1]};
      return (r >= lower*lower) && (r <= upper*upper);
    };
    
    template<arithmetic U, std::size_t D>
      requires initializable_from<T, U>
    [[nodiscard]]
    std::string format_input(const std::array<U, D>& vals) const
    {
      return std::format("{0} --> sqrt({1}*{1} + {2}*{2}) = {3}", vals, vals[0], vals[1], std::sqrt(vals[0] * vals[0] + vals[1] * vals[1]));
    }
  };

  template<auto Bounds>
  concept bounds
    =    has_value_type_v<decltype(Bounds)>
      && requires {
           { Bounds.lower } -> std::convertible_to<typename decltype(Bounds)::value_type>;
           { Bounds.upper } -> std::convertible_to<typename decltype(Bounds)::value_type>;
           requires (Bounds.lower < Bounds.upper);
         };

  template<auto Bounds>
  inline constexpr bool checks_single_val_against_bounds_v{
    requires {
      { Bounds(std::declval<typename decltype(Bounds)::value_type>()) } -> std::convertible_to<bool>;
    }
  };

  template<auto Bounds, std::size_t D>
  inline constexpr bool checks_array_against_bounds_v{
    requires {
      { Bounds(std::declval<std::array<typename decltype(Bounds)::value_type, D>>()) } -> std::convertible_to<bool>;
    }
  };

  template<auto Bounds, class ConvexSpace>
  concept bounds_for
    =      bounds<Bounds> && convex_space<ConvexSpace>
        && (   ((dimension_of<ConvexSpace> == 1) && checks_single_val_against_bounds_v<Bounds>)
            || ((dimension_of<ConvexSpace>  > 1) && checks_array_against_bounds_v<Bounds, dimension_of<ConvexSpace>>));
  
  namespace impl
  {
    template<auto Bounds, class T>
    struct range_of_bounds;

    template<auto Bounds, class T>
    inline constexpr bool range_of_bounds_v{range_of_bounds<Bounds, T>::value};

    template<auto Bounds, std::size_t... Is>
    struct range_of_bounds<Bounds, std::index_sequence<Is...>> : std::bool_constant<(bounds<Bounds[Is]> && ...)>
    {      
    };
  }

  template<auto Bounds>
  inline constexpr bool range_of_bounds_v{
       std::ranges::range<decltype(Bounds)>
    && requires {
         requires impl::range_of_bounds_v<Bounds, std::make_index_sequence<Bounds.size()>>;
       }
  };
  
  template<auto Bounds>
    requires bounds<Bounds> || range_of_bounds_v<Bounds>
  struct throwing_validator
  {
    using bounds_type = decltype(Bounds);
    using value_type  = bounds_type::value_type;
    
    template<arithmetic U>
      requires initializable_from<value_type, U>
    constexpr U operator()(const U val) const
    {
      return validate(val);
    }

    template<arithmetic U, std::size_t D>
      requires initializable_from<value_type, U>
    constexpr const std::array<U, D>& operator()(const std::array<U, D>& vals) const
    {
      return validate(vals);
    }
  private:
    template<class T>
    constexpr const T& validate(const T& t) const
    {
      if constexpr(range_of_bounds_v<Bounds>)
      {
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
          (throwing_validator<Bounds[Is]>{}(t[Is]), ...);
        }(std::make_index_sequence<Bounds.size()>());
      }
      else
      {
        if(!Bounds(t))
          throw std::domain_error{
            std::format("Input {} outside permitted domain [{}, {}]", Bounds.format_input(t), Bounds.lower, Bounds.upper)
          };
      }

      return t;
    }
  };
  
  [[nodiscard]]
  std::filesystem::path bounds_free_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void bounds_free_test::run_tests()
  {
    STATIC_CHECK( bounds<coordinate_bounds{0.0, 1.0}>);
    STATIC_CHECK(!bounds<coordinate_bounds{2.0, 1.0}>);
    STATIC_CHECK( bounds<annulus_bounds{1.0, 2.0}>);
    STATIC_CHECK(!bounds<annulus_bounds{2.1, 2.0}>);
    STATIC_CHECK( bounds_for<coordinate_bounds{0.0, 1.0}, euclidean_vector_space<double, 1>>);
    STATIC_CHECK( bounds_for<coordinate_bounds{0.0, 1.0}, euclidean_vector_space<double, 2>>);
    STATIC_CHECK( bounds_for<annulus_bounds{0.0, 1.0}, euclidean_vector_space<double, 2>>);
    STATIC_CHECK(!bounds_for<annulus_bounds{0.0, 1.0}, euclidean_vector_space<double, 1>>);
    STATIC_CHECK(!bounds_for<annulus_bounds{0.0, 1.0}, euclidean_vector_space<double, 3>>);

    check_exception_thrown<std::domain_error>(
      "",
      [](){
        throwing_validator<coordinate_bounds{0.0, 1.0}>{}(2.0);
      }
    );

    check_exception_thrown<std::domain_error>(
      "",
      [](){
        throwing_validator<coordinate_bounds<double>{0.0, 1.0}>{}(std::array{2.0, 1.0});
      }
    );

    check_exception_thrown<std::domain_error>(
      "",
      [](){
        throwing_validator<annulus_bounds{1.0, 2.0}>{}(std::array{3.0, 4.0});
      }
    );

    check_exception_thrown<std::domain_error>(
      "",
      [](){
        throwing_validator<std::array{coordinate_bounds{0.0, 1.0}, coordinate_bounds{-1.0, 0.0},}>{}(std::array{-1.0, 0.0});
      }
    );
  }
}
