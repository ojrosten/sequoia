////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "StaticPriorityQueueTest.hpp"
#include "StaticPriorityQueueTestingUtilities.hpp"

#include "sequoia/PlatformSpecific/Macros.hpp"

import std;
import sequoia.core.data_structures;
import sequoia.core.meta;

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path test_static_priority_queue::source_file()
  {
    return std::source_location::current().file_name();
  }

  void test_static_priority_queue::run_tests()
  {
    check_moves_do_not_copy();
    check_top_and_pop_in_constant_evaluation();
    check_pop_order_in_constant_evaluation();
    check_pop_comparisons_in_constant_evaluation();
    check_depth_0();
    check_depth_1();
    check_depth_2();
    check_depth_3();
    check_construction_from_list_shorter_than_depth();
    check_layout_after_pop();
  }

  void test_static_priority_queue::check_moves_do_not_copy()
  {
    using queue_t = data_structures::static_priority_queue<std::unique_ptr<int>, 1>;

    STATIC_CHECK(std::is_move_constructible_v<queue_t>);
    STATIC_CHECK(std::is_move_assignable_v<queue_t>);
  }

  /** The queue value-initialises the slots beyond the list, so they hold zero.
      Every element of the list is negative, so a slot heapified with the list
      would be the first top. The greatest element of the list is last, so
      heapifying too few of the list's elements would give the wrong first top
      too.
   */
  constexpr auto test_static_priority_queue::successive_tops_from_list_shorter_than_depth()
  {
    using namespace data_structures;

    static_priority_queue<int, 4> q{-3, -2, -1};

    const int first{q.top()};
    q.pop();
    const int second{q.top()};
    q.pop();

    return std::array{first, second, q.top()};
  }

  void test_static_priority_queue::check_top_and_pop_in_constant_evaluation()
  {
    constexpr auto tops{successive_tops_from_list_shorter_than_depth()};

    STATIC_CHECK(tops == std::array{-1, -2, -3});

    using queue_t = data_structures::static_priority_queue<int, 1>;

    STATIC_CHECK( invocation_is_constant_expression_v<[]() { return queue_t{1}.top(); }>);
    STATIC_CHECK(!invocation_is_constant_expression_v<[]() { return queue_t{}.top();  }>);
    STATIC_CHECK( invocation_is_constant_expression_v<[]() { queue_t{1}.pop(); }>);
    STATIC_CHECK(!invocation_is_constant_expression_v<[]() { queue_t{}.pop();  }>);
  }

  template<std::size_t N, class Compare>
  constexpr auto test_static_priority_queue::successive_tops_from(const std::array<int, N>& elements,
                                                                const Compare&             compare)
  {
    data_structures::static_priority_queue<int, N, Compare> q{compare};
    for(const int e : elements)
    {
      q.push(e);
    }

    std::array<int, N> tops{};
    for(int& t : tops)
    {
      t = q.top();
      q.pop();
    }

    return tops;
  }

  void test_static_priority_queue::check_pop_order_in_constant_evaluation()
  {
    // Popping ten elements passes through even sizes, where the last parent has a left child only
    static constexpr std::array<int, 10> ten{5, 3, 8, 1, 9, 2, 7, 4, 6, 0};
    STATIC_CHECK(successive_tops_from(ten, std::ranges::less{})    == std::array{9, 8, 7, 6, 5, 4, 3, 2, 1, 0});
    STATIC_CHECK(successive_tops_from(ten, std::ranges::greater{}) == std::array{0, 1, 2, 3, 4, 5, 6, 7, 8, 9});

    static constexpr std::array<int, 7> ascending{0, 1, 2, 3, 4, 5, 6};
    STATIC_CHECK(successive_tops_from(ascending, std::ranges::less{}) == std::array{6, 5, 4, 3, 2, 1, 0});

    static constexpr std::array<int, 7> duplicates{2, 2, 1, 2, 1, 1, 2};
    STATIC_CHECK(successive_tops_from(duplicates, std::ranges::less{}) == std::array{2, 2, 2, 2, 1, 1, 1});

    static constexpr std::array<int, 1> single{4};
    STATIC_CHECK(successive_tops_from(single, std::ranges::less{}) == std::array{4});
  }

  /** Compares by value, counting each comparison through a pointer: the
      queue copies its comparator, so the copies share the count.
   */
  struct test_static_priority_queue::counting_less
  {
    int* count{};

    [[nodiscard]]
    constexpr bool operator()(const int lhs, const int rhs) const
    {
      ++*count;
      return lhs < rhs;
    }
  };

  constexpr int test_static_priority_queue::comparisons_in_one_pop()
  {
    int count{};
    data_structures::static_priority_queue<int, 15, counting_less> q{counting_less{&count}};
    for(int i{}; i < 15; ++i)
    {
      q.push((i * 7) % 15);
    }

    count = 0;
    q.pop();

    return count;
  }

  /** A pop sifts down from the root, comparing at most twice at each of the
      three levels below it. Rebuilding the heap compares at least once for
      each of the other thirteen elements.
   */
  void test_static_priority_queue::check_pop_comparisons_in_constant_evaluation()
  {
    constexpr int comparisons{comparisons_in_one_pop()};

    STATIC_CHECK(comparisons <= 6);
  }

  /** The layout after a pop is the implementation's choice: sifting down from
      the root can leave a layout different from the one built from the same
      elements, and so a queue compares unequal to the queue built from them.
      The same holds for which of two equal children is swapped.
   */
  void test_static_priority_queue::check_layout_after_pop()
  {
    using queue_t = data_structures::static_priority_queue<int, 4>;

    queue_t q{};
    q.push(0);
    q.push(2);
    q.push(3);
    q.push(1);
    q.pop();

    check(equality, "Sifting down leaves the larger child in front", q, queue_t{2, 1, 0});
    check("Equality compares the layout, not the elements", q != queue_t{0, 1, 2});

    queue_t ties{};
    ties.push(1);
    ties.push(1);
    ties.push(1);
    ties.push(0);
    ties.pop();

    check(equality, "Sifting down swaps with the right child when the children are equal", ties, queue_t{1, 1, 0});
  }

  void test_static_priority_queue::check_depth_0()
  {
    using namespace data_structures;

    constexpr static_priority_queue<int, 0> s{};
    static_priority_queue<int, 0> t{};

    check(equality, "Equality of null queues", t, s);

    check_exception_thrown<std::logic_error>("Can't push to null queue"           , [&t]() { t.push(1); });
    check_exception_thrown<std::logic_error>("Can't construct non-null null queue", []() { static_priority_queue<int, 0>{1}; });
    check_exception_thrown<std::logic_error>("Can't access top of null queue"     , [&t]() { return t.top(); });
    check_exception_thrown<std::logic_error>("Can't pop from null queue"          , [&t]() { t.pop(); });
  }

  void test_static_priority_queue::check_depth_1()
  {
    using namespace data_structures;
    using queue_t = static_priority_queue<int, 1>;

    constexpr queue_t s{1};
    queue_t t{};
    t.push(2);

    check_semantics("Standard Semantics", s, t);

    check_exception_thrown<std::logic_error>("Trying to push two elements to queue of depth 1", [&t]() { t.push(1); });
    check_exception_thrown<std::logic_error>("Can't construct queue of depth 1 with 2 elements", []() { queue_t{1, 2}; });

    t.pop();
    check(equality, "", t, queue_t{});

    check_exception_thrown<std::logic_error>("Can't access top of empty queue", [&t]() { return t.top(); });
    check_exception_thrown<std::logic_error>("Can't pop from empty queue"     , [&t]() { t.pop(); });
    check(equality, "A refused pop leaves the queue empty", t, queue_t{});

    t.push(1);
    check(equality, "", t, queue_t{1});
  }

  void test_static_priority_queue::check_depth_2()
  {
    using namespace data_structures;

    {
      using queue_t = static_priority_queue<int, 2>;
      constexpr queue_t s{1, 2};
      queue_t t{4, 3};

      check(equality, "", s.top(), 2);
      check(equality, "", t.top(), 4);

      check_semantics("Standard Semantics", s, t);

      check_exception_thrown<std::logic_error>("Trying to push three elements to queue of depth 2", [&t]() { t.push(1); });
      check_exception_thrown<std::logic_error>("Can't construct queue of depth 2 with 3 elements", []() { queue_t{1, 2, 3}; });

      t.pop();
      check(equality, "", t, queue_t{3});

      t.push(5);
      check(equality, "", t, queue_t{5, 3});

    }

    {
      struct comp
      {
        constexpr comp(const int mod) : m_mod{mod} {}

        constexpr bool operator()(const int a, const int b) noexcept
        {
          return a%m_mod < b%m_mod;
        }
      private:
        int m_mod{};
      };

      constexpr static_priority_queue<int, 2, comp> s{{3, 2}, comp{3}}, t{{4, 6}, comp{2}};
      check(equality, "", s.top(), 2);

      check_semantics("Standard semantics", s, t);
    }
  }

  constexpr auto test_static_priority_queue::make_static_priority_queue_3()
  {
    using namespace data_structures;

    static_priority_queue<int, 3> s{};

    s.push(0);
    s.pop();
    s.push(6);
    s.push(2);
    s.push(8);

    return s;
  }

  void test_static_priority_queue::check_depth_3()
  {
    using namespace data_structures;
    using queue_t = static_priority_queue<int, 3>;

    constexpr queue_t s{make_static_priority_queue_3()};
    check(equality, "", s, queue_t{8, 2, 6});

    auto t{s};
    check(equality, "", t, queue_t{8, 2, 6});

    t.pop();
    check(equality, "", t, queue_t{6, 2});

    check_semantics("Standard semantics", s, t);

    t.pop();
    check(equality, "", t, queue_t{2});

    t.pop();
    check(equality, "", t, queue_t{});
  }

  void test_static_priority_queue::check_construction_from_list_shorter_than_depth()
  {
    check(
      equality,
      "Each top of a queue built from a list shorter than the queue's depth is the greatest element remaining",
      successive_tops_from_list_shorter_than_depth(),
      std::array{-1, -2, -3}
    );
  }
}
