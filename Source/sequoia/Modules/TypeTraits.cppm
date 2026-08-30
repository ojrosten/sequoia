////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

// EXPERIMENT: a bridge module over Core/Meta/TypeTraits.hpp.
//
// The header is included in the global module fragment and its entities are
// re-exported by using-declarations, so they remain attached to the global
// module and are the *same* entities a #include would produce. That is what
// makes the migration incremental: a translation unit may include the header
// or import this module, and two that disagree still agree about the types.

module;

#include "sequoia/Core/Meta/TypeTraits.hpp"

export module sequoia.core.meta.type_traits;

export namespace sequoia
{
  using sequoia::are_same;
  using sequoia::are_same_t;
  using sequoia::are_same_v;
  using sequoia::dependent_false;
  using sequoia::has_allocator_type;
  using sequoia::has_allocator_type_t;
  using sequoia::has_allocator_type_v;
  using sequoia::has_element_type_v;
  using sequoia::has_gettable_elements;
  using sequoia::has_value_type_v;
  using sequoia::heterogeneous_deep_equality;
  using sequoia::heterogeneous_deep_equality_v;
  using sequoia::heterogeneous_deep_total_order;
  using sequoia::heterogeneous_deep_total_order_v;
  using sequoia::is_compatible;
  using sequoia::is_compatible_t;
  using sequoia::is_compatible_v;
  using sequoia::is_const_pointer;
  using sequoia::is_const_pointer_t;
  using sequoia::is_const_pointer_v;
  using sequoia::is_const_reference;
  using sequoia::is_const_reference_t;
  using sequoia::is_const_reference_v;
  using sequoia::is_deep_equality_comparable;
  using sequoia::is_deep_equality_comparable_t;
  using sequoia::is_deep_equality_comparable_v;
  using sequoia::is_deep_totally_ordered;
  using sequoia::is_deep_totally_ordered_t;
  using sequoia::is_deep_totally_ordered_v;
  using sequoia::is_initializable;
  using sequoia::is_initializable_t;
  using sequoia::is_initializable_v;
  using sequoia::is_tuple;
  using sequoia::is_tuple_t;
  using sequoia::is_tuple_v;
  using sequoia::resolve_to_copy;
  using sequoia::resolve_to_copy_t;
  using sequoia::resolve_to_copy_v;
  using sequoia::value_type_of;
  using sequoia::value_type_of_t;
}
