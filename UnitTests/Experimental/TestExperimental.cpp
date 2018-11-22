#include "TestExperimental.hpp"

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>

namespace sequoia::unit_testing
{
  template<class R, class Locking=std::try_to_lock_t, class Task=std::packaged_task<R()>, class Q=std::queue<Task>>
  class task_queue
  {
  public:
    task_queue() = default;
    task_queue(const task_queue&) = delete;
    task_queue(task_queue&&)      = delete;

    ~task_queue() = default;
    
    task_queue& operator=(const task_queue&) = delete;
    task_queue& operator=(task_queue&&)      = delete;
    
    template<class Fn, class... Args>
    bool push(Fn&& fn, Args... args)
    {
      std::unique_lock<std::mutex> lock{m_Mutex, Locking{}};
      if(!lock) return false;

      m_Q.emplace([=]() { return fn(args...); });
      m_CV.notify_one();
      
      return true;
    }

    Task pop()
    {
      std::unique_lock<std::mutex> lock{m_Mutex, Locking{}};
      if(!lock || m_Q.empty()) return Task{};

      auto task{std::move(m_Q.front())};
      m_Q.pop();

      return std::move(task);
    }
  private:
    
    Q m_Q;
    std::mutex m_Mutex;
    std::condition_variable m_CV;
  };

  
  void test_experimental::run_tests()
  {      
    task_queue<int> q{};

    q.push([](int x){ return x;}, 1);
    auto t{q.pop()};
  }
}
