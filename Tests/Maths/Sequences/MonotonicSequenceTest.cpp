////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "MonotonicSequenceTest.hpp"
#include "MonotonicSequenceTestingUtilities.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view monotonic_sequence_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void monotonic_sequence_test::run_tests()
  {
    test_decreasing_sequence();
    test_static_decreasing_sequence();
    test_static_increasing_sequence();
  }

  void monotonic_sequence_test::test_decreasing_sequence()
  {
    using namespace maths;

    check_exception_thrown<std::logic_error>(LINE("Invariant violated by initialization"), [](){ monotonic_sequence<int> s{1,2}; });

    monotonic_sequence<int> s{}, t{1};
    // - ; 1

    check(equivalence, LINE(""), s, std::initializer_list<int>{});
    check(equivalence, LINE(""), t, std::initializer_list<int>{1});
    check_semantics(LINE(""), s, t);

    check_exception_thrown<std::logic_error>(LINE(""), [&t](){ t.push_back(2); });
    check(equivalence, LINE("Invariant violated by attempted push_back"), t, std::initializer_list<int>{1});

    check_exception_thrown<std::logic_error>(LINE("Invariant violated by attempted insertion at beginning"), [&t](){ t.insert(t.cbegin(), 0); });
    check(equivalence, LINE(""), t, std::initializer_list<int>{1});

    t.push_back(1);
    // - ; 1,1

    check(equivalence, LINE(""), t, std::initializer_list<int>{1, 1});
    check(equality, LINE(""), t, monotonic_sequence<int>{1,1});

    check_exception_thrown<std::logic_error>(LINE("Invariant violated by attempted insertion in middle"), [&t](){ t.insert(t.cbegin() + 1, 2); });

    t.mutate(t.cbegin(), t.cend() - 1, [](int a){ return a *= 2; });
    // - ; 2,1

    check(equivalence, LINE(""), t, std::initializer_list<int>{2, 1});
    check(equality, LINE(""), t, monotonic_sequence<int>{2,1});

    s.mutate(s.begin(), s.end(), [](int a) { return a+1; });
    check(equivalence, LINE(""), s, std::initializer_list<int>{});

    s.push_back(4);
    // 4 ; 2,1

    check(equivalence, LINE(""), s, std::initializer_list<int>{4});
    check_semantics(LINE(""), s, t);

    t.erase(t.begin(), t.end());
    // 4 ; -

    check(equivalence, LINE(""), t, std::initializer_list<int>{});

    s.erase(s.begin());
    // - ; -

    check(equivalence, LINE(""), s, std::initializer_list<int>{});

    check(equality, LINE("Capacity"), t.capacity(), 2_sz);
    t.shrink_to_fit();
    check(equality, LINE("Capacity"), t.capacity(), 0_sz);
    t.reserve(2);
    check(equality, LINE("Capacity"), t.capacity(), 2_sz);

    t = monotonic_sequence<int>{8, 4, 3};
    // - ; 8,4,3
    check(equivalence, LINE(""), t, std::initializer_list<int>{8, 4, 3});

    check_exception_thrown<std::logic_error>(LINE(""),
      [&t](){ t.mutate(t.begin(), t.begin()+2, [](const int i) { return i/2; }); });

    check(equivalence, LINE(""), t, std::initializer_list<int>{8, 4, 3});

    t.mutate(t.begin(), t.end(), [](const int i) { return i/2; });
    // -; 4, 2, 1

    check(equivalence, LINE(""), t, std::initializer_list<int>{4, 2, 1});
  }

  void monotonic_sequence_test::test_static_decreasing_sequence()
  {
    using namespace maths;

    check_exception_thrown<std::logic_error>(LINE("Invariant violated by initialization"), [](){ static_monotonic_sequence<double, 2> s{1,2}; });

    constexpr static_monotonic_sequence<double, 2> s{5.1, 3.8}, t{-3.4, -4.4};
    check_semantics(LINE(""), s, t);
  }

  template<bool Check>
  constexpr maths::static_monotonic_sequence<int,6, std::greater<int>>
  monotonic_sequence_test::make_sequence()
  {
    using namespace maths;

    static_monotonic_sequence<int, 6, std::greater<int>> s{-1,0,1,1,2,6};

    if constexpr(Check)
    {
      s.mutate(s.begin(), s.end(), [](const int i) { return i * 2; });
    }
    else
    {
      s.mutate(unsafe_t{}, s.begin(), s.end(), [](const int i) { return i * 2; });
    }

    return s;
  }

  void monotonic_sequence_test::test_static_increasing_sequence()
  {
    using namespace maths;

    constexpr auto s{make_sequence<false>()};
    check(equivalence, LINE(""), s, std::initializer_list<int>{-2,0,2,2,4,12});

    constexpr auto s2{make_sequence<true>()};
    check(equivalence, LINE(""), s2, std::initializer_list<int>{-2,0,2,2,4,12});

    check(equality, LINE(""), s, static_monotonic_sequence<int, 6, std::greater<int>>{-2,0,2,2,4,12});

    static_monotonic_sequence<int, 6, std::greater<int>> t{2,2,2,3,3,3};
    check_exception_thrown<std::logic_error>(LINE(""), [&t](){
        t.mutate(t.begin()+1, t.begin()+3,[](const int i){ return i*2;});});

    check(equivalence, LINE(""), t, std::initializer_list<int>{2,2,2,3,3,3});
    check_semantics(LINE(""), s, t);

    static_monotonic_sequence<int, 6, std::greater<int>> u{2,3,3,4,4,5};
    check_exception_thrown<std::logic_error>(LINE(""), [&u](){
        u.mutate(u.begin()+1, u.begin()+4,[](const int i){ return i*2;});});

    check(equivalence, LINE(""), u, std::initializer_list<int>{2,3,3,4,4,5});
  }
}
