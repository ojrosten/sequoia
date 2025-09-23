////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2025.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include <numeric>
#include <ratio>

namespace sequoia::maths
{
  template<auto Num, auto Den>
  struct ratio;

  template<auto Num, auto Den>
    requires std::integral<decltype(Num)> && std::integral<decltype(Den)>
  struct ratio<Num, Den>
  {
    constexpr static auto divisor{std::gcd(Num, Den)};
    constexpr static auto num{Num/divisor};
    constexpr static auto den{Den/divisor};
  };

  template<auto Num, auto Den>
    requires arithmetic<decltype(Num)> && arithmetic<decltype(Den)>
          && (std::floating_point<decltype(Num)> || std::floating_point<decltype(Den)>)
  struct ratio<Num, Den>
  {
    using num_type = std::remove_cv_t<decltype(Num)>;
    using den_type = std::remove_cv_t<decltype(Den)>;
    
    // TO DO: once constexpr fmod / remainder is supported,
    // consider more general reductions
    constexpr static bool is_identity_v{Num == Den};
    constexpr static auto num{is_identity_v ? num_type(1) : Num};
    constexpr static auto den{is_identity_v ? den_type(1) : Den};
  };

  template<class T>
  inline constexpr bool defines_ratio_v{
    requires {
      T::num;
      T::den;

      requires arithmetic<decltype(T::num)>;
      requires arithmetic<decltype(T::den)>;
    }
  };

  namespace impl
  {
    template<class...>
    struct ratio_product;

    template<class T, class U>
    using ratio_product_t = ratio_product<T, U>::type;

    template<auto Num1, auto Den1, auto Num2, auto Den2>
    requires (   !std::integral<decltype(Num1)> || !std::integral<decltype(Den1)>
              || !std::integral<decltype(Num2)> || !std::integral<decltype(Den2)>)
    struct ratio_product<ratio<Num1, Den1>, ratio<Num2, Den2>>
    {
      using type
        = std::conditional_t<
            Num1 == Den2,                    ratio<Num2, Den1>,
            std::conditional_t<Den1 == Num2, ratio<Num1, Den2>,
                                             ratio<Num1 * Num2, Den1 * Den2>>
          >;
    };

    template<auto Num1, auto Den1, std::intmax_t Num2, std::intmax_t Den2>
      requires std::integral<decltype(Den1)>
    struct ratio_product<ratio<Num1, Den1>, std::ratio<Num2, Den2>>
    {
      using prelim_ratio_type = std::ratio<Num2, Den1>;
      constexpr static auto prelim_num{prelim_ratio_type::num}, prelim_den{prelim_ratio_type::den};

      using reduced_ratio_type = std::ratio<prelim_num, prelim_den * Den2>;
      using type = ratio<Num1 * reduced_ratio_type::num, reduced_ratio_type::den>;
    };

    template<auto Num1, auto Den1, std::intmax_t Num2, std::intmax_t Den2>
      requires std::integral<decltype(Num1)>
    struct ratio_product<ratio<Num1, Den1>, std::ratio<Num2, Den2>>
    {
      using reduced_ratio_type = std::ratio<Num1 * Num2, Den2>;
      using type = ratio<reduced_ratio_type::num, reduced_ratio_type::den * Den1>;
    };

    template<intmax_t Num1, intmax_t Den1, auto Num2, auto Den2>
      requires std::integral<decltype(Den2)>
    struct ratio_product<std::ratio<Num1, Den1>, ratio<Num2, Den2>>
    {
      using reduced_ratio_type = std::ratio<Num1, Den1 * Den2>;
      using type = ratio<Num2 * reduced_ratio_type::num, reduced_ratio_type::den>;
    };

    template<intmax_t Num1, intmax_t Den1, auto Num2, auto Den2>
      requires std::integral<decltype(Num2)>
    struct ratio_product<std::ratio<Num1, Den1>, ratio<Num2, Den2>>
    {
      using reduced_ratio_type = std::ratio<Num1 * Num2, Den1>;
      using type = ratio<reduced_ratio_type::num, reduced_ratio_type::den * Den2>;
    };

    template<auto Num1, auto Den1, std::intmax_t Num2, std::intmax_t Den2>
    struct ratio_product<ratio<Num1, Den1>, std::ratio<Num2, Den2>> : ratio_product<ratio<Num1, Den1>, ratio<Num2, Den2>>
    {};

    template<std::intmax_t Num1, std::intmax_t Den1, auto Num2, auto Den2>
    struct ratio_product<std::ratio<Num1, Den1>, ratio<Num2, Den2>> : ratio_product<ratio<Num1, Den1>, ratio<Num2, Den2>>
    {};

    template<std::intmax_t Num1, std::intmax_t Den1, std::intmax_t Num2, std::intmax_t Den2>
    struct ratio_product<std::ratio<Num1, Den1>, std::ratio<Num2, Den2>>
    {
      using reduced_ratio_type = std::ratio_multiply<std::ratio<Num1, Den1>, std::ratio<Num2, Den2>>;
      using type = ratio<reduced_ratio_type::num, reduced_ratio_type::den>;
    };

    template<auto Num1, auto Den1, auto Num2, auto Den2>
      requires std::integral<decltype(Num1)> && std::integral<decltype(Den1)>
            && std::integral<decltype(Num2)> && std::integral<decltype(Den2)>
    struct ratio_product<ratio<Num1, Den1>, ratio<Num2, Den2>>
      : ratio_product<std::ratio<Num1, Den1>, std::ratio<Num2, Den2>>
    {
    };
  }

  template<class T, class U>
    requires defines_ratio_v<T> && defines_ratio_v<U>
  using ratio_multiply = impl::ratio_product_t<T, U>;
}
