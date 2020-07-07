////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Traits and Concepts for allocation checks.
*/

namespace sequoia::testing
{
  template<class A, class = std::void_t<>> struct counts_allocations : std::false_type
  {};
  
  template<class A> struct counts_allocations<A, std::void_t<decltype(std::declval<A>().allocs())>>
    : std::bool_constant<is_allocator_v<A>>
  {};

  template<class A> constexpr bool counts_allocations_v{counts_allocations<A>::value};

  template<class A>
  using counts_allocations_t = typename counts_allocations<A>::type;

  template <class A>
  concept counting_alloc = is_allocator_v<A> && counts_allocations_v<A>;

  template<class Fn, class T>
  concept alloc_getter = requires(Fn fn, const T& t) {
    counts_allocations_v<decltype(fn(t))> == true;                            
  };
}
