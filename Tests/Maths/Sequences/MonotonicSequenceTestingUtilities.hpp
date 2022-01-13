////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/TestFramework/RegularTestCore.hpp"

#include "sequoia/Maths/Sequences/MonotonicSequence.hpp"

namespace sequoia::testing
{
  namespace impl
  {
    template<test_mode Mode, class T>
    void check(test_logger<Mode>& logger, const T& sequence, const T& prediction)
    {
      check(equality, "Emptiness incorrect", logger, sequence.empty(), prediction.empty());

      if(check(equality, "Size incorrect", logger, sequence.size(), prediction.size()))
      {
        if(!prediction.empty())
        {
          check(equality, "Back element wrong", logger, sequence.back(), prediction.back());
          check(equality, "Front element wrong", logger, sequence.front(), prediction.front());
        }

        auto i_prediction{prediction.begin()}, i{sequence.begin()};
        auto ci_prediction{prediction.cbegin()}, ci{sequence.cbegin()};
        auto ri_prediction{prediction.rbegin()}, ri{sequence.rbegin()};
        auto cri_prediction{prediction.crbegin()}, cri{sequence.crbegin()};

        for(;i_prediction != prediction.end(); ++i_prediction, ++i, ++ci_prediction, ++ci, ++ri_prediction, ++ri, ++cri_prediction, ++cri)
        {
          using std::distance;
          const auto d{distance(prediction.begin(), i_prediction)};
          const auto mess{std::string{" for index "}.append(std::to_string(d))};

          check(equality, std::string{"Dereferenced iterator wrong"}.append(mess), logger, *i, *i_prediction);
          check(equality, std::string{"Dereferenced citerator wrong"}.append(mess), logger, *ci, *ci_prediction);

          check(equality, std::string{"operator[] wrong"}.append(mess), logger, sequence[d], prediction[d]);

          const auto shift{static_cast<int64_t>(prediction.size()) - d - 1};
          check(equality, std::string{"Dereferenced riterator wrong"}.append(mess), logger, *(ri + shift), *(ri_prediction + shift));
          check(equality, std::string{"Dereferenced criterator wrong"}.append(mess), logger, *(cri + shift), *(cri_prediction + shift));
        }

        testing::check("iterator location wrong", logger, i_prediction == prediction.end());
        testing::check("citerator location wrong", logger, ci_prediction == prediction.cend());
        testing::check("riterator location wrong", logger, ri_prediction == prediction.rend());
        testing::check("criterator location wrong", logger, cri_prediction == prediction.crend());
      }
    }
  }

  template<class T, class C, class Compare>
  struct value_tester<maths::monotonic_sequence<T, C, Compare>>
  {
    using type = maths::monotonic_sequence<T, C, Compare>;

    template<test_mode Mode>
    static void test_equality(test_logger<Mode>& logger, const type& sequence, const type& prediction)
    {
      impl::check(logger, sequence, prediction);
    }

    template<test_mode Mode>
    static void test_equivalence(test_logger<Mode>& logger, const type& sequence, std::initializer_list<T> prediction)
    {
      check_range("", logger, sequence.begin(), sequence.end(), prediction.begin(), prediction.end());
    }
  };

  template<class T, std::size_t N, class Compare>
  struct value_tester<maths::static_monotonic_sequence<T, N, Compare>>
  {
    using type = maths::static_monotonic_sequence<T, N, Compare>;

    template<test_mode Mode>
    static void test_equality(test_logger<Mode>& logger, const type& sequence, const type& prediction)
    {
      impl::check(logger, sequence, prediction);
    }

    template<test_mode Mode>
    static void test_equivalence(test_logger<Mode>& logger, const type& sequence, std::initializer_list<T> prediction)
    {
      check_range("", logger, sequence.begin(), sequence.end(), prediction.begin(), prediction.end());
    }
  };
}
