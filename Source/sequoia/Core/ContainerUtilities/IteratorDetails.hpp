////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/Core/Meta/TypeTraits.hpp"

/*! \file
    \brief Implementation details for iterator.
 */

namespace sequoia::utilities
{
  template<class Policy>
  concept dereference_policy = requires() {
    typename Policy::value_type;
    typename Policy::reference;
    //typename Policy::pointer;
  };

  template<dereference_policy Deref>
  struct pointer_type
  {
    using type = void;
  };

  template<dereference_policy Deref>
    requires requires { typename Deref::pointer; }
  struct pointer_type<Deref>
  {
    using type = typename Deref::pointer;
  };

  template<dereference_policy Deref>
  using pointer_type_t = typename pointer_type<Deref>::type;
}
