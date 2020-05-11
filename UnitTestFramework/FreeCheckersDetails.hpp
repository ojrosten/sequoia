////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Implementation details for the testing of free functions.
*/

#include "UnitTestLogger.hpp"

namespace sequoia::unit_testing
{
  template<class T, class... U>
  [[nodiscard]]
  std::string add_type_info(std::string_view description);

  template<class T, class... U>
  struct type_demangler;
}

namespace sequoia::unit_testing::impl
{
  template<class EquivChecker, test_mode Mode, class T, class S, class... U>
  bool check(std::string_view description, unit_test_logger<Mode>& logger, const T& value, const S& s, const U&... u)
  {
    using sentinel = typename unit_test_logger<Mode>::sentinel;

    const std::string message{
      add_type_info<S, U...>(
                             combine_messages(description, "Comparison performed using:\n\t" + type_demangler<EquivChecker>::make() + "\n\tWith equivalent types:", "\n"))
    };
      
    sentinel r{logger, message};
    const auto previousFailures{logger.failures()};
    
    EquivChecker::check(message, logger, value, s, u...);
      
    return logger.failures() == previousFailures;
  }
}
