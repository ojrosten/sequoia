////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

module;

#include <algorithm>
#include <array>
#include <concepts>
#include <functional>
#include <iterator>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

export module sequoia.maths.graph:StaticGraphConfig;

import :Edge;
import :EdgesAndNodesUtilities;
import :GraphDetails;
import :StaticGraphDetails;
export import sequoia.algorithms;
export import sequoia.core.container_utilities;
export import sequoia.core.meta;
export import sequoia.core.object;
export import sequoia.maths.sequences;

/** \file
    \brief Edge configuration for static graphs.
 */


export namespace sequoia::maths
{
  template<graph_flavour Flavour, std::size_t Size, std::size_t Order>
  struct static_edge_storage_config
  {
    using index_type = graph_impl::static_edge_index_type_generator<Size, Order, is_embedded(Flavour)>::index_type;

    template <class T> using storage_type
      = data_structures::static_partitioned_sequence<
          T,
          Order,
          graph_impl::num_static_edges(Flavour, Size),
          maths::static_monotonic_sequence<index_type, Order, std::ranges::greater>>;

    constexpr static edge_sharing_preference edge_sharing{edge_sharing_preference::independent};
  };
}
