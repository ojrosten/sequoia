////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "ExperimentalTest.hpp"

#include <iostream>
#include <source_location>

namespace sequoia::testing
{
  namespace
  {
    class check_info
    {
    public:
      check_info(std::string_view, std::source_location loc = std::source_location::current())
        : m_Loc{loc}
      {
      }

      [[nodiscard]]
      const std::source_location& location() const noexcept { return m_Loc; }
    private:
      std::source_location m_Loc{};
    };
  }

  [[nodiscard]]
  std::string_view experimental_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void experimental_test::run_tests()
  {
    check_info c{""};
    auto loc{c.location()};
    check(equality, LINE(""), loc.line(), uint_least32_t{42});
  }
}
