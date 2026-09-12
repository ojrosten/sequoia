////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Implementation details for monotonic sequences.
 */

#include "sequoia/Core/Meta/Concepts.hpp"

namespace sequoia::maths::impl
{
  template<class C> struct static_storage : std::false_type {};

  template<class T, std::size_t N> struct static_storage<std::array<T, N>> : std::true_type
  {
    constexpr static std::size_t size() noexcept { return N; }
  };

  /** \brief Whether swapping two instances of `C` is `noexcept`: the allocators propagate or are interchangeable.

      Made from the allocator rather than asked of the container, because `std::is_nothrow_swappable`
      follows the container's own promise and libc++ marks `vector::swap` unconditionally `noexcept`,
      so the answer would differ by standard library.
   */
  template<class C>
  struct swap_is_noexcept
    : std::bool_constant<
           std::allocator_traits<typename C::allocator_type>::propagate_on_container_swap::value
        || std::allocator_traits<typename C::allocator_type>::is_always_equal::value
      >
  {};

  template<class C>
    requires (!has_allocator_type_v<C>)
  struct swap_is_noexcept<C> : std::true_type
  {};

  template<class C>
  inline constexpr bool swap_is_noexcept_v{swap_is_noexcept<C>::value};
}
