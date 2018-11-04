#include "StaticStackTestingDiagnostics.hpp"
#include "StaticStackTestingUtilities.hpp"

namespace sequoia::unit_testing
{
  void test_static_stack_false_positives::run_tests()
  {
    using namespace data_structures;

    static_stack<int, 1> s{}, t{};
    t.push(1);

    check_equality(s, t, "Empty stack versus populated stack");
  }
}
