////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Traits and concepts for data structures.
 */

#include <type_traits>

namespace sequoia
{
  template<class T>
  concept has_partitions_allocator = requires() {
    typename T::partitions_allocator_type;
  };
}
