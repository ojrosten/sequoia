#include "TestStaticPriorityQueue.hpp"
#include "StaticPriorityQueueTestingUtilities.hpp"

#include "StaticPriorityQueue.hpp"

namespace sequoia::unit_testing
{
  void test_static_priority_queue::run_tests()
  {
    check_depth_1();
    check_constexpr_priority_queue();
  }

  void test_static_priority_queue::check_depth_1()
  {
    using namespace data_structures;

    static_priority_queue<int, 1> s{}, t{};
    s.push(1);
    t.push(2);

    check_standard_semantics(s, t, LINE("Standard Semantics"));
  }

  constexpr auto test_static_priority_queue::make_static_priority_queue()
  {
    using namespace data_structures;

    static_priority_queue<int, 4> s{};

    s.push(0);
    s.pop();
    s.push(6);
    s.push(2);
    s.push(8);

    return s;
  }

  void test_static_priority_queue::check_constexpr_priority_queue()
  {
    using namespace data_structures;
    
    constexpr auto s{make_static_priority_queue()};
    check_equality(s.top(), 8, LINE(""));

    auto t{s};
    check_equality(t.top(), 8, LINE(""));
    t.pop();
    check_equality(t.top(), 6, LINE(""));
    t.pop();
    check_equality(t.top(), 2, LINE(""));
  }


}
