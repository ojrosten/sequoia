#include "TestStaticQueue.hpp"
#include "StaticQueueTestingUtilities.hpp"

#include "StaticQueue.hpp"

namespace sequoia::unit_testing
{
  void test_static_queue::run_tests()
  {
    check_depth_0();
    check_depth_1();
    check_depth_2();
  }

  void test_static_queue::check_depth_0()
  {
    using namespace data_structures;

    constexpr static_queue<int, 0> s{};
    static_queue<int, 0> t{};

    check_equality(t, s, LINE("Equality of null queues"));

    check_exception_thrown<std::logic_error>([&t]() { t.push(1); }, LINE("Can't push to null queue"));
    check_exception_thrown<std::logic_error>([]() { static_queue<int, 0>{1}; }, LINE("Can't construct non-null null queue"));
  }

  void test_static_queue::check_depth_1()
  {
    using namespace data_structures;

    constexpr static_queue<int, 1> s{1};
    static_queue<int, 1> t{};
    t.push(2);

    check_regular_semantics(s, t, LINE("Standard Semantics"));

    check_exception_thrown<std::logic_error>([&t]() { t.push(1); }, LINE("Trying to push two elements to queue of depth 1"));
    check_exception_thrown<std::logic_error>([]() { static_queue<int, 1>{1, 2}; }, LINE("Can't construct queue of depth 1 with 2 elements"));

    t.pop();
    t.push(1);
    check_equality(s, t, LINE(""));
  }

  constexpr auto test_static_queue::make_static_queue_2()
  {
    using namespace data_structures;

    static_queue<int, 2> s{};

    s.push(10);
    s.pop();
    s.push(11);

    return s;
  }
  
  void test_static_queue::check_depth_2()
  {
    using namespace data_structures;

    static_queue<int, 2> a{}, b{1}, c{3, 2};
    constexpr static_queue<int, 2> s{make_static_queue_2()};

    check_regular_semantics(a, b, LINE("Standard Semantics"));
    check_regular_semantics(b, c, LINE("Standard Semantics"));
    check_regular_semantics(a, c, LINE("Standard Semantics"));
    check_regular_semantics(b, s, LINE("Standard Semantics"));

    check_exception_thrown<std::logic_error>([]() { static_queue<int, 2>{1, 2, 3}; }, LINE("Can't construct queue of depth 2 with 3 elements"));

    check_equality<std::size_t>(0, a.size(), LINE(""));
    check(a.empty(), LINE(""));
    
    a.push(5);
    a.push(7);
    check_equality(static_queue<int, 2>{5, 7}, a, LINE(""));
    
    check_exception_thrown<std::logic_error>([&a]() { a.push(0); }, LINE("Trying to push 3 elements to a queue of depth 2"));
    
    a.pop();
    check_equality(static_queue<int, 2>{7}, a, LINE(""));

    a.push(4);
    check_equality(static_queue<int, 2>{7, 4}, a, LINE(""));

    check_exception_thrown<std::logic_error>([&a]() { a.push(0); }, LINE("Trying to push 3 elements to a queue of depth 2"));

    a.pop();
    check_equality(static_queue<int, 2>{4}, a, LINE(""));

    a.pop();
    check_equality(static_queue<int, 2>{}, a, LINE(""));
  }
}
