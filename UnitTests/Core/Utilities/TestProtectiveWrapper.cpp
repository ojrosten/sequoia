////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestProtectiveWrapper.hpp"
#include "ProtectiveWrapperTestingUtilities.hpp"

namespace sequoia::unit_testing
{
  // a+1, b*2
  constexpr utilities::protective_wrapper<data> make(int a, double b)
  {
    utilities::protective_wrapper<data> d{a, b};
    d.set(a, b);
    d.mutate([](auto& e) { e.a+=1; e.b*=2; });

    return d;
  }
  
  void test_protective_wrapper::run_tests()
  {
    test_basic_type();
    test_container_type();
    test_aggregate_type();   
  }

  void test_protective_wrapper::test_basic_type()
  {
    using namespace utilities;

    using wrapper = protective_wrapper<int>;

    static_assert(sizeof(wrapper) == sizeof(int));

    wrapper w{};
    constexpr wrapper v{1};

    check_regular_semantics(w, v, LINE("Regular semantics"));

    w.set(2);

    check_equality(wrapper{2}, w, LINE(""));

    w.mutate([](auto& u) { u *=2; });

    check_equality(wrapper{4}, w, LINE(""));
  }
  
  void test_protective_wrapper::test_container_type()
  {
    using namespace utilities;

    using wrapper = protective_wrapper<std::vector<int>>;

    static_assert(sizeof(wrapper) == sizeof(std::vector<int>));

    wrapper w{}, v{1};

    check_regular_semantics(w, v, LINE("Regular semantics"));

    w.set(2);

    check_equality(wrapper{std::vector<int>{2}}, w, LINE(""));

    check_regular_semantics(w, v, LINE("Regular semantics"));

    v.mutate([](auto& u) { u.push_back(3); });

    check_equality(wrapper{std::vector<int>{1, 3}}, v, LINE(""));

    check_regular_semantics(w, v, LINE("Regular semantics"));
  }
  
  void test_protective_wrapper::test_aggregate_type()
  {
    using namespace utilities;

    using wrapper = protective_wrapper<data>;

    static_assert(sizeof(wrapper) == sizeof(data));

    wrapper w{};
    constexpr wrapper v{make(1, 2.0)};

    check_equality(wrapper{2, 4.0}, v, LINE(""));

    check_regular_semantics(w, v, LINE("Regular semantics"));
  }
}
