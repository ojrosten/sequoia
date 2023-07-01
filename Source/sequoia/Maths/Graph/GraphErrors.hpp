////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Error messages for graphs

 */

#include <string>
#include <stdexcept>

namespace sequoia::maths::graph_errors
{
  struct edge_indices
  {
    std::size_t node{}, edge{};
  };

  [[nodiscard]]
  inline std::string to_string(const edge_indices& indices)
  {
    return std::string{"[node: "}.append(std::to_string(indices.node)).append(", edge: ").append(std::to_string(indices.edge)).append("]");
  }

  [[nodiscard]]
  inline std::string prefix(std::string_view method)
  {
    return std::string{"connectivity::"}.append(method);
  }

  [[nodiscard]]
  inline std::string error_prefix(std::string_view method)
  {
    return prefix(method).append(": ");
  }

  [[nodiscard]]
  inline std::string error_prefix(std::string_view method, const edge_indices indices)
  {
    return prefix(method).append(" ").append(to_string(indices)).append(": ");
  }

  constexpr void check_node_index_range(std::string_view method, const std::size_t order, const std::size_t node)
  {
    if(node >= order)
      throw std::out_of_range{error_prefix(method).append("node index ").append(std::to_string(node)).append(" out of range - graph order is ").append(std::to_string(order))};
  }

  constexpr void check_node_index_range(std::string_view method, const std::size_t order, const std::size_t node1, const std::size_t node2)
  {
    if((node1 >= order) || (node2 >= order))
      throw std::out_of_range{error_prefix(method).append("at least one node index [")
            .append(std::to_string(node1)).append(", ").append(std::to_string(node2))
            .append("] out of range - graph order is ").append(std::to_string(order))};
  }

  constexpr void check_edge_index_range(std::string_view method, const edge_indices edgeIndices, std::string_view indexName, const std::size_t size, const std::size_t index)
  {
    if(index >= size)
      throw std::out_of_range{error_prefix(method, edgeIndices).append(indexName).append(" index ").append(std::to_string(index)).append(" out of range - max index is ").append(std::to_string(size))};
  }

  constexpr void check_edge_swap_indices(std::size_t node, std::size_t i, std::size_t j, std::size_t numEdges)
  {
    auto check{
      [node, numEdges](std::size_t index){
        if(index >= numEdges)
          throw std::out_of_range{std::string{"swap_edges: edge index "}.append(std::to_string(index)).append(" out of range for node ")
                              .append(std::to_string(node)).append(", which has ").append(std::to_string(numEdges)).append(" edge(s)")};
      }
    };

    check(i);
    check(j);
  }

  [[nodiscard]]
  inline std::string self_referential_error(const edge_indices edgeIndices, const std::size_t target, const std::size_t compIndex)
  {
    return error_prefix("process_complementary_edges", edgeIndices).append("Indices [target: ").append(std::to_string(target)).append(", comp: ").append(std::to_string(compIndex)).append("] are self-referential");
  }

  [[nodiscard]]
  inline std::string reciprocated_error_message(const edge_indices edgeIndices, const std::string_view indexName, const std::size_t reciprocatedIndex, const std::size_t index)
  {
    return error_prefix("process_complementary_edges", edgeIndices).append("Reciprocated ").append(indexName)
      .append(" index ").append(std::to_string(reciprocatedIndex)).append(" does not match ").append(std::to_string(index));
  }

  constexpr void check_reciprocated_index(const edge_indices edgeIndices, std::string_view indexName, const std::size_t reciprocatedIndex, const std::size_t index)
  {
    if(reciprocatedIndex != index)
      throw std::logic_error{reciprocated_error_message(edgeIndices, indexName, reciprocatedIndex, index)};
  }

  constexpr void check_embedded_edge(const std::size_t nodeIndex, const std::size_t source, const std::size_t target)
  {
    if((source != nodeIndex) && (target != nodeIndex))
      throw std::logic_error{error_prefix("process_complementary_edges").append("At least one of source and target must match current node")};
  }

  [[nodiscard]]
  inline std::string erase_edge_error(const std::size_t partner, const edge_indices indices)
  {
    return error_prefix("erase_edge")
      .append("partner in partition ").append(std::to_string(partner)).append(" not found for edge ").append(to_string(indices));
  }

  [[nodiscard]]
  inline std::string odd_num_loops_error(std::string_view method, std::size_t nodeIndex)
  {
    return error_prefix(method).append("Odd number of loop edges for node ").append(std::to_string(nodeIndex));
  }
}
