////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Implementation details for static graphs.
 */

#include "sequoia/Maths/Graph/GraphDetails.hpp"
#include "sequoia/Core/Meta/TypeAlgorithms.hpp"

#include <algorithm>
#include <limits>
#include <tuple>
#include <type_traits>

namespace sequoia::maths::graph_impl
{
  template<std::size_t MaxValue>
  struct narrowest_unsigned_holding
  {
    template<class T>
    using holds = std::bool_constant<(std::numeric_limits<T>::max() >= MaxValue)>;

    using candidates = std::tuple<unsigned char, unsigned short, unsigned int, std::size_t>;

    using type = std::tuple_element_t<meta::find_if_v<candidates, holds>, candidates>;
  };

  template<std::size_t MaxValue>
  using narrowest_unsigned_holding_t = narrowest_unsigned_holding<MaxValue>::type;

  /** \brief The index type shared by a static graph's node indices and its edge-storage offsets.

      The type holds both `Order`, the number of partitions of the edge storage, and `NumEdges`, the largest
      partition offset, which also bounds an embedded edge's complementary index. Holding `Order` keeps every
      node index below the type's maximum, which a partition iterator reserves for `npos`.
   */
  template<std::size_t Order, std::size_t NumEdges>
  using static_edge_index_type = narrowest_unsigned_holding_t<std::ranges::max(Order, NumEdges)>;
}
