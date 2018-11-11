#include "TestStaticQueue.hpp"
#include "StaticQueueTestingUtilities.hpp"

#include "StaticQueue.hpp"

namespace sequoia::unit_testing
{
  void test_static_queue::run_tests()
  {
    check_depth_1();
    check_constexpr_queue();
  }

  void test_static_queue::check_depth_1()
  {
    using namespace data_structures;

    static_queue<int, 1> s{}, t{};
    s.push(1);
    t.push(2);

    check_standard_semantics(s, t, LINE("Standard Semantics"));
  }

  constexpr auto test_static_queue::make_static_queue()
  {
    using namespace data_structures;

    static_queue<int, 2> s{};

    s.push(10);
    s.pop();
    s.push(11);

    return s;
  }

  void test_static_queue::check_constexpr_queue()
  {
    using namespace data_structures;
    
    constexpr auto s{make_static_queue()};

    static_queue<int, 2> t{};
    t.push(11);
    
    check(s != t, LINE("constexpr queue remembers its history"));
  }


}
