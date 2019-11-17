////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include <type_traits>

namespace sequoia
{
  template<class T, class = std::void_t<>>
  struct has_partitions_allocator_type : std::false_type
  {};

  template<class T>
  struct has_partitions_allocator_type<T, std::void_t<typename T::partitions_allocator_type>> : std::true_type
  {};

  template<class T> constexpr bool has_partitions_allocator_type_v{has_partitions_allocator_type<T>::value};

  template<class T> using has_partitions_allocator_type_t = typename has_partitions_allocator_type<T>::type;
}
