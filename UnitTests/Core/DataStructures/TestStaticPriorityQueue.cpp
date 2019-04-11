////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestStaticPriorityQueue.hpp"
#include "StaticPriorityQueueTestingUtilities.hpp"

#include "StaticPriorityQueue.hpp"

namespace sequoia::unit_testing
{
  void test_static_priority_queue::run_tests()
  {
    check_depth_0();
    check_depth_1();
    check_depth_2();
    check_depth_3();
  }

  void test_static_priority_queue::check_depth_0()
  {
    using namespace data_structures;

    constexpr static_priority_queue<int, 0> s{};
    static_priority_queue<int, 0> t{};

    check_equality(t, s, LINE("Equality of null queues"));

    check_exception_thrown<std::logic_error>([&t]() { t.push(1); }, LINE("Can't push to null queue"));
    check_exception_thrown<std::logic_error>([]() { static_priority_queue<int, 0>{1}; }, LINE("Can't construct non-null null queue"));

  }
  
  void test_static_priority_queue::check_depth_1()
  {
    using namespace data_structures;
    using queue_t = static_priority_queue<int, 1>;

    constexpr queue_t s{1};
    queue_t t{};
    t.push(2);

    check_regular_semantics(s, t, LINE("Standard Semantics"));

    check_exception_thrown<std::logic_error>([&t]() { t.push(1); }, LINE("Trying to push two elements to queue of depth 1"));
    check_exception_thrown<std::logic_error>([]() { queue_t{1, 2}; }, LINE("Can't construct queue of depth 1 with 2 elements"));

    t.pop();
    check_equality(t, queue_t{}, LINE(""));
    
    t.push(1);
    check_equality(t, queue_t{1}, LINE(""));
  }

  void test_static_priority_queue::check_depth_2()
  {
    using namespace data_structures;
    
    {
      using queue_t = static_priority_queue<int, 2>;
      constexpr queue_t s{1, 2};
      queue_t t{4, 3};

      check_equality(s.top(), 2, LINE(""));
      check_equality(t.top(), 4, LINE(""));
      
      check_regular_semantics(s, t, LINE("Standard Semantics"));

      check_exception_thrown<std::logic_error>([&t]() { t.push(1); }, LINE("Trying to push three elements to queue of depth 2"));
      check_exception_thrown<std::logic_error>([]() { queue_t{1, 2, 3}; }, LINE("Can't construct queue of depth 2 with 3 elements"));

      t.pop();
      check_equality(t, queue_t{3}, LINE(""));

      t.push(5);
      check_equality(t, queue_t{5, 3}, LINE(""));
      
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
      check_equality(s.top(), 2, LINE(""));

      check_regular_semantics(s, t, LINE("Standard semantics"));
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
    check_equality(s, queue_t{8, 2, 6}, LINE(""));

    auto t{s};
    check_equality(t, queue_t{8, 2, 6}, LINE(""));

    t.pop();
    check_equality(t, queue_t{6, 2}, LINE(""));

    check_regular_semantics(s, t, LINE("Standard semantics"));

    t.pop();
    check_equality(t, queue_t{2}, LINE(""));

    t.pop();
    check_equality(t, queue_t{}, LINE(""));
  }
}
