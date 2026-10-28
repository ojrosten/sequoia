////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief A check, for any graph type, that `sort_nodes` keeps the order of
           the nodes its comparator treats as equivalent.
 */

#include "Maths/Graph/GraphTestingUtilities.hpp"

#include "sequoia/Core/Numeric/IntegerConversions.hpp"
#include "sequoia/TestFramework/RegularTestCore.hpp"
#include "sequoia/TestFramework/StateTransitionUtilities.hpp"

#include <algorithm>
#include <array>
#include <concepts>
#include <functional>

namespace sequoia::testing
{
  /** \brief The number of nodes in each graph whose sort is checked for
             stability.

      With the weights `0` to `stability_check_node_count - 1` in ascending
      order and `stability_check_equivalence_class_count` classes of them,
      std::ranges::sort, which need not be stable, first reorders equivalent
      nodes at 17 nodes under libstdc++ and at 25 under libc++.
   */
  inline constexpr std::size_t stability_check_node_count{64};

  /** \brief The number of classes of equivalent node weights in each check
             of stability: two weights are equivalent if their remainders on
             division by it are equal.
   */
  inline constexpr std::size_t stability_check_equivalence_class_count{4};

  static_assert(   (1 < stability_check_equivalence_class_count)
                && (stability_check_equivalence_class_count < stability_check_node_count));

  using stability_check_weights_type = std::array<std::size_t, stability_check_node_count>;

  /** \brief Returns the weights `0` to `stability_check_node_count - 1`, in
             ascending order.
   */
  [[nodiscard]]
  constexpr stability_check_weights_type ascending_stability_check_weights()
  {
    stability_check_weights_type weights{};
    std::ranges::iota(weights, 0uz);
    return weights;
  }

  /** \brief Returns `ascending_stability_check_weights()`, ordered by their
             remainders on division by
             `stability_check_equivalence_class_count`, and each class in
             ascending order.
   */
  [[nodiscard]]
  constexpr stability_check_weights_type stability_check_weights_by_remainder()
  {
    stability_check_weights_type weights{};
    std::size_t position{};
    for(std::size_t remainder{}; remainder < stability_check_equivalence_class_count; ++remainder)
    {
      for(auto weight{remainder};
          weight < stability_check_node_count;
          weight += stability_check_equivalence_class_count)
      {
        weights[position++] = weight;
      }
    }

    return weights;
  }

  /** \brief Returns a graph without edges whose nodes have `weights`. */
  template<maths::dynamic_network G>
    requires std::same_as<typename G::node_weight_type, std::size_t>
  [[nodiscard]]
  G edgeless_graph_with_node_weights(const stability_check_weights_type& weights)
  {
    G g{};
    for(const auto weight : weights)
    {
      g.add_node(weight);
    }

    return g;
  }

  /** \brief Returns a graph without edges whose nodes have `weights`. */
  template<maths::static_network G>
    requires    std::same_as<typename G::node_weight_type, std::size_t>
             && (G::size() == 0)
             && (G::order() == stability_check_node_count)
  [[nodiscard]]
  G edgeless_graph_with_node_weights(const stability_check_weights_type& weights)
  {
    G g{};
    std::ranges::copy(weights, g.node_weights().begin());
    return g;
  }

  /** \brief Checks, through `test`, that sorting the nodes of an edgeless
             graph of type `G` by the remainders of their weights on division
             by `stability_check_equivalence_class_count` keeps each class's
             nodes in their order, and that sorting them by weight restores
             them.
   */
  template<maths::network G>
  void check_sort_nodes_is_stable(regular_test& test)
  {
    enum graph_description { ascending_weights = 0, weights_by_remainder };

    using transition_graph = transition_checker<G>::transition_graph;

    auto weightOf{
      [](const G& g, std::size_t node) {
        return g.cbegin_node_weights()[numeric::unchecked_conversion_to<std::ptrdiff_t>(node)];
      }
    };

    transition_graph trg{
      {
        {  // begin 'graph_description::ascending_weights'
          {
            graph_description::weights_by_remainder,
            test.report("Sorting the nodes keeps the relative order of nodes the comparator treats as equivalent"),
            [weightOf](G g) -> G {
              g.sort_nodes([&g, weightOf](std::size_t i, std::size_t j) {
                return   (weightOf(g, i) % stability_check_equivalence_class_count)
                       < (weightOf(g, j) % stability_check_equivalence_class_count);
              });
              return g;
            }
          }
        }, // end 'graph_description::ascending_weights'
        {  // begin 'graph_description::weights_by_remainder'
          {
            graph_description::ascending_weights,
            test.report("Sorting the nodes by weight"),
            [weightOf](G g) -> G {
              g.sort_nodes([&g, weightOf](std::size_t i, std::size_t j) { return weightOf(g, i) < weightOf(g, j); });
              return g;
            }
          }
        }  // end 'graph_description::weights_by_remainder'
      },
      {
        // 'graph_description::ascending_weights'
        []() -> G { return edgeless_graph_with_node_weights<G>(ascending_stability_check_weights()); },

        // 'graph_description::weights_by_remainder'
        []() -> G { return edgeless_graph_with_node_weights<G>(stability_check_weights_by_remainder()); }
      }
    };

    auto checkerFn{
      [&test](std::string_view description, const G& obtained, const G& prediction, const G& parent) {
        test.check(equality, description, obtained, prediction);
        test.check_semantics(description, prediction, parent);
      }
    };

    transition_checker<G>::check(test.report(""), trg, checkerFn);
  }
}
