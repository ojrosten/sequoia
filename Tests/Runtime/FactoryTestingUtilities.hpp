////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/RegularTestCore.hpp"
#include "sequoia/Runtime/Factory.hpp"

#include <map>

namespace sequoia::testing
{
  template<class... Products>
  struct value_tester<sequoia::runtime::factory<Products...>>
  {
    using type = sequoia::runtime::factory<Products...>;
    using element = std::pair<std::string, std::variant<Products...>>;

    template<test_mode Mode, class... Args>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const type& actual, const std::array<element, sizeof...(Products)>& prediction, Args&&... args)
    {
      for(const auto&[name, product] : prediction)
      {
        const auto message{std::string{"Product generated by name '"}.append(name).append("'")};
        check(equality, message, logger, actual.create(name, args...), product);
      }
    }

    template<test_mode Mode>
    static void test(equality_check_t, test_logger<Mode>& logger, const type& actual, const type& prediction)
    {
      check(equality, "Names", logger, actual.begin_names(), actual.end_names(), prediction.begin_names(), prediction.end_names());
    }
  };
}
