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

  template<std::intmax_t Num, intmax_t Den>
  struct ratio<Num, Den> : std::ratio<Num, Den>
  {
  };

  template<long double Num, long double Den>
  struct ratio<Num, Den>
  {
    // TO DO: once constexpr fmod / remainder is supported,
    // consider more general reductions
    constexpr static bool is_identity_v{Num == Den};
    constexpr static auto num{is_identity_v ? 1.0L : Num};
    constexpr static auto den{is_identity_v ? 1.0L : Den};
  };

  template<long double Num, std::intmax_t Den>
  struct ratio<Num, Den>
  {
    constexpr static auto num{Num};
    constexpr static auto den{Den};
  };

  template<std::intmax_t Num, long double Den>
  struct ratio<Num, Den>
  {
    constexpr static auto num{Num};
    constexpr static auto den{Den};
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
