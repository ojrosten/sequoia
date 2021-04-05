////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "NodeStorageTestingUtilities.hpp"

#include "sequoia/TestFramework/AllocationCheckers.hpp"
#include "sequoia/Core/Ownership/DataPool.hpp"
#include "sequoia/Core/Meta/Concepts.hpp"

namespace sequoia::testing
{
  template<class Storage>
  struct node_storage_alloc_getter
  {
    using allocator_type = typename Storage::allocator_type;
    using alloc_equivalence_class = allocation_equivalence_classes::container_of_pointers<allocator_type>;

    [[nodiscard]]
    allocator_type operator()(const Storage& s) const
    {
      return s.get_node_allocator();
    }
  };
}
