////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Traits and Concepts for allocation checks.
*/

#include "Concepts.hpp"

namespace sequoia::testing
{
  template <class A>
  concept counting_alloc = alloc<A> && requires(const std::remove_reference_t<A>& a) {
     a.allocs();
  };

  template<class Fn, class T>
  concept alloc_getter = requires(Fn fn, const std::remove_reference_t<T>& t) {
    { fn(t) } -> counting_alloc;
  };

  template<class Fn> concept defines_alloc_equivalence_class = requires { Fn::alloc_equivalence_class; };
}
