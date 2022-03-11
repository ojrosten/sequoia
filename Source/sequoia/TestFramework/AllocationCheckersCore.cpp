////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for AllocationCheckersCore.hpp
*/

#include "sequoia/TestFramework/AllocationCheckersCore.hpp"

#include <stdexcept>

namespace sequoia::testing
{
  [[nodiscard]]
  std::string to_string(container_tag tag)
  {
    switch(tag)
    {
    case container_tag::x:
      return "x";
    case container_tag::y:
      return "y";
    }

    throw std::logic_error{"Unrecognized option for container_tag"};
  }
}