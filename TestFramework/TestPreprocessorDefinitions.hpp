////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Central location for pre-processor options.
 */

namespace sequoia::unit_testing
{
  [[nodiscard]]
  constexpr bool minimal_graph_tests() noexcept
  {
    #ifndef MINIMAL_GRAPH_TESTS
     return false;
    #endif

   return true;
  }
}
