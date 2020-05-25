////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "RegularTestCore.hpp"
#include "Edge.hpp"

namespace sequoia::testing
{
  namespace impl
  {
    template<test_mode Mode, class Edge, class Prediction>
    void check_partial(std::string_view description, test_logger<Mode>& logger, const Edge& edge, const Prediction& prediction)
    {
      check_equality(combine_messages(description, "Target node incorrect"), logger, edge.target_node(), prediction.target_node());
    
      if constexpr (!std::is_empty_v<typename Edge::weight_type>)
      {
        check_equality(combine_messages(description, "Weight incorrect"), logger, edge.weight(), prediction.weight());
      }
    }

    template<test_mode Mode, class Edge, class Prediction>
    void check_complementary(std::string_view description, test_logger<Mode>& logger, const Edge& edge, const Prediction& prediction)
    {
      check_equality(combine_messages(description, "Complementary index incorrect"), logger, edge.complementary_index(), prediction.complementary_index());
    }
  
    template<test_mode Mode, class Edge, class Prediction>
    void check_source(std::string_view description, test_logger<Mode>& logger, const Edge& edge, const Prediction& prediction)
    {
      check_equality(combine_messages(description, "Host node incorrect"), logger, edge.source_node(), prediction.source_node());
      check_equality(combine_messages(description, "Inversion flag incorrect"), logger, edge.inverted(), prediction.inverted()); 

    }
  }
  
  template
  <
    class Weight,
    template <class> class WeightSharingPolicy,
    class WeightProxy,
    class IndexType
  >
  struct detailed_equality_checker<maths::partial_edge<Weight, WeightSharingPolicy, WeightProxy, IndexType>>
  {
    using type = maths::partial_edge<Weight, WeightSharingPolicy, WeightProxy, IndexType>;
    
    template<test_mode Mode>
    static void check(std::string_view description, test_logger<Mode>& logger, const type& edge, const type& prediction)
    {
      impl::check_partial(description, logger, edge, prediction);
    }
  };


  template
  <
    class Weight,
    template <class> class WeightSharingPolicy,
    class WeightProxy,
    class IndexType
  >
  struct equivalence_checker<maths::partial_edge<Weight, WeightSharingPolicy, WeightProxy, IndexType>>
  {
    using type = maths::partial_edge<Weight, WeightSharingPolicy, WeightProxy, IndexType>;
    
    template<test_mode Mode, template <class> class OtherWSPolicy, class OtherWProxy>
    static void check(std::string_view description, test_logger<Mode>& logger, const type& edge, const maths::partial_edge<Weight, OtherWSPolicy, OtherWProxy, IndexType>& prediction)
    {
      impl::check_partial(description, logger, edge, prediction);
    }
  };
  

  template
  <
    class Weight,
    template <class> class WeightSharingPolicy,
    class WeightProxy,
    class IndexType
  >
  struct detailed_equality_checker<maths::embedded_partial_edge<Weight, WeightSharingPolicy, WeightProxy, IndexType>>
  {
    using type = maths::embedded_partial_edge<Weight, WeightSharingPolicy, WeightProxy, IndexType>;
    
    template<test_mode Mode>
    static void check(std::string_view description, test_logger<Mode>& logger, const type& edge, const type& prediction)
    {
      impl::check_partial(description, logger, edge, prediction);
      impl::check_complementary(description, logger, edge, prediction);
    }
  };

  template
  <
    class Weight,
    template <class> class WeightSharingPolicy,
    class WeightProxy,
    class IndexType
  >
  struct equivalence_checker<maths::embedded_partial_edge<Weight, WeightSharingPolicy, WeightProxy, IndexType>>
  {
    using type = maths::embedded_partial_edge<Weight, WeightSharingPolicy, WeightProxy, IndexType>;
    
    template<test_mode Mode, template <class> class OtherWSPolicy, class OtherWProxy>
    static void check(std::string_view description, test_logger<Mode>& logger, const type& edge, const maths::embedded_partial_edge<Weight, OtherWSPolicy, OtherWProxy, IndexType>& prediction)
    {
      impl::check_partial(description, logger, edge, prediction);
      impl::check_complementary(description, logger, edge, prediction);
    }
  };

  template
  <
    class Weight,
    class WeightProxy,
    class IndexType
  >
  struct detailed_equality_checker<maths::edge<Weight, WeightProxy, IndexType>>
  {
    using type = maths::edge<Weight, WeightProxy, IndexType>;
    
    template<test_mode Mode>
    static void check(std::string_view description, test_logger<Mode>& logger, const type& edge, const type& prediction)
    {
      impl::check_partial(description, logger, edge, prediction);
      impl::check_source(description, logger, edge, prediction);
    }
  };

  template
  <
    class Weight,
    class WeightProxy,
    class IndexType
  >
  struct equivalence_checker<maths::edge<Weight, WeightProxy, IndexType>>
  {
    using type = maths::edge<Weight, WeightProxy, IndexType>;
    
    template<test_mode Mode, class OtherWProxy>
    static void check(std::string_view description, test_logger<Mode>& logger, const type& edge, const maths::edge<Weight, OtherWProxy, IndexType>& prediction)
    {
      impl::check_partial(description, logger, edge, prediction);
      impl::check_source(description, logger, edge, prediction);
    }
  };

  template
  <
    class Weight,
    template <class> class WeightSharingPolicy,
    class WeightProxy,
    class IndexType
  >
  struct detailed_equality_checker<maths::embedded_edge<Weight, WeightSharingPolicy, WeightProxy, IndexType>>
  {
    using type = maths::embedded_edge<Weight, WeightSharingPolicy, WeightProxy, IndexType>;
    
    template<test_mode Mode>
    static void check(std::string_view description, test_logger<Mode>& logger, const type& edge, const type& prediction)
    {
      impl::check_partial(description, logger, edge, prediction);
      impl::check_complementary(description, logger, edge, prediction);
      impl::check_source(description, logger, edge, prediction);
    }
  };

  template
  <
    class Weight,
    template <class> class WeightSharingPolicy,
    class WeightProxy,
    class IndexType
  >
  struct equivalence_checker<maths::embedded_edge<Weight, WeightSharingPolicy, WeightProxy, IndexType>>
  {
    using type = maths::embedded_edge<Weight, WeightSharingPolicy, WeightProxy, IndexType>;
    
    template<test_mode Mode, template <class> class OtherWSPolicy, class OtherWProxy>
    static void check(std::string_view description, test_logger<Mode>& logger, const type& edge, const maths::embedded_edge<Weight, OtherWSPolicy, OtherWProxy, IndexType>& prediction)
    {
      impl::check_partial(description, logger, edge, prediction);
      impl::check_complementary(description, logger, edge, prediction);
      impl::check_source(description, logger, edge, prediction);
    }
  };
  
  template
  <
    class Weight,
    class WeightProxy,
    class IndexType
  >
  struct weak_equivalence_checker<maths::edge<Weight, WeightProxy, IndexType>>
  {
    using type = maths::edge<Weight, WeightProxy, IndexType>;
    
    template<test_mode Mode, class PredictionType>
    static void check(std::string_view description, test_logger<Mode>& logger, const type& edge, const PredictionType& prediction)
    {
      impl::check_partial(description, logger, edge, prediction);
      impl::check_source(description, logger, edge, prediction);
    }
  };
}
