////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Edge configuration for static graphs.
 */

#include "sequoia/Maths/Graph/StaticGraphDetails.hpp"

namespace sequoia::maths
{
  template<graph_flavour Flavour, std::size_t Size, std::size_t Order>
  struct static_edge_storage_config
  {
    using index_type = typename graph_impl::static_edge_index_type_generator<Size, Order, is_embedded(Flavour)>::index_type;

    template <class T> using storage_type
      = data_structures::static_partitioned_sequence<
          T,
          Order,
          graph_impl::num_static_edges(Flavour, Size),
          maths::static_monotonic_sequence<index_type, Order, std::ranges::greater>>;

    constexpr static edge_sharing_preference edge_sharing{edge_sharing_preference::independent};
  };
}
