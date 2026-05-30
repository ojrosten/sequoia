////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "sequoia/Core/Meta/Concepts.hpp"

namespace sequoia::maths
{
  template<arithmetic T>
  inline constexpr T greatest_upper_bound{
    std::numeric_limits<T>::has_infinity ? std::numeric_limits<T>::infinity() : std::numeric_limits<T>::max()
  };

  template<arithmetic T>
  inline  constexpr T least_lower_bound{
    std::numeric_limits<T>::has_infinity ? -std::numeric_limits<T>::infinity() : std::numeric_limits<T>::lowest()
  };
  
  template<arithmetic T>
  [[nodiscard]]
  constexpr T saturating_mul(T x, T y) noexcept
  {
    using value_t = T;

    constexpr auto llb{least_lower_bound<value_t>},
                   gub{greatest_upper_bound<value_t>};

    if(std::isnan(x) || std::isnan(y))
      return std::numeric_limits<value_t>::quiet_NaN();

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
