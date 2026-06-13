////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "sequoia/Core/Meta/Concepts.hpp"

#include <cmath>

namespace sequoia::maths
{
  template<arithmetic T>
  inline constexpr T greatest_upper_bound{
    std::numeric_limits<T>::has_infinity ? std::numeric_limits<T>::infinity() : std::numeric_limits<T>::max()
  };

  template<arithmetic T>
  inline constexpr T least_lower_bound{
    // Use IIL to avoid unhelpful MSVC warning from ternary
    [](){
      if constexpr(std::numeric_limits<T>::has_infinity)
        return -std::numeric_limits<T>::infinity();
      else
        return  std::numeric_limits<T>::lowest();
    }()
  };

  // Hack to work around various the fact that, for MSVC:
  // isnan isn't constexpr
  template<arithmetic T>
  constexpr bool isnan(T val) noexcept
  {
#if defined(_MSC_VER)
    return !(val == val);
#else
    return std::isnan(val);
#endif
  }

  // TO DO: refine this, esp for mixed floating-point / integral
  // The current logic is adapted to the fact that the impl uses
  // std::common_type. The common type of int and unsigned is
  // unsigned - wich is undesriable - whereas that of long and
  // unsigned is long - which is desirable. Hence the sizeof
  // conditions
  template<arithmetic T, arithmetic U>
  inline constexpr bool has_saturating_arithmetic_v{    
       (std::is_signed_v<T>   && std::is_signed_v<U>)
    || (std::is_unsigned_v<T> && std::is_unsigned_v<U>)
    || (std::is_signed_v<T>   && (sizeof(T) > sizeof(U)))
    || (std::is_signed_v<U>   && (sizeof(T) < sizeof(U)))
  };

  template<arithmetic T, arithmetic U>
    requires has_saturating_arithmetic_v<T, U>
  [[nodiscard]]
  constexpr std::common_type_t<T, U> saturating_add(T x, U y) noexcept
  {
    using value_t = std::common_type_t<T, U>;

    constexpr auto llb{least_lower_bound<value_t>},
                   gub{greatest_upper_bound<value_t>};

    if constexpr(std::numeric_limits<value_t>::has_quiet_NaN)
    {
      if(isnan(x) || isnan(y))
        return std::numeric_limits<value_t>::quiet_NaN();
    }

    if((x > 0) && (y > 0))
    {
      if((x == gub) || (y == gub))
        return gub;

      return x > gub - y ? gub : x + y;
    }

    if((x < 0) && (y < 0))
    {
      if((x == llb) || (y == llb))
        return llb;

      return x < llb - y ? llb : x + y;
    }

    return x + y;
  }
  
  template<arithmetic T, arithmetic U>
    requires has_saturating_arithmetic_v<T, U>
  [[nodiscard]]
  constexpr std::common_type_t<T, U> saturating_mul(T x, U y) noexcept
  {
    using value_t = std::common_type_t<T, U>;

    constexpr auto llb{least_lower_bound<value_t>},
                   gub{greatest_upper_bound<value_t>};

    if constexpr(std::numeric_limits<value_t>::has_quiet_NaN)
    {
      if(isnan(x) || isnan(y))
        return std::numeric_limits<value_t>::quiet_NaN();
    }

    if((x > 0) && (y > 0))
    {
      if((x == gub) || (y == gub))
        return gub;

      return x > gub / y ? gub : x * y;
    }
   
    if((x < 0) && (y < 0))
    {
      if((x == llb) || (y == llb))
        return gub;

      return x < gub / y ? gub : x * y;
    }
   
    if((x > 0) && (y < 0))
    {
      if((x == gub) || (y == llb))
        return llb;

      return x > llb / y ? llb : x * y;
    }
   
    if((x < 0) && (y > 0))
    {
      if((x == llb) || (y == gub))
        return llb;

      return y > llb / x ? llb : x * y;
    }
   
    return {};
  }
  
}
