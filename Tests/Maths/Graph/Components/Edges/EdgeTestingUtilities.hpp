////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/RegularTestCore.hpp"
#include "sequoia/Maths/Graph/Edge.hpp"

namespace sequoia::testing
{
  namespace impl
  {
    template<class CheckerType, test_mode Mode, class Edge, class Prediction>
    void check_partial([[maybe_unused]] CheckerType flavour, test_logger<Mode>& logger, const Edge& edge, const Prediction& prediction)
    {
      check(equality, "Target node incorrect", logger, edge.target_node(), prediction.target_node());

      if constexpr (!std::is_empty_v<typename Edge::weight_type>)
      {
        check(flavour, "Weight incorrect", logger, edge.weight(), prediction.weight());
      }

      if constexpr(!std::is_empty_v<typename Edge::meta_data_type>)
      {
        check(flavour, "Meta data incorrect", logger, edge.meta_data(), prediction.meta_data());
      }
    }

    template<test_mode Mode, class Edge, class Prediction>
    void check_complementary(test_logger<Mode>& logger, const Edge& edge, const Prediction& prediction)
    {
      check(equality, "Complementary index incorrect", logger, edge.complementary_index(), prediction.complementary_index());
    }
  }

  template<class WeightHandler, class MetaData, std::integral IndexType>
    requires object::handler<WeightHandler>
  struct value_tester<maths::partial_edge<WeightHandler, MetaData, IndexType>>
  {
    using type        = maths::partial_edge<WeightHandler, MetaData, IndexType>;
    using weight_type = typename type::weight_type;

    template<class CheckerType, test_mode Mode>
    static void test(CheckerType flavour, test_logger<Mode>& logger, const type& edge, const type& prediction)
    {
      impl::check_partial(flavour, logger, edge, prediction);
    }

    template<test_mode Mode, class OtherHandler>
      requires (object::handler<OtherHandler> && !std::is_same_v<WeightHandler, OtherHandler>)
    static void test(equivalence_check_t, test_logger<Mode>& logger, const type& edge, const maths::partial_edge<OtherHandler, MetaData, IndexType>& prediction)
    {
      impl::check_partial(with_best_available, logger, edge, prediction);
    }

    template<test_mode Mode>
      requires std::is_empty_v<weight_type> && std::is_empty_v<MetaData>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const type& edge, const IndexType& target)
    {
      check(equality, "Target node incorrect", logger, edge.target_node(), target);
    }

    template<test_mode Mode>
      requires (!std::is_empty_v<weight_type>) && std::is_empty_v<MetaData>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const type& edge, const std::pair<IndexType, weight_type>& prediction)
    {
      check(equality, "Target node incorrect", logger, edge.target_node(), prediction.first);
      check(equality, "Weight incorrect", logger, edge.weight(), prediction.second);
    }

    template<test_mode Mode>
      requires std::is_empty_v<weight_type> && (!std::is_empty_v<MetaData>)
    static void test(equivalence_check_t, test_logger<Mode>& logger, const type& edge, const std::pair<IndexType, MetaData>& prediction)
    {
      check(equality, "Target node incorrect", logger, edge.target_node(), prediction.first);
      check(equality, "Weight incorrect", logger, edge.meta_data(), prediction.second);
    }

    template<test_mode Mode>
      requires (!std::is_empty_v<weight_type>) && (!std::is_empty_v<MetaData>)
    static void test(equivalence_check_t, test_logger<Mode>& logger, const type& edge, const std::tuple<IndexType, MetaData, weight_type>& prediction)
    {
      check(equality, "Target node incorrect", logger, edge.target_node(), std::get<0>(prediction));
      check(equality, "Weight incorrect", logger, edge.weight(), std::get<2>(prediction));
      check(equality, "Meta data incorrect", logger, edge.meta_data(), std::get<1>(prediction));
    }
  };

  template<class WeightHandler, class MetaData, std::integral IndexType>
    requires object::handler<WeightHandler>
  struct value_tester<maths::embedded_partial_edge<WeightHandler, MetaData, IndexType>>
  {
    using type = maths::embedded_partial_edge<WeightHandler, MetaData, IndexType>;
    using weight_type = typename type::weight_type;

    template<class CheckerType, test_mode Mode>
    static void test(CheckerType flavour, test_logger<Mode>& logger, const type& edge, const type& prediction)
    {
      impl::check_partial(flavour, logger, edge, prediction);
      impl::check_complementary(logger, edge, prediction);
    }

    template<test_mode Mode, class OtherHandler>
      requires (object::handler<OtherHandler> && !std::is_same_v<WeightHandler, OtherHandler>)
    static void test(equivalence_check_t, test_logger<Mode>& logger, const type& edge, const maths::embedded_partial_edge<OtherHandler, MetaData, IndexType>& prediction)
    {
      impl::check_partial(with_best_available, logger, edge, prediction);
      impl::check_complementary(logger, edge, prediction);
    }

    template<test_mode Mode>
      requires std::is_empty_v<weight_type>&& std::is_empty_v<MetaData>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const type& edge, const std::pair<IndexType, IndexType>& prediction)
    {
      check(equality, "Target node incorrect", logger, edge.target_node(), prediction.first);
      check(equality, "Complementary index incorrect", logger, edge.complementary_index(), prediction.second);
    }

    template<test_mode Mode>
      requires (!std::is_empty_v<weight_type>) && std::is_empty_v<MetaData>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const type& edge, const std::tuple<IndexType, IndexType, weight_type>& prediction)
    {
      check(equality, "Target node incorrect", logger, edge.target_node(), std::get<0>(prediction));
      check(equality, "Complementary index incorrect", logger, edge.complementary_index(), std::get<1>(prediction));
      check(equality, "Weight incorrect", logger, edge.weight(), std::get<2>(prediction));
    }

    template<test_mode Mode>
      requires std::is_empty_v<weight_type> && (!std::is_empty_v<MetaData>)
    static void test(equivalence_check_t, test_logger<Mode>& logger, const type& edge, const std::tuple<IndexType, IndexType, MetaData>& prediction)
    {
      check(equality, "Target node incorrect", logger, edge.target_node(), std::get<0>(prediction));
      check(equality, "Complementary index incorrect", logger, edge.complementary_index(), std::get<1>(prediction));
      check(equality, "Weight incorrect", logger, edge.meta_data(), std::get<2>(prediction));
    }

    template<test_mode Mode>
      requires (!std::is_empty_v<weight_type>) && (!std::is_empty_v<MetaData>)
    static void test(equivalence_check_t, test_logger<Mode>& logger, const type& edge, const std::tuple<IndexType, IndexType, MetaData, weight_type>& prediction)
    {
      check(equality, "Target node incorrect", logger, edge.target_node(), std::get<0>(prediction));
      check(equality, "Complementary index incorrect", logger, edge.complementary_index(), std::get<1>(prediction));
      check(equality, "Weight incorrect", logger, edge.weight(), std::get<3>(prediction));
      check(equality, "Meta data incorrect", logger, edge.meta_data(), std::get<2>(prediction));
    }
  };
}
