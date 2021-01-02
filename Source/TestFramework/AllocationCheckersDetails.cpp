////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for AllocationCheckersDetails.hpp
*/

#include "AllocationCheckersDetails.hpp"

namespace sequoia::testing::impl
{
  std::string allocation_advice::operator()(int count, int) const
  {
    return (count < 0)
      ? "A negative allocation count generally indicates an allocator propagting when it shouldn't or not propagating when it should.\n"
        "Alternatively, for scoped allocator adaptors, it may be that the predicted number of (inner) containers is incorrect."
      : "";
  }
}
