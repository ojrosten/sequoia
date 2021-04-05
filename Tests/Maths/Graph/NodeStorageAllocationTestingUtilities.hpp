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
      if constexpr (has_msvc_v && (iterator_debug_level() > 0))
      {
        return {p.unshifted(), p.value() + 1 - p.unshifted()};
      }
      else
      {
        return p;
      }
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

  template<class Storage, class Allocator = typename Storage::allocator_type>
  struct equiv_class_generator;

  template<class Storage, class Allocator = typename Storage::allocator_type>
  using equiv_class_generator_t = typename equiv_class_generator<Storage, Allocator>::type;

  template<class Storage>
  concept has_weight_maker_v = requires { typename Storage::weight_maker_type; };

  template<class Storage, class Allocator>
    requires has_weight_maker_v<Storage>
  struct equiv_class_generator<Storage, Allocator>
  {
    using allocator_type = Allocator;
    using type = allocation_equivalence_classes::container_of_pointers<allocator_type>;
  };

  template<class Storage, class Allocator>
    requires    has_weight_maker_v<Storage>
             && same_as<typename Storage::weight_maker_type, ownership::data_pool<typename Storage::weight_type>>
  struct equiv_class_generator<Storage, Allocator>
  {
    using allocator_type = Allocator;
    using type = allocation_equivalence_classes::container_of_clonables<allocator_type>;
  };

  template<class Storage>
  struct node_storage_alloc_getter
  {
    using alloc_equivalence_class = equiv_class_generator_t<Storage>;

    [[nodiscard]]
    auto operator()(const Storage& s) const
    {
      return s.get_node_allocator();
    }
  };
}
