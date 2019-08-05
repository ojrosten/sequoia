////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file MonotonicSequenceDetails.hpp
    \brief Implementation details for monotonic sequences.
 */

#include "TypeTraits.hpp"

namespace sequoia::maths::impl
{
  template<class C> struct static_storage : std::false_type {};

  template<class T, std::size_t N> struct static_storage<std::array<T, N>> : std::true_type
  {
    constexpr static std::size_t size() noexcept { return N; }
  };

  template<class C, bool=has_allocator_type_v<C>>
  struct noexcept_spec
    : std::bool_constant<
           std::allocator_traits<typename C::allocator_type>::propagate_on_container_swap::value
        || std::allocator_traits<typename C::allocator_type>::is_always_equal::value
      >
  {};

  template<class C>
  struct noexcept_spec<C, false> : std::true_type
  {};

  template<class C> constexpr bool noexcept_spec_v{noexcept_spec<C>::value};
}
