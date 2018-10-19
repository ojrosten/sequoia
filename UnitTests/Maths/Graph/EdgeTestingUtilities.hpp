#pragma once

#include "UnitTestUtils.hpp"
#include "Edge.hpp"

namespace sequoia::unit_testing
{
  namespace impl
  {
    template<class Logger, class Edge>
    bool check_partial(Logger& logger, const Edge& reference, const Edge& actual, const std::string& description)
    {
      bool equal{
        check_equality(logger, reference.target_node(), actual.target_node(),
                       impl::concat_messages(description, "Target node incorrect"))
          };
    
      if constexpr (!std::is_empty_v<typename Edge::weight_type>)
      {
        if(!check_equality(reference.weight, actual.weight(), impl::concat_messages(description, "Weight incorrect"))) equal = false;
      }

      return equal;
    }

    template<class Logger, class Edge>
    bool check_complementary(Logger& logger, const Edge& reference, const Edge& actual, const std::string& description)
    {
      return check_equality(logger, reference.complementary_index(), actual.complementary_index()),
        impl::concat_messages(description, "Complementary index incorrect");
    }
  
    template<class Logger, class Edge>
    bool check_host(Logger& logger, const Edge& reference, const Edge& actual, const std::string& description)
    {
      return check_equality(logger, reference.host_node(), actual.host_node()),
        impl::concat_messages(description, "Host node incorrect");
    }
  }
  
  template
  <
    class Weight,
    template <class> class WeightSharingPolicy,
    class WeightProxy,
    class IndexType
  >
  struct equality_checker<maths::partial_edge<Weight, WeightSharingPolicy, WeightProxy, IndexType>>
  {
    template<class Logger>
    static bool check(Logger& logger, const maths::partial_edge<Weight, WeightSharingPolicy, WeightProxy, IndexType>& reference, const maths::partial_edge<Weight, WeightSharingPolicy, WeightProxy, IndexType>& actual, const std::string& description="")
    {
      typename Logger::sentinel s{logger, description};
      return impl::check_partial(logger, reference, actual, description);
    }
  };

  template
  <
    class Weight,
    template <class> class WeightSharingPolicy,
    class WeightProxy,
    class IndexType
  >
  struct equality_checker<maths::embedded_partial_edge<Weight, WeightSharingPolicy, WeightProxy, IndexType>>
  {
    template<class Logger>
    static bool check(Logger& logger, const maths::embedded_partial_edge<Weight, WeightSharingPolicy, WeightProxy, IndexType>& reference, const maths::embedded_partial_edge<Weight, WeightSharingPolicy, WeightProxy, IndexType>& actual, const std::string& description="")
    {
      typename Logger::sentinel s{logger, description};
      bool equal{impl::check_partial(logger, reference, actual, description)};
      if(!impl::check_complementary(logger, reference, actual, description)) equal = false;

      return equal;
    }
  };

  template
  <
    class Weight,
    class WeightProxy,
    class IndexType
  >
  struct equality_checker<maths::edge<Weight, WeightProxy, IndexType>>
  {
    template<class Logger>
    static bool check(Logger& logger, const maths::edge<Weight, WeightProxy, IndexType>& reference, const maths::edge<Weight, WeightProxy, IndexType>& actual, const std::string& description="")
    {
      typename Logger::sentinel s{logger, description};
      bool equal{impl::check_partial(logger, reference, actual, description)};
      if(!impl::check_host(logger, reference, actual, description)) equal = false;

      return equal;
    }
  };

  template
  <
    class Weight,
    template <class> class WeightSharingPolicy,
    class WeightProxy,
    class IndexType
  >
  struct equality_checker<maths::embedded_edge<Weight, WeightSharingPolicy, WeightProxy, IndexType>>
  {
    template<class Logger>
    static bool check(Logger& logger, const maths::embedded_edge<Weight, WeightSharingPolicy, WeightProxy, IndexType>& reference, const maths::embedded_edge<Weight, WeightSharingPolicy, WeightProxy, IndexType>& actual, const std::string& description="")
    {
      typename Logger::sentinel s{logger, description};
      bool equal{impl::check_partial(logger, reference, actual, description)};
      if(!impl::check_complementary(logger, reference, actual, description)) equal = false;
      if(!impl::check_host(logger, reference, actual, description)) equal = false;
      
      return equal;
    }
  };
}
