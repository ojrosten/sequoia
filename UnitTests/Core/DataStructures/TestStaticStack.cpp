#include "TestStaticStack.hpp"
#include "StaticStackTestingUtilities.hpp"

#include "StaticStack.hpp"

namespace sequoia::unit_testing
{
  void test_static_stack::run_tests()
  {
    check_depth_1();
    check_constexpr_stack();
  }

  void test_static_stack::check_depth_1()
  {
    using namespace data_structures;

    static_stack<int, 1> s{}, t{};
    s.push(1);
    t.push(2);

    check_standard_semantics(s, t, LINE("Standard Semantics"));
  }

  constexpr auto test_static_stack::make_static_stack()
  {
    using namespace data_structures;

    static_stack<int, 1> s{};

    s.push(10);
    s.pop();
    s.push(11);

    return s;
  }

  void test_static_stack::check_constexpr_stack()
  {
    using namespace data_structures;
    
    constexpr auto s{make_static_stack()};

    static_stack<int, 1> t{};
    t.push(11);
    
    check_equality(s, t, LINE("constexpr stack"));
  }


}
