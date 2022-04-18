////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/RegularTestCore.hpp"

#include "sequoia/Core/Object/FaithfulWrapper.hpp"

namespace sequoia::testing
{
  template<class T>
  struct value_tester<object::faithful_wrapper<T>>
  {
    using type = object::faithful_wrapper<T>;

    template<class CheckerType, test_mode Mode>
    static void test(CheckerType flavour, test_logger<Mode>& logger, const type& wrapper, const type& prediction)
    {
      check(flavour, "", logger, wrapper.get(), prediction.get());
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