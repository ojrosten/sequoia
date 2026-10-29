////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "MonotonicSequenceTest.hpp"
#include "MonotonicSequenceTestingUtilities.hpp"

#include <cstdint>
#include <limits>

namespace sequoia::testing
{
  namespace
  {
    template<class Sequence>
    concept translatable = requires(Sequence& s) { s.translate(s.cbegin(), s.cend(), 1); };

    /// A comparison that no non-zero translation preserves. It flags each pair
    /// whose second element is 3.
    struct three_never_follows
    {
      [[nodiscard]]
      constexpr bool operator()(const int, const int y) const noexcept { return y == 3; }
    };
  }

  [[nodiscard]]
  std::filesystem::path monotonic_sequence_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void monotonic_sequence_test::run_tests()
  {
    test_translation_constraints();
    test_decreasing_sequence();
    test_static_decreasing_sequence();
    test_static_increasing_sequence();
    test_translation_limits();
    test_overwrite();
  }

  void monotonic_sequence_test::test_translation_constraints()
  {
    using namespace maths;

    STATIC_CHECK(translatable<monotonic_sequence<int>>);
    STATIC_CHECK(translatable<static_monotonic_sequence<std::size_t, 2, std::ranges::greater>>);

    STATIC_CHECK(!translatable<static_monotonic_sequence<double, 2>>);
    STATIC_CHECK(!translatable<static_monotonic_sequence<int, 2, std::ranges::less_equal>>);
    STATIC_CHECK(!translatable<static_monotonic_sequence<int, 2, std::greater<int>>>);
    STATIC_CHECK(!translatable<static_monotonic_sequence<int, 2, three_never_follows>>);
  }

  void monotonic_sequence_test::test_decreasing_sequence()
  {
    using namespace maths;

    check_exception_thrown<std::invalid_argument>("Invariant violated by initialization",
                                                  [](){ monotonic_sequence<int> s{1,2}; });

    monotonic_sequence<int> s{}, t{1};
    // - ; 1

    check(equivalence, "", s, std::initializer_list<int>{});
    check(equivalence, "", t, std::initializer_list<int>{1});
    check_semantics("", s, t);

    check_exception_thrown<std::invalid_argument>("", [&t](){ t.push_back(2); });
    check(equivalence, "Invariant violated by attempted push_back", t, std::initializer_list<int>{1});

    check_exception_thrown<std::invalid_argument>("Invariant violated by attempted insertion at beginning",
                                                  [&t](){ t.insert(t.cbegin(), 0); });
    check(equivalence, "", t, std::initializer_list<int>{1});

    t.push_back(1);
    // - ; 1,1

    check(equivalence, "", t, std::initializer_list<int>{1, 1});
    check(equality, "", t, monotonic_sequence<int>{1,1});

    check_exception_thrown<std::invalid_argument>("Invariant violated by attempted insertion in middle",
                                                  [&t](){ t.insert(t.cbegin() + 1, 2); });

    t.translate(t.cbegin(), t.cend() - 1, 1);
    // - ; 2,1

    check(equivalence, "", t, std::initializer_list<int>{2, 1});
    check(equality, "", t, monotonic_sequence<int>{2,1});

    s.translate(s.begin(), s.end(), 1);
    check(equivalence, "Translating an empty sequence leaves it empty", s, std::initializer_list<int>{});

    s.push_back(4);
    // 4 ; 2,1

    check(equivalence, "", s, std::initializer_list<int>{4});
    check_semantics("", s, t);

    t.erase(t.begin(), t.end());
    // 4 ; -

    check(equivalence, "", t, std::initializer_list<int>{});

    s.erase(s.begin());
    // - ; -

    check(equivalence, "", s, std::initializer_list<int>{});

    check(equality, "Capacity", t.capacity(), 2uz);
    t.shrink_to_fit();
    check(equality, "Capacity", t.capacity(), 0uz);
    t.reserve(2);
    check(equality, "Capacity", t.capacity(), 2uz);

    t = monotonic_sequence<int>{8, 4, 3};
    // - ; 8,4,3
    check(equivalence, "", t, std::initializer_list<int>{8, 4, 3});

    check_exception_thrown<std::invalid_argument>("Translating 8, 4 by -2 throws, since 2 is less than the 3 after it",
                                                  [&t](){ t.translate(t.begin(), t.begin()+2, -2); });

    check(equivalence, "A refused translation leaves the sequence unchanged", t, std::initializer_list<int>{8, 4, 3});

    t.translate(t.begin(), t.end(), -4);
    // -; 4, 0, -1

    check(equivalence, "Translating every element by -4", t, std::initializer_list<int>{4, 0, -1});

    t.translate(t.begin() + 1, t.end(), 4);
    // -; 4, 4, 3

    check(equivalence,
          "Translating 0, -1 by 4 brings 4 level with the 4 before it",
          t,
          std::initializer_list<int>{4, 4, 3});
  }

  void monotonic_sequence_test::test_static_decreasing_sequence()
  {
    using namespace maths;

    check_exception_thrown<std::invalid_argument>("Invariant violated by initialization",
                                                  [](){ static_monotonic_sequence<double, 2> s{1,2}; });

    constexpr static_monotonic_sequence<double, 2> s{5.1, 3.8}, t{-3.4, -4.4};
    check_semantics("", s, t);

    // On the trait, since static_monotonic_sequence has no public swap
    STATIC_CHECK(maths::impl::swap_is_noexcept_v<std::array<double, 2>>);
  }

  constexpr maths::static_monotonic_sequence<int, 6, std::ranges::greater>
  monotonic_sequence_test::make_sequence()
  {
    using namespace maths;

    static_monotonic_sequence<int, 6, std::ranges::greater> s{-1,0,1,1,2,6};
    s.translate(s.begin(), s.end(), 1);

    return s;
  }

  void monotonic_sequence_test::test_static_increasing_sequence()
  {
    using namespace maths;

    constexpr auto s{make_sequence()};
    check(equivalence,
          "Translating a static sequence by 1 in a constant expression",
          s,
          std::initializer_list<int>{0,1,2,2,3,7});
    check(equality, "", s, static_monotonic_sequence<int, 6, std::ranges::greater>{0,1,2,2,3,7});

    static_monotonic_sequence<int, 6, std::ranges::greater> t{2,2,2,3,3,3};
    check_exception_thrown<std::invalid_argument>("Translating 2, 2 by 2 throws, since 4 exceeds the 3 after it",
                                                  [&t](){ t.translate(t.begin()+1, t.begin()+3, 2); });

    check(equivalence,
          "A refused translation leaves the sequence unchanged",
          t,
          std::initializer_list<int>{2,2,2,3,3,3});
    check_semantics("", s, t);

    static_monotonic_sequence<int, 6, std::ranges::greater> u{2,3,3,4,4,5};
    check_exception_thrown<std::invalid_argument>("Translating 3, 3, 4 by -2 throws, since 1 is below the 2 before it",
                                                  [&u](){ u.translate(u.begin()+1, u.begin()+4, -2); });

    check(equivalence,
          "A refused translation leaves the sequence unchanged",
          u,
          std::initializer_list<int>{2,3,3,4,4,5});
  }

  void monotonic_sequence_test::test_translation_limits()
  {
    using namespace maths;

    auto checkTranslation{
      [this]<integer T>(std::string_view description, const T x, const std::ptrdiff_t delta, const T prediction) {
        static_monotonic_sequence<T, 1> s{x};
        s.translate(s.begin(), s.end(), delta);
        check(equivalence, description, s, std::initializer_list<T>{prediction});
      }
    };

    auto checkRefusal{
      [this]<integer T>(std::string_view description, const T x, const std::ptrdiff_t delta) {
        static_monotonic_sequence<T, 1> s{x};
        check_exception_thrown<std::invalid_argument>(description,
                                                      [&s, delta](){ s.translate(s.begin(), s.end(), delta); });
        check(equivalence,
              std::string{description}.append(", and the sequence is unchanged"),
              s,
              std::initializer_list<T>{x});
      }
    };

    checkTranslation("254 + 1 is the maximum of std::uint8_t", std::uint8_t{254}, 1, std::uint8_t{255});
    checkRefusal("255 + 1 exceeds the maximum of std::uint8_t", std::uint8_t{255}, 1);
    checkTranslation("255 - 255 is the minimum of std::uint8_t", std::uint8_t{255}, -255, std::uint8_t{});
    checkRefusal("0 - 1 is below the minimum of std::uint8_t", std::uint8_t{}, -1);
    checkRefusal("0 + 256 exceeds the maximum of std::uint8_t, though 256 wraps to 0", std::uint8_t{}, 256);
    checkRefusal("255 - 256 is below the minimum of std::uint8_t, though -256 wraps to 0", std::uint8_t{255}, -256);

    checkTranslation("-128 + 255 is the maximum of std::int8_t", std::int8_t{-128}, 255, std::int8_t{127});
    checkTranslation("127 - 255 is the minimum of std::int8_t", std::int8_t{127}, -255, std::int8_t{-128});
    checkRefusal("127 + 1 exceeds the maximum of std::int8_t", std::int8_t{127}, 1);
    checkRefusal("-128 - 1 is below the minimum of std::int8_t", std::int8_t{-128}, -1);

    constexpr auto deltaMax{std::numeric_limits<std::ptrdiff_t>::max()},
                   deltaMin{std::numeric_limits<std::ptrdiff_t>::lowest()};
    constexpr auto int64Max{std::numeric_limits<std::int64_t>::max()},
                   int64Min{std::numeric_limits<std::int64_t>::lowest()};
    constexpr auto uint64Max{std::numeric_limits<std::uint64_t>::max()},
                   twoToThe63{static_cast<std::uint64_t>(deltaMax) + 1};

    checkTranslation("The maximum of std::int64_t plus the minimum delta is -1", int64Max, deltaMin, std::int64_t{-1});
    checkTranslation("0 plus the minimum delta is the minimum of std::int64_t", std::int64_t{}, deltaMin, int64Min);
    checkRefusal("-1 plus the minimum delta is below the minimum of std::int64_t", std::int64_t{-1}, deltaMin);

    checkTranslation("2^63 plus the minimum delta is 0", twoToThe63, deltaMin, std::uint64_t{});
    checkRefusal("2^63 - 1 plus the minimum delta is below 0", twoToThe63 - 1, deltaMin);
    checkTranslation("2^64 - 1 - (2^63 - 1) plus the maximum delta is the maximum of std::uint64_t",
                     uint64Max - static_cast<std::uint64_t>(deltaMax), deltaMax, uint64Max);
    checkRefusal("2^63 + 1 plus the maximum delta exceeds the maximum of std::uint64_t", twoToThe63 + 1, deltaMax);

    static_monotonic_sequence<std::uint8_t, 2, std::ranges::greater> s{254, 255};
    check_exception_thrown<std::invalid_argument>("Translating 254, 255 by 2 throws, though wrapped 0, 1 is monotonic",
                                                  [&s](){ s.translate(s.begin(), s.end(), 2); });
    check(equivalence,
          "A refused translation leaves the sequence unchanged",
          s,
          std::initializer_list<std::uint8_t>{254, 255});

    static_monotonic_sequence<std::uint8_t, 2, std::ranges::greater> t{1, 255};
    check_exception_thrown<std::invalid_argument>("Translating 1, 255 by 1 throws, since 256 exceeds the maximum",
                                                  [&t](){ t.translate(t.begin(), t.end(), 1); });
    check(equivalence,
          "A refused translation leaves the sequence unchanged",
          t,
          std::initializer_list<std::uint8_t>{1, 255});

    static_monotonic_sequence<std::uint8_t, 2> u{255, 1};
    check_exception_thrown<std::invalid_argument>("Translating 255, 1 by 1 throws, since 256 exceeds the maximum",
                                                  [&u](){ u.translate(u.begin(), u.end(), 1); });
    check(equivalence,
          "A refused translation leaves the sequence unchanged",
          u,
          std::initializer_list<std::uint8_t>{255, 1});
  }

  void monotonic_sequence_test::test_overwrite()
  {
    using namespace maths;

    monotonic_sequence<int> s{8, 4, 3};
    s.overwrite(std::array{9, 9, 1});
    check(equivalence,
          "Overwriting with as many monotonic values replaces every element",
          s,
          std::initializer_list<int>{9, 9, 1});

    check_exception_thrown<std::invalid_argument>("Overwriting with fewer values throws",
                                                  [&s](){ s.overwrite(std::array{7, 7}); });
    check(equivalence,
          "Overwriting with fewer values leaves the sequence unchanged",
          s,
          std::initializer_list<int>{9, 9, 1});

    check_exception_thrown<std::invalid_argument>("Overwriting with more values throws",
                                                  [&s](){ s.overwrite(std::array{9, 9, 1, 0}); });
    check(equivalence,
          "Overwriting with more values leaves the sequence unchanged",
          s,
          std::initializer_list<int>{9, 9, 1});

    check_exception_thrown<std::invalid_argument>("Overwriting with values that are not monotonic throws",
                                                  [&s](){ s.overwrite(std::array{1, 9, 9}); });
    check(equivalence,
          "Overwriting with values that are not monotonic leaves the sequence unchanged",
          s,
          std::initializer_list<int>{9, 9, 1});

    constexpr auto t{
      [](){
        static_monotonic_sequence<int, 3, std::greater<int>> u{1, 2, 3};
        u.overwrite(std::array{0, 4, 4});
        return u;
      }()
    };

    check(equivalence,
          "Overwriting a static sequence in a constant expression replaces every element",
          t,
          std::initializer_list<int>{0, 4, 4});
  }
}
