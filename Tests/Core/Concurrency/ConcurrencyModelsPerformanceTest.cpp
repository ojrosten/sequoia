////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "ConcurrencyModelsPerformanceTest.hpp"
#include "sequoia/Core/Concurrency/ConcurrencyModels.hpp"

#include <functional>

namespace sequoia::testing
{
  using namespace concurrency;

  namespace
  {
    template<std::invocable Task, class ThreadModel>
    class waiting_task
    {
    public:
      using return_type = ThreadModel::return_type;

      template<class... ModelArgs>
      waiting_task(const std::size_t nTasks, Task t, ModelArgs&&... args)
        : m_NumTasks{nTasks}
        , m_Task{std::move(t)}
        , m_Model{std::forward<ModelArgs>(args)...}
      {}

      void operator()()
        requires std::is_void_v<return_type>
      {
        std::vector<std::future<void>> futures{};
        futures.reserve(m_NumTasks);

        for(std::size_t i{}; i < m_NumTasks; ++i)
        {
          futures.emplace_back(m_Model.push(m_Task));
        }

        for(auto& f : futures) f.get();
      }

      std::vector<return_type> operator()()
        requires (!std::is_void_v<return_type>)
      {
        std::vector<std::future<return_type>> futures{};
        futures.reserve(m_NumTasks);

        for(std::size_t i{}; i < m_NumTasks; ++i)
        {
          futures.emplace_back(m_Model.push(m_Task));
        }

        std::vector<return_type> results{};
        results.reserve(m_NumTasks);
        for(auto& f : futures) results.emplace_back(f.get());

        return results;
      }
    private:
      std::size_t m_NumTasks{};
      Task m_Task;
      ThreadModel m_Model;
    };

    template<std::invocable Task>
    class waiting_task<Task, serial<void>>
    {
    public:
      waiting_task(const std::size_t nTasks, Task t)
        : m_NumTasks{nTasks}
        , m_Task{std::move(t)}
      {}

      void operator()() const
      {
        serial<void> model{};
        for(std::size_t i{}; i < m_NumTasks; ++i)
        {
          model.push(m_Task);
        }
      }
    private:
      std::size_t m_NumTasks{};
      Task m_Task;
    };

    template<std::invocable Task, class R>
    class waiting_task<Task, serial<R>>
    {
    public:
      waiting_task(const std::size_t nTasks, Task t)
        : m_NumTasks{nTasks}
        , m_Task{std::move(t)}
      {}

      std::vector<R> operator()() const
      {
        serial<R> model{};
        std::vector<R> results{};
        results.reserve(m_NumTasks);
        for(std::size_t i{}; i < m_NumTasks; ++i)
        {
          results.emplace_back(model.push(m_Task));
        }

        return results;
      }
    private:
      std::size_t m_NumTasks{};
      Task m_Task;
    };
  }

  [[nodiscard]]
  std::filesystem::path threading_models_performance_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void threading_models_performance_test::run_tests()
  {
    test_waiting_task(std::chrono::milliseconds{15});
    test_waiting_task_return(std::chrono::milliseconds{15});
  }

  /* Each model is constructed once, before its checks, and the checks reach it
     by reference. A task which constructed its model would add the start and
     join of a pool's threads to every trial's timing.

     The lower bounds sit further below the ideal speed-up than a symmetric tolerance would put
     them, because a pool of N on a machine with N hardware threads has no headroom: the pool's
     threads and the thread waiting on them are N+1 runnable entities on N cores, and
     instrumentation costs the rest. A tighter bound does not distinguish a pool which failed to
     parallelise from a machine with nothing spare, which is the distinction these checks exist
     to make.

     The upper bounds are not slack: exceeding the ideal would mean the serial baseline was wrong.

     This is a holding position. Bounds which have to be widened whenever the hardware changes are
     measuring the machine as much as the code.
  */
  void threading_models_performance_test::test_waiting_task(const std::chrono::milliseconds millisecs)
  {
    {
      waiting_task<wait, thread_pool<void>>        threadPoolTask{2u, wait{millisecs}, 2u};
      waiting_task<wait, thread_pool<void, false>> threadPoolMonoTask{2u, wait{millisecs}, 2u};
      waiting_task<wait, serial<void>>             nullThreadTask{2u, wait{millisecs}};
      waiting_task<wait, asynchronous<void>>       asyncTask{2u, wait{millisecs}};

      check_relative_performance("Two Waiting tasks; pool_2/null",
                                 std::ref(threadPoolTask), std::ref(nullThreadTask),
                                 {.prediction{.lower{1.8}, .upper{2.1}}});
      check_relative_performance("Two Waiting tasks; pool_2M/null",
                                 std::ref(threadPoolMonoTask), std::ref(nullThreadTask),
                                 {.prediction{.lower{1.8}, .upper{2.1}}});
      check_relative_performance("Two Waiting tasks; async/null",
                                 std::ref(asyncTask), std::ref(nullThreadTask),
                                 {.prediction{.lower{1.8}, .upper{2.1}}});
    }

    {
      waiting_task<wait, thread_pool<void>>        threadPoolTask{4u, wait{millisecs}, 2u};
      waiting_task<wait, thread_pool<void, false>> threadPoolMonoTask{4u, wait{millisecs}, 2u};
      waiting_task<wait, serial<void>>             nullThreadTask{4u, wait{millisecs}};

      check_relative_performance("Four Waiting tasks; pool_2/null",
                                 std::ref(threadPoolTask), std::ref(nullThreadTask),
                                 {.prediction{.lower{1.8}, .upper{2.1}}});
      check_relative_performance("Four Waiting tasks; pool_2M/null",
                                 std::ref(threadPoolMonoTask), std::ref(nullThreadTask),
                                 {.prediction{.lower{1.8}, .upper{2.1}}});
    }

    {
      waiting_task<wait, thread_pool<void>>        threadPoolTask{4u, wait{millisecs}, 4u};
      waiting_task<wait, thread_pool<void, false>> threadPoolMonoTask{4u, wait{millisecs}, 4u};
      waiting_task<wait, serial<void>>             nullThreadTask{4u, wait{millisecs}};

      check_relative_performance("Four Waiting tasks; pool_4/null",
                                 std::ref(threadPoolTask), std::ref(nullThreadTask),
                                 {.prediction{.lower{3.5}, .upper{4.1}}});
      check_relative_performance("Four Waiting tasks; pool_4M/null",
                                 std::ref(threadPoolMonoTask), std::ref(nullThreadTask),
                                 {.prediction{.lower{3.5}, .upper{4.1}}});
    }
  }

  void threading_models_performance_test::test_waiting_task_return(const std::chrono::milliseconds millisecs)
  {
    {
      auto waitReturnVal{ [millisecs](){ wait{millisecs}(); return 42; } };
      using wait_return = decltype(waitReturnVal);

      waiting_task<wait_return, thread_pool<int>>        threadPoolTask{2u, waitReturnVal, 2u};
      waiting_task<wait_return, thread_pool<int, false>> threadPoolMonoTask{2u, waitReturnVal, 2u};
      waiting_task<wait_return, serial<int>>             nullThreadTask{2u, waitReturnVal};
      waiting_task<wait_return, asynchronous<int>>       asyncTask{2u, waitReturnVal};

      check_relative_performance("Two Waiting tasks; pool_2/null",
                                 std::ref(threadPoolTask), std::ref(nullThreadTask),
                                 {.prediction{.lower{1.8}, .upper{2.1}}});
      check_relative_performance("Two Waiting tasks; pool_2M/null",
                                 std::ref(threadPoolMonoTask), std::ref(nullThreadTask),
                                 {.prediction{.lower{1.8}, .upper{2.1}}});
      check_relative_performance("Two Waiting tasks; async/null",
                                 std::ref(asyncTask), std::ref(nullThreadTask),
                                 {.prediction{.lower{1.8}, .upper{2.1}}});
    }
  }
}
