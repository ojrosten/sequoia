////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Traits and Concepts for Sharing Policies.

 */

#include "Concepts.hpp"

namespace sequoia
{
  template<class T>
  concept sharing_policy = requires(const T& t) {
    typename T::handle_type;
    typename T::elementary_type;

    { t.get(std::declval<typename T::handle_type>()) } -> same_as<const typename T::elementary_type&>;
    { t.get_ptr(std::declval<typename T::handle_type>()) } -> same_as<const typename T::elementary_type*>;
  };
}
