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

    test_experimental::waiting_task(std::chrono::milliseconds{10});
  }

  void test_experimental::waiting_task(const std::chrono::milliseconds millisecs)
  {
    waiting_task<experimental::thread_pool<void>>(2u, millisecs, 2u);
  }

  template<class ThreadModel, class... Args>
  void test_experimental::waiting_task(const std::size_t nTasks, const std::chrono::milliseconds millisecs, Args&&... args)
  {
    ThreadModel model{std::forward<Args>(args)...};
    
    for(std::size_t i{}; i < nTasks; ++i)
    {
      model.push(Wait{millisecs});
    }

    model.join();
  }
}
