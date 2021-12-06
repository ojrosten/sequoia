////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/TestFramework/RegularTestCore.hpp"
#include "sequoia/Maths/Graph/Edge.hpp"

namespace sequoia::testing
{
  namespace impl
  {
    template<test_mode Mode, class Edge, class Prediction>
    void check_partial(test_logger<Mode>& logger, const Edge& edge, const Prediction& prediction)
    {
      check_equality("Target node incorrect", logger, edge.target_node(), prediction.target_node());

      if constexpr (!std::is_empty_v<typename Edge::weight_type>)
      {
        check_equality("Weight incorrect", logger, edge.weight(), prediction.weight());
      }
    }

    template<test_mode Mode, class Edge, class Prediction>
    void check_complementary(test_logger<Mode>& logger, const Edge& edge, const Prediction& prediction)
    {
      check_equality("Complementary index incorrect", logger, edge.complementary_index(), prediction.complementary_index());
    }

    template<test_mode Mode, class Edge, class Prediction>
    void check_source(test_logger<Mode>& logger, const Edge& edge, const Prediction& prediction)
    {
      check_equality("Host node incorrect", logger, edge.source_node(), prediction.source_node());
      check_equality("Inversion flag incorrect", logger, edge.inverted(), prediction.inverted());

    }
  }

  template<class WeightHandler, std::integral IndexType>
    requires ownership::handler<WeightHandler>
  struct value_checker<maths::partial_edge<WeightHandler, IndexType>>
  {
    using type = maths::partial_edge<WeightHandler, IndexType>;

    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const type& edge, const type& prediction)
    {
      impl::check_partial(logger, edge, prediction);
    }
  };


  template<class WeightHandler, std::integral IndexType>
    requires ownership::handler<WeightHandler>
  struct equivalence_checker<maths::partial_edge<WeightHandler, IndexType>>
  {
    using type = maths::partial_edge<WeightHandler, IndexType>;

    template<test_mode Mode, class OtherHandler>
      requires ownership::handler<OtherHandler>
    static void check(test_logger<Mode>& logger, const type& edge, const maths::partial_edge<OtherHandler, IndexType>& prediction)
    {
      impl::check_partial(logger, edge, prediction);
    }
  };


  template<class WeightHandler, std::integral IndexType>
    requires ownership::handler<WeightHandler>
  struct value_checker<maths::embedded_partial_edge<WeightHandler, IndexType>>
  {
    using type = maths::embedded_partial_edge<WeightHandler, IndexType>;

    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const type& edge, const type& prediction)
    {
      impl::check_partial(logger, edge, prediction);
      impl::check_complementary(logger, edge, prediction);
    }
  };

  template<class WeightHandler, std::integral IndexType>
    requires ownership::handler<WeightHandler>
  struct equivalence_checker<maths::embedded_partial_edge<WeightHandler, IndexType>>
  {
    using type = maths::embedded_partial_edge< WeightHandler,IndexType>;

    template<test_mode Mode, class OtherHandler>
      requires ownership::handler<OtherHandler>
    static void check(test_logger<Mode>& logger, const type& edge, const maths::embedded_partial_edge<OtherHandler, IndexType>& prediction)
    {
      impl::check_partial(logger, edge, prediction);
      impl::check_complementary(logger, edge, prediction);
    }
  };

  template<class WeightHandler, std::integral IndexType>
    requires ownership::handler<WeightHandler>
  struct value_checker<maths::edge<WeightHandler, IndexType>>
  {
    using type = maths::edge<WeightHandler, IndexType>;

    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const type& edge, const type& prediction)
    {
      impl::check_partial(logger, edge, prediction);
      impl::check_source(logger, edge, prediction);
    }
  };

  template<class WeightHandler, std::integral IndexType>
    requires ownership::handler<WeightHandler>
  struct equivalence_checker<maths::edge<WeightHandler, IndexType>>
  {
    using type = maths::edge<WeightHandler, IndexType>;

    template<test_mode Mode, class OtherHandler>
      requires ownership::handler<OtherHandler>
    static void check(test_logger<Mode>& logger, const type& edge, const maths::edge<OtherHandler, IndexType>& prediction)
    {
      impl::check_partial(logger, edge, prediction);
      impl::check_source(logger, edge, prediction);
    }
  };

  template<class WeightHandler, std::integral IndexType>
    requires ownership::handler<WeightHandler>
  struct value_checker<maths::embedded_edge<WeightHandler, IndexType>>
  {
    using type = maths::embedded_edge<WeightHandler, IndexType>;

    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const type& edge, const type& prediction)
    {
      impl::check_partial(logger, edge, prediction);
      impl::check_complementary(logger, edge, prediction);
      impl::check_source(logger, edge, prediction);
    }
  };

  template<class WeightHandler, std::integral IndexType>
    requires ownership::handler<WeightHandler>
  struct equivalence_checker<maths::embedded_edge<WeightHandler, IndexType>>
  {
    using type = maths::embedded_edge<WeightHandler, IndexType>;

    template<test_mode Mode, class OtherHandler>
      requires ownership::handler<OtherHandler>
    static void check(test_logger<Mode>& logger, const type& edge, const maths::embedded_edge<OtherHandler, IndexType>& prediction)
    {
      impl::check_partial(logger, edge, prediction);
      impl::check_complementary(logger, edge, prediction);
      impl::check_source(logger, edge, prediction);
    }
  };

  template<class WeightHandler, std::integral IndexType>
    requires ownership::handler<WeightHandler>
  struct weak_equivalence_checker<maths::edge<WeightHandler, IndexType>>
  {
    using type = maths::edge<WeightHandler, IndexType>;

    template<test_mode Mode, class PredictionType>
    static void check(test_logger<Mode>& logger, const type& edge, const PredictionType& prediction)
    {
      impl::check_partial(logger, edge, prediction);
      impl::check_source(logger, edge, prediction);
    }
  };
}
