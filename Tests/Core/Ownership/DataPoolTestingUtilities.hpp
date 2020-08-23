////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "DataPool.hpp"

namespace sequoia::testing
{
  template<class T, class Allocator>
  struct weak_equivalence_checker<ownership::data_pool<T, Allocator>>
  {
    using type = ownership::data_pool<T, Allocator>;
    using prediction_type = std::initializer_list<std::pair<T, long>>;
    
    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const type& pool, prediction_type prediction)
    {
      check_equality("empty", logger, pool.empty(), prediction.size() == 0);
      check_equality("size", logger, pool.size(), prediction.size());
      
      check_range_equivalence("iterator", logger, pool.begin(), pool.end(), prediction.begin(), prediction.end());
      check_range_equivalence("citerator", logger, pool.cbegin(), pool.cend(), prediction.begin(), prediction.end());
      check_range_equivalence("riterator", logger, pool.rbegin(), pool.rend(), std::rbegin(prediction), std::rend(prediction));
      check_range_equivalence("criterator", logger, pool.crbegin(), pool.crend(), std::rbegin(prediction), std::rend(prediction));
    }
  };
}
