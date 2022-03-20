////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Traits and Concepts for Sharing Policies.

 */

#include "sequoia/Core/Meta/Concepts.hpp"

namespace sequoia::object
{
  template<class H>
  concept handler = requires(H& h, typename H::handle_type& x, const typename H::handle_type& cx){
    typename H::handle_type;
    typename H::elementary_type;

    { h.get(x) }      -> std::same_as<typename H::elementary_type&>;
    { h.get_ptr(x) }  -> std::same_as<typename H::elementary_type*>;
    { h.get(cx) }     -> std::same_as<const typename H::elementary_type&>;
    { h.get_ptr(cx) } -> std::same_as<const typename H::elementary_type*>;
  };
}
