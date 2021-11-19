////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Traits and Concepts for Pooled/Spawned data.

 */

namespace sequoia::ownership
{
  // TO DO: replace with contexpr bool once MSVC can cope with this
  template<class T>
  concept creator = requires(T & a) {
    typename T::proxy;
    typename T::value_type;

    { a.make() } -> std::same_as<typename T::proxy>;
  };
}
