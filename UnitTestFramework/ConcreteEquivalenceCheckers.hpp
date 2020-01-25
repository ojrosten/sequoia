////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "UnitTestCheckers.hpp"

namespace sequoia::unit_testing
{
  template<class Char, class Traits, class Allocator>
  struct equivalence_checker<std::basic_string<Char, Traits, Allocator>>
  {
    using string_type = std::basic_string<Char, Traits, Allocator>;
    
    template<class Logger, std::size_t N>
    static void check(std::string_view description, Logger& logger, const string_type& s, char const (&prediction)[N])
    {
      check_equality(description, logger, std::string_view{s}, std::string_view{prediction});
    }

    template<class Logger>
    static void check(std::string_view description, Logger& logger, const string_type& s, std::basic_string_view<Char, Traits> prediction)
    {
      check_equality(description, logger, std::string_view{s}, prediction);
    }
  };
}
