////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "RegularTestCore.hpp"

#include "ProtectiveWrapper.hpp"

namespace sequoia::testing
{
  template<class T>
  struct detailed_equality_checker<utilities::protective_wrapper<T>>
  {
    using type = utilities::protective_wrapper<T>;
    
    template<test_mode Mode>
    static void check(std::string_view description, test_logger<Mode>& logger, const type& wrapper, const type& prediction)
    {
      check_equality(description, logger, wrapper.get(), prediction.get());
    }
  };

  
  struct data
  {
    int a;
    double b;

    friend constexpr bool operator==(const data& lhs, const data& rhs) noexcept = default;

    friend constexpr bool operator!=(const data& lhs, const data& rhs) noexcept = default;

    template<class Stream> friend Stream& operator<<(Stream& stream, const data& p)
    {
      stream << p.a << p.b;

      return stream;
    }
  };
}
