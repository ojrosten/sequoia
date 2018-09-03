#include "TestSample.hpp"

namespace sequoia
{
  namespace unit_testing
  {
    void test_sample::run_tests()
    {
      test_single_data_set<double>();
      test_single_data_set<float>();
    }
  }
}
