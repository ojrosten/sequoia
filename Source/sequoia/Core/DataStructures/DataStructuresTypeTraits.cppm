////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

export module sequoia.core.data_structures:DataStructuresTypeTraits;

import std;

/** \file
    \brief Traits for data structures.
 */

export namespace sequoia
{
  template<class T>
  inline constexpr bool has_partitions_allocator{
    requires { typename T::partitions_allocator_type; }
  };
}
