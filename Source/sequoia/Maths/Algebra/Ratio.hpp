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
    using num_type = std::remove_cv_t<decltype(Num)>;
    using den_type = std::remove_cv_t<decltype(Den)>;

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
  
  enum class allow_ratio_fp_conversion : bool { no, yes };

  namespace impl
  {
    
    template<allow_ratio_fp_conversion, class...>
    struct ratio_product;

    template<allow_ratio_fp_conversion Relaxed, class T, class U>
    using ratio_product_t = ratio_product<Relaxed, T, U>::type;

    template<allow_ratio_fp_conversion Relaxed, auto Num1, auto Den1, auto Num2, auto Den2>
      requires arithmetic<decltype(Num1)> && arithmetic<decltype(Den1)>
            && arithmetic<decltype(Num2)> && arithmetic<decltype(Den2)>
    struct ratio_product<Relaxed, ratio<Num1, Den1>, ratio<Num2, Den2>>
    {
      using reduced_ratio_a_type = ratio<ratio<Num1, Den1>::num, ratio<Num2, Den2>::den>;
      using reduced_ratio_b_type = ratio<ratio<Num2, Den2>::num, ratio<Num1, Den1>::den>;

      template<auto M, auto N>
      constexpr static auto product() {
        using common_t = std::common_type_t<std::remove_cv_t<decltype(M)>, std::remove_cv_t<decltype(N)>>;
        constexpr static bool overflow{std::numeric_limits<common_t>::max() / M < N};
        if constexpr((Relaxed == allow_ratio_fp_conversion::no) || (!overflow))
          return M * N;
        else
          return static_cast<long double>(M) * static_cast<long double>(N) ;
      }

      constexpr static auto num{product<reduced_ratio_a_type::num, reduced_ratio_b_type::num>()};
      constexpr static auto den{product<reduced_ratio_a_type::den, reduced_ratio_b_type::den>()};

      using type = ratio<num, den>;
    };

    template<allow_ratio_fp_conversion Relaxed, auto Num1, auto Den1, std::intmax_t Num2, std::intmax_t Den2>
      requires arithmetic<decltype(Num1)> && arithmetic<decltype(Den1)>
    struct ratio_product<Relaxed, ratio<Num1, Den1>, std::ratio<Num2, Den2>>
      : ratio_product<Relaxed, ratio<Num1, Den1>, ratio<Num2, Den2>>
    {
    };

    template<allow_ratio_fp_conversion Relaxed, std::intmax_t Num1, std::intmax_t Den1, auto Num2, auto Den2>
      requires arithmetic<decltype(Num2)> && arithmetic<decltype(Den2)>
    struct ratio_product<Relaxed, std::ratio<Num1, Den1>, ratio<Num2, Den2>>
      : ratio_product<Relaxed, ratio<Num1, Den1>, ratio<Num2, Den2>>
    {
    };


    template<allow_ratio_fp_conversion Relaxed, std::intmax_t Num1, std::intmax_t Den1, std::intmax_t Num2, std::intmax_t Den2>
    struct ratio_product<Relaxed, std::ratio<Num1, Den1>, std::ratio<Num2, Den2>>
      : ratio_product<Relaxed, ratio<Num1, Den1>, ratio<Num2, Den2>>
    {
    };
  }

  template<class T, class U, allow_ratio_fp_conversion Relaxed=allow_ratio_fp_conversion::no>
    requires defines_ratio_v<T> && defines_ratio_v<U>
  using ratio_multiply = impl::ratio_product_t<Relaxed, T, U>;

  template<class T, class U, allow_ratio_fp_conversion Relaxed=allow_ratio_fp_conversion::no>
    requires defines_ratio_v<T> && defines_ratio_v<U>
  using ratio_divide = impl::ratio_product_t<Relaxed, T, ratio<U::den, U::num>>;
}
