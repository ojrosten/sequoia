////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/Maths/Graph/GraphErrors.hpp"

#include <format>

namespace sequoia::maths::graph_errors
{
  namespace
  {
    [[nodiscard]]
    std::string to_string(const edge_indices& indices)
    {
      return std::format("[node: {}, edge: {}]", indices.node, indices.edge);
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
      return std::format("{} {}: ", prefix(method), to_string(indices));
    }
  }

  [[nodiscard]]
  std::string node_index_range_message (std::string_view method, const std::size_t order, const std::size_t node)
  {
    return std::format("{}node index {} out of range - graph order is {}", error_prefix(method), node, order);
  }

  [[nodiscard]]
  std::string node_index_range_message(std::string_view method, const std::size_t order, const std::size_t node1, const std::size_t node2)
  {
    return std::format("{}at least one node index [{}, {}] out of range - graph order is {}",
                       error_prefix(method),
                       node1,
                       node2,
                       order);
  }

  [[nodiscard]]
  std::string edge_index_range_message(std::string_view method, const edge_indices edgeIndices, std::string_view indexName, const std::size_t size, const std::size_t index)
  {
    return std::format("{}{} index {} out of range - max index is {}",
                       error_prefix(method, edgeIndices),
                       indexName,
                       index,
                       size);
  }

  [[nodiscard]]
  std::string edge_insertion_index_message(std::string_view method, const std::size_t node, const std::size_t sizeAfterFirstInsertion, const std::size_t index)
  {
    return std::format("{}insertion position {} out of range for node {} - max index is {}",
                       error_prefix(method),
                       index,
                       node,
                       sizeAfterFirstInsertion);
  }

  [[nodiscard]]
  std::string edge_swap_indices_message(std::size_t node, std::size_t index, std::size_t numEdges)
  {
    return std::format("swap_edges: edge index {} out of range for node {}, which has {} edge(s)",
                       index,
                       node,
                       numEdges);
  }

  [[nodiscard]]
  std::string reciprocated_error_message(const edge_indices edgeIndices, const std::string_view indexName, const std::size_t reciprocatedIndex, const std::size_t index)
  {
    return std::format("{}Reciprocated {} index {} does not match {}",
                       error_prefix("process_complementary_edges", edgeIndices),
                       indexName,
                       reciprocatedIndex,
                       index);
  }

  [[nodiscard]]
  std::string embedded_edge_message(const std::size_t nodeIndex, const std::size_t source, const std::size_t target)
  {
    return std::format("{}At least one of source {} and target {} must match current node {}",
                       error_prefix("process_complementary_edges"),
                       source,
                       target,
                       nodeIndex);
  }

  [[nodiscard]]
  std::string erase_edge_error(const std::size_t partner, const edge_indices indices)
  {
    return std::format("{}partner in partition {} not found for edge {}",
                       error_prefix("erase_edge"),
                       partner,
                       to_string(indices));
  }

  [[nodiscard]]
  std::string odd_num_loops_error(std::string_view method, std::size_t nodeIndex)
  {
    return std::format("{}Odd number of loop edges for node {}", error_prefix(method), nodeIndex);
  }

  [[nodiscard]]
  std::string self_referential_error(const edge_indices edgeIndices, const std::size_t target, const std::size_t compIndex)
  {
    return std::format("{}Indices [target: {}, comp: {}] are self-referential",
                       error_prefix("process_complementary_edges", edgeIndices),
                       target,
                       compIndex);
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
    return std::format("Error initializing graph\n"
                       "Number of node weights:    {}\n"
                       "Number of edge paritions:  {}\n"
                       "Please ensure these numbers are the same",
                       numNodes,
                       edgeParitions);
  }

  [[nodiscard]]
  std::string inversion_consistency_message(std::size_t nodeIndex, edge_inversion_info zerothEdge, edge_inversion_info firstEdge)
  {
    auto toString{
      [](edge_inversion_info info){
        return std::format("{} / {}", info.edge, info.inverted ? "inverted" : "standard");
      }
    };

    return std::format("{}mismatched inverson for node {}, edges ({}, {})",
                       error_prefix("process_complementary_edges"),
                       nodeIndex,
                       toString(zerothEdge),
                       toString(firstEdge));
  }
}
