#include "TestHeterogeneousNodeStorage.hpp"

namespace sequoia::unit_testing
{
  void test_heterogeneous_node_storage::run_tests()
  {
    storage_tester<int, double> s{3, 0.8};

    check_equality(3, s.node_weight<0>(), LINE(""));
    check_equality(3, s.node_weight<int>(), LINE(""));

    check_equality(0.8, s.node_weight<1>(), LINE(""));
    check_equality(0.8, s.node_weight<double>(), LINE(""));

    s.node_weight<0>(-4);
    check_equality(-4, s.node_weight<0>(), LINE(""));
    check_equality(-4, s.node_weight<int>(), LINE(""));

    s.node_weight<int>(10);
    check_equality(10, s.node_weight<0>(), LINE(""));
    check_equality(10, s.node_weight<int>(), LINE(""));

    s.node_weight<1>(3.2);
    check_equality(3.2, s.node_weight<1>(), LINE(""));
    check_equality(3.2, s.node_weight<double>(), LINE(""));

    s.node_weight<double>(10.4);
    check_equality(10.4, s.node_weight<1>(), LINE(""));
    check_equality(10.4, s.node_weight<double>(), LINE(""));

    s.mutate_node_weight<0>([](int& i) { return i+=5; });
    check_equality(15, s.node_weight<0>(), LINE(""));
    check_equality(15, s.node_weight<int>(), LINE(""));

    s.mutate_node_weight<int>([](int& i) { return i/=5; });
    check_equality(3, s.node_weight<0>(), LINE(""));
    check_equality(3, s.node_weight<int>(), LINE(""));

    s.mutate_node_weight<1>([](double& d) { return d-=4.4; });
    check_equality(6.0, s.node_weight<1>(), LINE(""));
    check_equality(6.0, s.node_weight<double>(), LINE(""));

    s.mutate_node_weight<double>([](double& d) { return d/=6; });
    check_equality(1.0, s.node_weight<1>(), LINE(""));
    check_equality(1.0, s.node_weight<double>(), LINE(""));
  }
}
