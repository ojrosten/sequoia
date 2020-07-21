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
  concept ownership = requires() {
    typename T::handle_type;
    typename T::elementary_type;

    { T::get(std::declval<typename T::handle_type>()) } -> same_as<const typename T::elementary_type&>;
    { T::get_ptr(std::declval<typename T::handle_type>()) } -> same_as<const typename T::elementary_type*>;
    { T::get(makelval<typename T::handle_type>()) } -> same_as<typename T::elementary_type&>;
    { T::get_ptr(makelval<typename T::handle_type>()) } -> same_as<typename T::elementary_type*>;
    { T::make() } -> same_as<typename T::handle_type>;
  };
}
