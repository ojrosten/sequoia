////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for GraphError.hpp

 */

#include "sequoia/Maths/Graph/GraphErrors.hpp"

namespace sequoia::maths::graph_errors
{
  namespace
  {
    [[nodiscard]]
    std::string to_string(const edge_indices& indices)
    {
      return std::string{"[node: "}.append(std::to_string(indices.node)).append(", edge: ").append(std::to_string(indices.edge)).append("]");
    }

    [[nodiscard]]
    std::string prefix(std::string_view method)
    {
      return std::string{"connectivity::"}.append(method);
    }

    [[nodiscard]]
    std::string error_prefix(std::string_view method)
    {
      return prefix(method).append(": ");
    }

    [[nodiscard]]
    std::string error_prefix(std::string_view method, const edge_indices indices)
    {
      return prefix(method).append(" ").append(to_string(indices)).append(": ");
    }
  }

  [[nodiscard]]
  std::string node_index_range_message (std::string_view method, const std::size_t order, const std::size_t node)
  {
    return error_prefix(method).append("node index ").append(std::to_string(node)).append(" out of range - graph order is ").append(std::to_string(order));
  }

  [[nodiscard]]
  std::string node_index_range_message(std::string_view method, const std::size_t order, const std::size_t node1, const std::size_t node2)
  {
    return error_prefix(method).append("at least one node index [")
            .append(std::to_string(node1)).append(", ").append(std::to_string(node2))
            .append("] out of range - graph order is ").append(std::to_string(order));
  }

  [[nodiscard]]
  std::string edge_index_range_message(std::string_view method, const edge_indices edgeIndices, std::string_view indexName, const std::size_t size, const std::size_t index)
  {
    return error_prefix(method, edgeIndices).append(indexName).append(" index ").append(std::to_string(index)).append(" out of range - max index is ").append(std::to_string(size));
  }

  [[nodiscard]]
  std::string edge_insertion_index_message(std::string_view method, const std::size_t node, const std::size_t sizeAfterFirstInsertion, const std::size_t index)
  {
    return error_prefix(method).append("insertion position ").append(std::to_string(index))
            .append(" out of range for node ").append(std::to_string(node)).append(" - max index is ").append(std::to_string(sizeAfterFirstInsertion));
  }

  [[nodiscard]]
  std::string edge_swap_indices_message(std::size_t node, std::size_t index, std::size_t numEdges)
  {
    return std::string{"swap_edges: edge index "}.append(std::to_string(index)).append(" out of range for node ")
      .append(std::to_string(node)).append(", which has ").append(std::to_string(numEdges)).append(" edge(s)");
  }

  [[nodiscard]]
  std::string reciprocated_error_message(const edge_indices edgeIndices, const std::string_view indexName, const std::size_t reciprocatedIndex, const std::size_t index)
  {
    return error_prefix("process_complementary_edges", edgeIndices).append("Reciprocated ").append(indexName)
      .append(" index ").append(std::to_string(reciprocatedIndex)).append(" does not match ").append(std::to_string(index));
  }

  [[nodiscard]]
  std::string embedded_edge_message(const std::size_t nodeIndex, const std::size_t source, const std::size_t target)
  {
    return error_prefix("process_complementary_edges").append("At least one of source ").append(std::to_string(source))
      .append(" and target ").append(std::to_string(target)).append(" must match current node ").append(std::to_string(nodeIndex));
  }

  [[nodiscard]]
  std::string erase_edge_error(const std::size_t partner, const edge_indices indices)
  {
    return error_prefix("erase_edge")
      .append("partner in partition ").append(std::to_string(partner)).append(" not found for edge ").append(to_string(indices));
  }

  [[nodiscard]]
  std::string odd_num_loops_error(std::string_view method, std::size_t nodeIndex)
  {
    return error_prefix(method).append("Odd number of loop edges for node ").append(std::to_string(nodeIndex));
  }

  [[nodiscard]]
  std::string self_referential_error(const edge_indices edgeIndices, const std::size_t target, const std::size_t compIndex)
  {
    return error_prefix("process_complementary_edges", edgeIndices).append("Indices [target: ").append(std::to_string(target)).append(", comp: ").append(std::to_string(compIndex)).append("] are self-referential");
  }

  [[nodiscard]]
  std::string mismatched_weights_message(std::string_view method, edge_indices edgeIndices)
  {
    return error_prefix(method, edgeIndices).append("Mismatch between weights");
  }

  [[nodiscard]]
  std::string absent_reciprocated_partial_edge_message(std::string_view method, edge_indices edgeIndices)
  {
    return error_prefix(method, edgeIndices).append("Reciprocated partial edge does not exist");
  }

  [[nodiscard]]
  std::string absent_partner_weight_message(std::string_view method, edge_indices edgeIndices)
  {
    return error_prefix(method, edgeIndices).append("Partner weight not found");
  }

  [[nodiscard]]
  std::string inconsistent_initialization_message(std::size_t numNodes, std::size_t edgeParitions)
  {
    return std::string{"Error initializing graph\n"}
            .append("Number of node weights:    ").append(std::to_string(numNodes)).append("\n")
            .append("Number of edge paritions:  ").append(std::to_string(edgeParitions)).append("\n")
            .append("Please ensure these numbers are the same");
  }

}
