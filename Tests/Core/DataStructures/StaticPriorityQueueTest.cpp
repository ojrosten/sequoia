////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "StaticPriorityQueueTest.hpp"
#include "StaticPriorityQueueTestingUtilities.hpp"

#include "sequoia/Core/DataStructures/StaticPriorityQueue.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path test_static_priority_queue::source_file()
  {
    return std::source_location::current().file_name();
  }

  void test_static_priority_queue::run_tests()
  {
    check_pops_in_constant_evaluation();
    check_depth_0();
    check_depth_1();
    check_depth_2();
    check_depth_3();
    check_construction_from_list_shorter_than_depth();
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

  void test_static_priority_queue::check_pops_in_constant_evaluation()
  {
    constexpr auto tops{successive_tops_from_list_shorter_than_depth()};

    STATIC_CHECK(tops == std::array{-1, -2, -3});

    constexpr bool isEmptyAfterPoppingEmpty{
      []() {
        data_structures::static_priority_queue<int, 1> q{};
        q.pop();
        return q.empty();
      }()
    };

    STATIC_CHECK(isEmptyAfterPoppingEmpty);
  }

  void test_static_priority_queue::check_depth_0()
  {
    using namespace data_structures;

    constexpr static_priority_queue<int, 0> s{};
    static_priority_queue<int, 0> t{};

    check(equality, "Equality of null queues", t, s);

    check_exception_thrown<std::logic_error>("Can't push to null queue", [&t]() { t.push(1); });
    check_exception_thrown<std::logic_error>("Can't construct non-null null queue", []() { static_priority_queue<int, 0>{1}; });

    t.pop();
    check(equality, "Popping a null queue leaves it empty", t, s);
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

    t.pop();
    check(equality, "Popping an empty queue leaves it empty", t, queue_t{});

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
