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

namespace sequoia::testing
{
  namespace allocation_equivalence_classes
  {
    template<class Allocator>
    struct container_of_clonables {};
  }

  template<class Allocator>
  class alloc_prediction_shifter<allocation_equivalence_classes::container_of_clonables<Allocator>>
    : public alloc_prediction_shifter<allocation_equivalence_classes::container_of_pointers<Allocator>>
  {
    using base_t = alloc_prediction_shifter<allocation_equivalence_classes::container_of_pointers<Allocator>>;

    template<auto Event>
    constexpr static alloc_prediction<Event> increment(alloc_prediction<Event> p) noexcept
    {
      return {p.unshifted(), p.value() + 1 - p.unshifted()};
    }
  public:
    using alloc_prediction_shifter<allocation_equivalence_classes::container_of_pointers<Allocator>>::alloc_prediction_shifter;
    using alloc_prediction_shifter<allocation_equivalence_classes::container_of_pointers<Allocator>>::shift;

    [[nodiscard]]
    constexpr copy_prediction shift(copy_prediction p, container_tag tag) const noexcept
    {
      return increment(base_t::shift(p, tag));
    }

    [[nodiscard]]
    constexpr assign_prediction shift(assign_prediction p) const noexcept
    {
      return increment(base_t::shift(p));
    }

    [[nodiscard]]
    constexpr assign_prop_prediction shift(assign_prop_prediction p) const noexcept
    {
      return increment(base_t::shift(p));
    }

    [[nodiscard]]
    constexpr para_copy_prediction shift(para_copy_prediction p) const noexcept
    {
      return increment(base_t::shift(p));
    }
  };

  template<class Storage>
  struct equiv_class_generator;

  template<class Storage>
  using equiv_class_generator_t = typename equiv_class_generator<Storage>::type;

  template<class Sharing, bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  struct equiv_class_generator<node_storage_tester<Sharing, PropagateCopy, PropagateMove, PropagateSwap>>
  {
    using node_allocator = typename node_storage_tester<Sharing, PropagateCopy, PropagateMove, PropagateSwap>::allocator_type;
    using type = allocation_equivalence_classes::container_of_pointers<node_allocator>;
  };

  template<class T, bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  struct equiv_class_generator<node_storage_tester<ownership::data_pool<T>, PropagateCopy, PropagateMove, PropagateSwap>>
  {
    using node_allocator = typename node_storage_tester<ownership::data_pool<T>, PropagateCopy, PropagateMove, PropagateSwap>::allocator_type;
    using type = allocation_equivalence_classes::container_of_clonables<node_allocator>;
  };

  template<class Storage>
  struct node_alloc_getter
  {
    using node_allocator = typename Storage::allocator_type;

    using alloc_equivalence_class = equiv_class_generator_t<Storage>;

    [[nodiscard]]
    node_allocator operator()(const Storage& s) const
    {
      return s.get_node_allocator();
    }
  };
}
