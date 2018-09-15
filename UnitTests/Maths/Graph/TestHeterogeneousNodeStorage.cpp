#include "TestHeterogeneousNodeStorage.hpp"

namespace sequoia::unit_testing
{
  void test_heterogeneous_node_storage::run_tests()
  {
    storage_tester<int, double> s{3, 0.8};
  }
}
