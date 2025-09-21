////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2025.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include <numbers>
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
    constexpr static auto num{Num};
    constexpr static auto den{Den};
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
  struct is_ratio : std::false_type {};

  template<class T>
  using is_ratio_t = is_ratio<T>::type;

  template<class T>
  inline constexpr bool is_ratio_v{is_ratio<T>::value};

  template<std::intmax_t Num, std::intmax_t Den>
  struct is_ratio<std::ratio<Num, Den>> : std::true_type {};

  template<auto Num, auto Den>
  struct is_ratio<ratio<Num, Den>> : std::true_type {};

}
