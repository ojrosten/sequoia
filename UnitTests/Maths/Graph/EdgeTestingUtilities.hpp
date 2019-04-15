#pragma once

#include "UnitTestUtils.hpp"
#include "Edge.hpp"

namespace sequoia::unit_testing
{
  namespace impl
  {
    template<class Logger, class Edge>
    void check_partial(Logger& logger, const Edge& edge, const Edge& prediction, std::string_view description)
    {
      check_equality(logger, edge.target_node(), prediction.target_node(),
                       impl::concat_messages(description, "Target node incorrect"));
    
      if constexpr (!std::is_empty_v<typename Edge::weight_type>)
      {
        check_equality(logger, prediction.weight(), edge.weight(), impl::concat_messages(description, "Weight incorrect"));
      }
    }

    template<class Logger, class Edge>
    void check_complementary(Logger& logger, const Edge& edge, const Edge& prediction, std::string_view description)
    {
      check_equality(logger, edge.complementary_index(), prediction.complementary_index(),
                     impl::concat_messages(description, "Complementary index incorrect"));
    }
  
    template<class Logger, class Edge>
    void check_host(Logger& logger, const Edge& prediction, const Edge& edge, std::string_view description)
    {
      check_equality(logger, edge.host_node(), prediction.host_node(),
                         impl::concat_messages(description, "Host node incorrect"));

      check_equality(logger, edge.inverted(), prediction.inverted(),
                         impl::concat_messages(description, "Inversion flag incorrect")); 

    }
  }
  
  template
  <
    class Weight,
    template <class> class WeightSharingPolicy,
    class WeightProxy,
    class IndexType
  >
  struct details_checker<maths::partial_edge<Weight, WeightSharingPolicy, WeightProxy, IndexType>>
  {
    using type = maths::partial_edge<Weight, WeightSharingPolicy, WeightProxy, IndexType>;
    
    template<class Logger>
    static void check(Logger& logger, const type& prediction, const type& edge, std::string_view description="")
    {
      impl::check_partial(logger, edge, prediction, description);
    }
  };

  template
  <
    class Weight,
    template <class> class WeightSharingPolicy,
    class WeightProxy,
    class IndexType
  >
  struct details_checker<maths::embedded_partial_edge<Weight, WeightSharingPolicy, WeightProxy, IndexType>>
  {
    using type = maths::embedded_partial_edge<Weight, WeightSharingPolicy, WeightProxy, IndexType>;
    template<class Logger>
    static void check(Logger& logger, const type& prediction, const type& edge, std::string_view description="")
    {
      impl::check_partial(logger, edge, prediction, description);
      impl::check_complementary(logger, edge, prediction, description);
    }
  };

  template
  <
    class Weight,
    class WeightProxy,
    class IndexType
  >
  struct details_checker<maths::edge<Weight, WeightProxy, IndexType>>
  {
    using type = maths::edge<Weight, WeightProxy, IndexType>;
    
    template<class Logger>
    static void check(Logger& logger, const type& edge, const type& prediction, std::string_view description="")
    {
      impl::check_partial(logger, edge, prediction, description);
      impl::check_host(logger, edge, prediction, description);
    }
  };

  template
  <
    class Weight,
    template <class> class WeightSharingPolicy,
    class WeightProxy,
    class IndexType
  >
  struct details_checker<maths::embedded_edge<Weight, WeightSharingPolicy, WeightProxy, IndexType>>
  {
    using type = maths::embedded_edge<Weight, WeightSharingPolicy, WeightProxy, IndexType>;
    
    template<class Logger>
    static void check(Logger& logger, const type& edge, const type& prediction, std::string_view description="")
    {
      impl::check_partial(logger, edge, prediction, description);
      impl::check_complementary(logger, edge, prediction, description);
      impl::check_host(logger, edge, prediction, description);
    }
  };
}
