////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "EdgeTestingUtilities.hpp"
#include "NodeStorageTestingUtilities.hpp"

#include "sequoia/Core/Object/FaithfulWrapper.hpp"
#include "sequoia/Core/DataStructures/PartitionedData.hpp"
#include "sequoia/Core/Object/DataPool.hpp"
#include "sequoia/Maths/Graph/GraphImpl.hpp"
#include "sequoia/Maths/Graph/GraphTraits.hpp"

namespace sequoia::testing
{
  template
  <
    maths::directed_flavour Directedness,
    class EdgeTraits,
    class WeightMaker
  >
  struct value_tester<maths::connectivity<Directedness, EdgeTraits, WeightMaker>>
  {
    using type            = maths::connectivity<Directedness, EdgeTraits, WeightMaker>;
    using edge_index_type = typename type::edge_index_type;

    template<class E>
    using connectivity_equivalent_type = std::initializer_list<std::initializer_list<E>>;

    template<class CheckType, test_mode Mode>
    static void test(CheckType flavour, test_logger<Mode>& logger, const type& connectivity, const type& prediction)
    {
      check(equality, "Connectivity size incorrect", logger, connectivity.size(), prediction.size());

      if(check(equality, "Connectivity order incorrect", logger, connectivity.order(), prediction.order()))
      {
        for(edge_index_type i{}; i<connectivity.order(); ++i)
        {
          const auto message{std::string{"Partition "}.append(std::to_string(i))};
          check(flavour, append_lines(message, "cedge_iterator"), logger, connectivity.cbegin_edges(i), connectivity.cend_edges(i), prediction.cbegin_edges(i), prediction.cend_edges(i));
          check(flavour, append_lines(message, "credge_iterator"), logger, connectivity.crbegin_edges(i), connectivity.crend_edges(i), prediction.crbegin_edges(i), prediction.crend_edges(i));
        }
      }
    }

    template<class CheckType, test_mode Mode, class E>
    static void test(CheckType flavour, test_logger<Mode>& logger, const type& connectivity, connectivity_equivalent_type<E> prediction)
    {
      check_connectivity(flavour, logger, connectivity, prediction);
    }
  private:
    template<class CheckType, test_mode Mode, class E>
    static void check_connectivity(CheckType flavour, test_logger<Mode>& logger, const type& connectivity, connectivity_equivalent_type<E> prediction)
    {
      if(check(equality, "Connectivity order incorrect", logger, connectivity.order(), prediction.size()))
      {
        for(edge_index_type i{}; i < connectivity.order(); ++i)
        {
          const auto message{"Partition " + std::to_string(i)};
          check(flavour, append_lines(message, "cedge_iterator"), logger, connectivity.cbegin_edges(i), connectivity.cend_edges(i), (prediction.begin() + i)->begin(), (prediction.begin() + i)->end());
        }
      }
    }
  };

  template<maths::network Graph>
  struct graph_value_tester_base
  {
    using type = Graph;

    template<class E>
    using connectivity_equivalent_type = std::initializer_list<std::initializer_list<E>>;

    using connectivity_type = typename type::connectivity_type;
    using nodes_type = typename type::nodes_type;

    template<class CheckType, test_mode Mode>
    static void test(CheckType flavour, test_logger<Mode>& logger, const Graph& graph, const Graph& prediction)
    {
      check(flavour, "", logger, static_cast<const connectivity_type&>(graph), static_cast<const connectivity_type&>(prediction));
      check(flavour, "", logger, static_cast<const nodes_type &>(graph), static_cast<const nodes_type&>(prediction));
    }

    template<class CheckType, test_mode Mode, class E, class NodesEquivalentType>
      requires (!std::is_empty_v<nodes_type>)
    static void test(CheckType flavour, test_logger<Mode>& logger, const type& graph, connectivity_equivalent_type<E> connPrediction, NodesEquivalentType&& nodesPrediction)
    {
      check(flavour, "", logger, static_cast<const connectivity_type&>(graph), connPrediction);
      check(flavour, "", logger, static_cast<const nodes_type&>(graph), std::forward<NodesEquivalentType>(nodesPrediction));
    }

    template<class CheckType, test_mode Mode, class E>
      requires std::is_empty_v<nodes_type>
    static void test(CheckType flavour, test_logger<Mode>& logger, const type& graph, connectivity_equivalent_type<E> connPrediction)
    {
      check(flavour, "", logger, static_cast<const connectivity_type&>(graph), connPrediction);
    }
  };

  template<maths::network Graph>
  struct value_tester<Graph> : graph_value_tester_base<Graph>
  {};

  constexpr bool mutual_info(const maths::graph_flavour flavour) noexcept
  {
    return flavour != maths::graph_flavour::directed;
  }

  struct unsortable
  {
    int x{};

    [[nodiscard]]
    friend constexpr bool operator==(const unsortable& lhs, const unsortable& rhs) noexcept = default;

    template<class Stream> friend Stream& operator<<(Stream& s, const unsortable& u)
    {
      s << std::to_string(u.x);
      return s;
    }
  };

  struct big_unsortable
  {
    int w{}, x{1}, y{2}, z{3};

    friend constexpr bool operator==(const big_unsortable& lhs, const big_unsortable& rhs) noexcept = default;

    template<class Stream> friend Stream& operator<<(Stream& s, const big_unsortable& u)
    {
      s << std::to_string(u.w) << ' ' << std::to_string(u.x) << ' ' << std::to_string(u.y) << ' ' << std::to_string(u.z);
      return s;
    }
  };
}
