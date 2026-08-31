////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

export module sequoia.maths.graph:StaticGraphDetails;

import std;

import :Edge;
import :EdgesAndNodesUtilities;
import :GraphDetails;
export import sequoia.core.data_structures;
export import sequoia.core.meta;
export import sequoia.core.object;

/** \file
    \brief Implementatoin details for static graphs.
 */

export namespace data_structures
{
}

export namespace sequoia::maths::graph_impl
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
}
