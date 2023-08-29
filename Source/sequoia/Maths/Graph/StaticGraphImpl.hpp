////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Edge traits for static homogeneous graphs.
 */

#include "sequoia/Maths/Graph/GraphDetails.hpp"

namespace data_structures
{
  template <class, std::size_t, std::size_t, class> class static_partitioned_sequence;
}

namespace sequoia::maths::graph_impl
{
  enum class index_type_tag { u_char, u_short, u_int, u_long};

  template<std::size_t Size, std::size_t Order, bool Embedded>
  [[nodiscard]]
  constexpr index_type_tag to_index_max() noexcept
  {
    if constexpr     ((Order < 255) && (!Embedded || (Size < 255)))     return index_type_tag::u_char;
    else if constexpr((Order < 65535) && (!Embedded || (Size < 65535))) return index_type_tag::u_short;
    else                                                                return index_type_tag::u_long;
  }

  template
  <
    std::size_t Size,
    std::size_t Order,
    bool Embedded,
    index_type_tag=to_index_max<Size, Order, Embedded>()
  >
  struct static_edge_index_type_generator
  {
    using index_type = std::size_t;
  };

  template
  <
    std::size_t Size,
    std::size_t Order,
    bool Embedded
  >
  struct static_edge_index_type_generator<Size, Order, Embedded, index_type_tag::u_char>
  {
    using index_type = unsigned char;
  };

  template
  <
    std::size_t Size,
    std::size_t Order,
    bool Embedded
  >
  struct static_edge_index_type_generator<Size, Order, Embedded, index_type_tag::u_short>
  {
    using index_type = unsigned short;
  };

  template
  <
    graph_flavour GraphFlavour,
    std::size_t Order,
    std::size_t Size,
    class EdgeWeight,
    class EdgeMetaData,
    std::integral IndexType
  >
  struct static_edge_traits
    : public edge_type_generator<GraphFlavour, EdgeWeight, EdgeMetaData, IndexType, edge_sharing_preference::independent>
  {
    using edge_type = typename edge_type_generator<GraphFlavour, EdgeWeight, EdgeMetaData, IndexType, edge_sharing_preference::independent>::edge_type;
    using edge_storage_type = data_structures::static_partitioned_sequence<edge_type, Order, num_static_edges(GraphFlavour, Size), maths::static_monotonic_sequence<IndexType, Order, std::ranges::greater>>;
  };
}
