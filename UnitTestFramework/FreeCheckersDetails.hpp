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
  
}
