#include "TestExperimental.hpp"

namespace sequoia::unit_testing
{
  
  void test_experimental::run_tests()
  {
    using namespace experimental;

    using q_t = task_queue<int>;
    using task_t = q_t::task_t;

    q_t q{};

    q.push(task_t{[](){ return 1;}}, std::try_to_lock);
    auto t{q.pop(std::try_to_lock)};

    q.push(task_t{[](){ return 1;}});
    t = q.pop();

    q.finish();
  }
}
