////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/TestFramework/RegularTestCore.hpp"
#include "sequoia/Runtime/Factory.hpp"

#include <map>

namespace sequoia::testing
{
  template<class... Products>
  struct equivalence_checker<sequoia::runtime::factory<Products...>>
  {
    using type = sequoia::runtime::factory<Products...>;
    using element = std::pair<std::string, std::variant<Products...>>;
    
    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const type& actual, const std::array<element, sizeof...(Products)>& prediction)
    {
      for(const auto&[name, product] : prediction)
      {
        const auto message{std::string{"Product generated by name '"}.append(name).append("'")};
        check_equality(message, logger, actual.create(name), product);
      }
    }
  };
}
