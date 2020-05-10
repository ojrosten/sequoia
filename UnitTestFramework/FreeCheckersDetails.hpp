////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Implementation details for the testing of free functions.
*/

#include "UnitTestLogger.hpp"

namespace sequoia::unit_testing
{
  template<class T, class... U>
  [[nodiscard]]
  std::string add_type_info(std::string_view description);

  template<class T, class... U>
  struct type_demangler;
}

namespace sequoia::unit_testing::impl
{
  template<class EquivChecker, test_mode Mode, class T, class S, class... U>
  bool check(std::string_view description, unit_test_logger<Mode>& logger, const T& value, const S& s, const U&... u)
  {
    using sentinel = typename unit_test_logger<Mode>::sentinel;

    const std::string message{
      add_type_info<S, U...>(
                             combine_messages(description, "Comparison performed using:\n\t" + type_demangler<EquivChecker>::make() + "\n\tWith equivalent types:", "\n"))
    };
      
    sentinel r{logger, message};
    const auto previousFailures{logger.failures()};
    
    EquivChecker::check(message, logger, value, s, u...);
      
    return logger.failures() == previousFailures;
  }

  template<test_mode Mode, class Tag, class Iter, class PredictionIter>
  bool check_range(std::string_view description, unit_test_logger<Mode>& logger, Tag tag, Iter first, Iter last, PredictionIter predictionFirst, PredictionIter predictionLast)
  {
    typename unit_test_logger<Mode>::sentinel r{logger, description};
    bool equal{true};

    using std::distance;
    using std::advance;

    const auto predictedSize{distance(predictionFirst, predictionLast)};
    if(check_equality(combine_messages(description, "Container size wrong", "\n"), logger, distance(first, last), predictedSize))
    {
      auto predictionIter{predictionFirst};
      auto iter{first};
      for(; predictionIter != predictionLast; advance(predictionIter, 1), advance(iter, 1))
      {
        std::string dist{std::to_string(distance(predictionFirst, predictionIter)).append("\n")};
        if(!dispatch_check(combine_messages(description, "element ", "\n").append(std::move(dist)), logger, tag, *iter, *predictionIter)) equal = false;
      }
    }
    else
    {
      equal = false;
    }
      
    return equal;
  }
}
