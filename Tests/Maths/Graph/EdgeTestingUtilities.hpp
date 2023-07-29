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
    }

    template<test_mode Mode, class Edge, class Prediction>
    void check_complementary(test_logger<Mode>& logger, const Edge& edge, const Prediction& prediction)
    {
      check(equality, "Complementary index incorrect", logger, edge.complementary_index(), prediction.complementary_index());
    }

    template<test_mode Mode, class Edge, class Prediction>
    void check_source(test_logger<Mode>& logger, const Edge& edge, const Prediction& prediction)
    {
      check(equality, "Host node incorrect", logger, edge.source_node(), prediction.source_node());
      check(equality, "Inversion flag incorrect", logger, edge.inverted(), prediction.inverted());
    }
  }

  template<class WeightHandler, std::integral IndexType>
    requires object::handler<WeightHandler>
  struct value_tester<maths::partial_edge<WeightHandler, IndexType>>
  {
    using type = maths::partial_edge<WeightHandler, IndexType>;

    template<class CheckerType, test_mode Mode>
    static void test(CheckerType flavour, test_logger<Mode>& logger, const type& edge, const type& prediction)
    {
      impl::check_partial(flavour, logger, edge, prediction);
    }

    template<test_mode Mode, class OtherHandler>
      requires (object::handler<OtherHandler> && !std::is_same_v<WeightHandler, OtherHandler>)
    static void test(equivalence_check_t, test_logger<Mode>& logger, const type& edge, const maths::partial_edge<OtherHandler, IndexType>& prediction)
    {
      impl::check_partial(with_best_available, logger, edge, prediction);
    }
  };

  template<class WeightHandler, std::integral IndexType>
    requires object::handler<WeightHandler>
  struct value_tester<maths::embedded_partial_edge<WeightHandler, IndexType>>
  {
    using type = maths::embedded_partial_edge<WeightHandler, IndexType>;

    template<class CheckerType, test_mode Mode>
    static void test(CheckerType flavour, test_logger<Mode>& logger, const type& edge, const type& prediction)
    {
      impl::check_partial(flavour, logger, edge, prediction);
      impl::check_complementary(logger, edge, prediction);
    }

    template<test_mode Mode, class OtherHandler>
      requires (object::handler<OtherHandler> && !std::is_same_v<WeightHandler, OtherHandler>)
    static void test(equivalence_check_t, test_logger<Mode>& logger, const type& edge, const maths::embedded_partial_edge<OtherHandler, IndexType>& prediction)
    {
      impl::check_partial(with_best_available, logger, edge, prediction);
      impl::check_complementary(logger, edge, prediction);
    }
  };

  template<class WeightHandler, std::integral IndexType>
    requires object::handler<WeightHandler>
  struct value_tester<maths::edge<WeightHandler, IndexType>>
  {
    using type = maths::edge<WeightHandler, IndexType>;

    template<class CheckerType, test_mode Mode>
    static void test(CheckerType flavour, test_logger<Mode>& logger, const type& edge, const type& prediction)
    {
      impl::check_partial(flavour, logger, edge, prediction);
      impl::check_source(logger, edge, prediction);
    }

    template<test_mode Mode, class OtherHandler>
      requires (object::handler<OtherHandler> && !std::is_same_v<WeightHandler, OtherHandler>)
    static void test(equivalence_check_t, test_logger<Mode>& logger, const type& edge, const maths::edge<OtherHandler, IndexType>& prediction)
    {
      impl::check_partial(with_best_available, logger, edge, prediction);
      impl::check_source(logger, edge, prediction);
    }

    template<test_mode Mode, class PredictionType>
    static void test(weak_equivalence_check_t, test_logger<Mode>& logger, const type& edge, const PredictionType& prediction)
    {
      impl::check_partial(with_best_available, logger, edge, prediction);
      impl::check_source(logger, edge, prediction);
    }
  };

  template<class WeightHandler, std::integral IndexType>
    requires object::handler<WeightHandler>
  struct value_tester<maths::embedded_edge<WeightHandler, IndexType>>
  {
    using type = maths::embedded_edge<WeightHandler, IndexType>;

    template<class CheckerType, test_mode Mode>
    static void test(CheckerType flavour, test_logger<Mode>& logger, const type& edge, const type& prediction)
    {
      impl::check_partial(flavour, logger, edge, prediction);
      impl::check_complementary(logger, edge, prediction);
      impl::check_source(logger, edge, prediction);
    }

    template<test_mode Mode, class OtherHandler>
      requires (object::handler<OtherHandler> && !std::is_same_v<WeightHandler, OtherHandler>)
    static void test(equivalence_check_t, test_logger<Mode>& logger, const type& edge, const maths::embedded_edge<OtherHandler, IndexType>& prediction)
    {
      impl::check_partial(with_best_available, logger, edge, prediction);
      impl::check_complementary(logger, edge, prediction);
      impl::check_source(logger, edge, prediction);
    }
  };
}
