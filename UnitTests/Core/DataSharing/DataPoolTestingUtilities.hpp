////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "DataPool.hpp"

namespace sequoia::unit_testing
{
  template<class T, class Allocator>
  struct weak_equivalence_checker<data_sharing::data_pool<T, Allocator>>
  {
    using type = data_sharing::data_pool<T, Allocator>;
    using prediction_type = std::initializer_list<std::pair<T, long>>;
    
    template<class Logger>
    static void check(std::string_view description, Logger& logger, const type& pool, prediction_type prediction)
    {
      check_equality(combine_messages(description, "empty"), logger, pool.empty(), prediction.size() == 0);
      check_equality(combine_messages(description, "size"), logger, pool.size(), prediction.size());
      
      check_range_equivalence(combine_messages(description, "iterator"), logger, pool.begin(), pool.end(), prediction.begin(), prediction.end());
      check_range_equivalence(combine_messages(description, "citerator"), logger, pool.cbegin(), pool.cend(), prediction.begin(), prediction.end());
      check_range_equivalence(combine_messages(description, "riterator"), logger, pool.rbegin(), pool.rend(), std::rbegin(prediction), std::rend(prediction));
      check_range_equivalence(combine_messages(description, "criterator"), logger, pool.crbegin(), pool.crend(), std::rbegin(prediction), std::rend(prediction));
    }
  };
}
