#pragma once

#include "UnitTestUtils.hpp"
#include "Edge.hpp"

namespace sequoia::unit_testing
{
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
      bool equal{true};
      if(!check_equality(logger, reference.target_node(), actual.target_node(), impl::concat_messages(description, "Target node incorrect")))
        equal = false;
    
      if constexpr (!std::is_empty_v<typename maths::partial_edge<Weight, WeightSharingPolicy, WeightProxy, IndexType>::weight_type>)
      {
        if(!check_equality(reference.weight, actual.weight(), impl::concat_messages(description, "Weight incorrect"))) equal = false;
      }

      return equal;
    }
  };
}
