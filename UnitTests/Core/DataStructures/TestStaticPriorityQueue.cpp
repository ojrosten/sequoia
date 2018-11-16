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

    constexpr static_priority_queue<int, 1> s{1};
    static_priority_queue<int, 1> t{};
    t.push(2);

    check_standard_semantics(s, t, LINE("Standard Semantics"));

    check_exception_thrown<std::logic_error>([&t]() { t.push(1); }, LINE("Trying to push two elements to queue of depth 1"));
    check_exception_thrown<std::logic_error>([]() { static_priority_queue<int, 1>{1, 2}; }, LINE("Can't construct queue of depth 1 with 2 elements"));

    t.pop();
    check_equality<std::size_t>(0, t.size(), LINE(""));
    check(t.empty(), LINE(""));
    
    t.push(1);
    check_equality<std::size_t>(1, t.size(), LINE(""));
    check(!t.empty(), LINE(""));
  }

  void test_static_priority_queue::check_depth_2()
  {
    using namespace data_structures;

    {
      constexpr static_priority_queue<int, 2> s{1, 2};
      static_priority_queue<int, 2> t{4, 3};

      check_equality(2, s.top(), LINE(""));
      check_equality(4, t.top(), LINE(""));
      check_equality<std::size_t>(2, t.size(), LINE(""));
      check(!t.empty(), LINE(""));
      
      check_standard_semantics(s, t, LINE("Standard Semantics"));

      t.pop();
      check_equality(3, t.top(), LINE(""));
      check_equality<std::size_t>(1, t.size(), LINE(""));
      check(!t.empty(), LINE(""));

      t.push(5);
      check_equality(5, t.top(), LINE(""));
      check_equality<std::size_t>(2, t.size(), LINE(""));
      check(!t.empty(), LINE(""));
      
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
      
      constexpr static_priority_queue<int, 2, comp> s{{3, 2}, comp{3}};
      check_equality(2, s.top(), LINE(""));
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

    constexpr static_priority_queue<int, 3> s{make_static_priority_queue_3()};

    check_equality(8, s.top(), LINE(""));

    auto t{s};
    check_equality(8, t.top(), LINE(""));
    check_equality<std::size_t>(3, t.size(), LINE(""));
    check(!t.empty(), LINE(""));

    t.pop();
    check_equality(6, t.top(), LINE(""));
    check_equality<std::size_t>(2, t.size(), LINE(""));
    check(!t.empty(), LINE(""));

    t.pop();
    check_equality(2, t.top(), LINE(""));
    check_equality<std::size_t>(1, t.size(), LINE(""));
    check(!t.empty(), LINE(""));

    t.pop();
    check_equality<std::size_t>(0, t.size(), LINE(""));
    check(t.empty(), LINE(""));
  }
}
