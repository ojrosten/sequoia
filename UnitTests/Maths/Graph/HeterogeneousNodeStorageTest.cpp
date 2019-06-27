////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "HeterogeneousNodeStorageTest.hpp"

namespace sequoia::unit_testing
{
  constexpr auto test_heterogeneous_node_storage::make_storage() -> storage_tester<float, int>
  {
    storage_tester<float, int> s{};

    s.node_weight<0>(2.0f);
    s.node_weight<int>(4);

    s.mutate_node_weight<float>([](float& f) { f+= 1.0; });
    s.mutate_node_weight<int>([](int& i){ i -= 2; });

    return s;
  }
  
  void test_heterogeneous_node_storage::run_tests()
  {
    storage_tester<int, double> s{3, 0.8};

    check_equality(LINE(""), s.node_weight<0>(), 3);
    check_equality(LINE(""), s.node_weight<int>(), 3);

    check_equality(LINE(""), s.node_weight<1>(), 0.8);
    check_equality(LINE(""), s.node_weight<double>(), 0.8);

    s.node_weight<0>(-4);
    check_equality(LINE(""), s.node_weight<0>(), -4);
    check_equality(LINE(""), s.node_weight<int>(), -4);

    s.node_weight<int>(10);
    check_equality(LINE(""), s.node_weight<0>(), 10);
    check_equality(LINE(""), s.node_weight<int>(), 10);

    s.node_weight<1>(3.2);
    check_equality(LINE(""), s.node_weight<1>(), 3.2);
    check_equality(LINE(""), s.node_weight<double>(), 3.2);

    s.node_weight<double>(10.4);
    check_equality(LINE(""), s.node_weight<1>(), 10.4);
    check_equality(LINE(""), s.node_weight<double>(), 10.4);

    s.mutate_node_weight<0>([](int& i) { return i+=5; });
    check_equality(LINE(""), s.node_weight<0>(), 15);
    check_equality(LINE(""), s.node_weight<int>(), 15);

    s.mutate_node_weight<int>([](int& i) { return i/=5; });
    check_equality(LINE(""), s.node_weight<0>(), 3);
    check_equality(LINE(""), s.node_weight<int>(), 3);

    s.mutate_node_weight<1>([](double& d) { return d-=4.4; });
    check_equality(LINE(""), s.node_weight<1>(), 6.0);
    check_equality(LINE(""), s.node_weight<double>(), 6.0);

    s.mutate_node_weight<double>([](double& d) { return d/=6; });
    check_equality(LINE(""), s.node_weight<1>(), 1.0);
    check_equality(LINE(""), s.node_weight<double>(), 1.0);

    constexpr storage_tester<float, int> t{make_storage()};

    check_equality(LINE(""), t.node_weight<0>(), 3.0f);
    check_equality(LINE(""), t.node_weight<float>(), 3.0f);

    check_equality(LINE(""), t.node_weight<1>(), 2);
    check_equality(LINE(""), t.node_weight<int>(), 2);
  }
}
