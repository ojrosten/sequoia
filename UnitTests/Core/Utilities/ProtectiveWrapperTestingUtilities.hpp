////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "UnitTestCore.hpp"

#include "ProtectiveWrapper.hpp"

namespace sequoia::unit_testing
{
  template<class T>
  struct detailed_equality_checker<utilities::protective_wrapper<T, std::is_empty_v<T>>>
  {
    using type = utilities::protective_wrapper<T, std::is_empty_v<T>>;
    
    template<class Logger>
    static void check(Logger& logger, const type& wrapper, const type& prediction, std::string_view description)
    {
      check_equality(logger, wrapper.get(), prediction.get(), description);
    }
  };

  
  struct data
  {
    int a;
    double b;

    friend constexpr bool operator==(const data& lhs, const data& rhs)
    {
      return (lhs.a == rhs.a) && (lhs.b == rhs.b);
    }

    friend constexpr bool operator!=(const data& lhs, const data& rhs)
    {
      return !(lhs == rhs);
    }

    template<class Stream> friend Stream& operator<<(Stream& stream, const data& p)
    {
      stream << p.a << p.b;

      return stream;
    }
  };
}
