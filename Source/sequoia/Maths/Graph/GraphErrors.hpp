////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
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

  /** \brief The number of partial edges from `node` to `target`, and from `target` back to `node`.

      In a weighted graph, only the partial edges of one weight are counted, since a partial edge is
      reciprocated only by one of equal weight.
   */
  struct partial_edge_counts
  {
    std::size_t node{}, target{}, to_target{}, from_target{};
  };

  enum class edge_weighting { unweighted, weighted };

  [[nodiscard]]
  std::string node_index_range_message(std::string_view method, std::size_t order, std::size_t node);

  [[nodiscard]]
  std::string node_index_range_message(std::string_view method, std::size_t order, std::size_t node1, std::size_t node2);

  [[nodiscard]]
  std::string edge_index_range_message(std::string_view method, edge_indices edgeIndices, std::string_view indexName, std::size_t size, std::size_t index);

  [[nodiscard]]
  std::string edge_insertion_index_message(std::string_view method, std::size_t node,  std::size_t sizeAfterFirstInsertion, std::size_t index);

  [[nodiscard]]
  std::string edge_swap_indices_message(std::size_t node, std::size_t index, std::size_t numEdges);

  [[nodiscard]]
  std::string reciprocated_error_message(const edge_indices edgeIndices, const std::string_view indexName, const std::size_t reciprocatedIndex, const std::size_t index);

  [[nodiscard]]
  std::string inconsistent_initialization_message(std::size_t numNodes, std::size_t edgeParitions);

  constexpr void check_node_index_range(std::string_view method, const std::size_t order, const std::size_t node)
  {
    if(node >= order)
      throw std::out_of_range{node_index_range_message(method, order, node)};
  }

  constexpr void check_node_index_range(std::string_view method, const std::size_t order, const std::size_t node1, const std::size_t node2)
  {
    if((node1 >= order) || (node2 >= order))
      throw std::out_of_range{node_index_range_message(method, order, node1, node2)};
  }

  constexpr void check_edge_index_range(std::string_view method, const edge_indices edgeIndices, std::string_view indexName, const std::size_t size, const std::size_t index)
  {
    if(index >= size)
      throw std::out_of_range{edge_index_range_message(method, edgeIndices, indexName, size, index)};
  }

  constexpr void check_edge_insertion_index(std::string_view method, const std::size_t node, const std::size_t sizeAfterFirstInsertion, const std::size_t index)
  {
    if(index > sizeAfterFirstInsertion)
      throw std::out_of_range{edge_insertion_index_message(method, node, sizeAfterFirstInsertion, index)};
  }

  constexpr void check_edge_swap_indices(std::size_t node, std::size_t i, std::size_t j, std::size_t numEdges)
  {
    auto check{
      [node, numEdges](std::size_t index){
        if(index >= numEdges)
          throw std::out_of_range{edge_swap_indices_message(node, index, numEdges)};
      }
    };

    check(i);
    check(j);
  }

  constexpr void check_reciprocated_index(const edge_indices edgeIndices, std::string_view indexName, const std::size_t reciprocatedIndex, const std::size_t index)
  {
    if(reciprocatedIndex != index)
      throw std::logic_error{reciprocated_error_message(edgeIndices, indexName, reciprocatedIndex, index)};
  }

  [[nodiscard]]
  std::string erase_edge_error(std::size_t partner, edge_indices indices);

  [[nodiscard]]
  std::string odd_num_loops_error(std::string_view method, std::size_t nodeIndex);

  [[nodiscard]]
  std::string self_referential_error(edge_indices edgeIndices, std::size_t target, std::size_t compIndex);

  [[nodiscard]]
  std::string mismatched_weights_message(std::string_view method, edge_indices edgeIndices);

  /** \pre `counts.to_target` is non-zero and differs from `counts.from_target` */
  [[nodiscard]]
  std::string absent_reciprocated_partial_edge_message(std::string_view method,
                                                       partial_edge_counts counts,
                                                       edge_weighting weighting);

  [[nodiscard]]
  std::string absent_partner_weight_message(std::string_view method, edge_indices edgeIndices);
}
