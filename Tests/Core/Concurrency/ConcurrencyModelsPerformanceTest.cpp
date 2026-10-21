////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "ConcurrencyModelsPerformanceTest.hpp"
#include "sequoia/Core/Concurrency/ConcurrencyModels.hpp"
#include "sequoia/PlatformSpecific/Helpers.hpp"

#include <functional>

namespace sequoia::testing
{
  using namespace concurrency;

  namespace
  {
    class wait
    {
      std::chrono::milliseconds m_Wait{};
    public:
      wait(const std::chrono::milliseconds millisecs) : m_Wait{millisecs} {}

      void operator()() const
      {
        precise_sleep_for(m_Wait);
      }
    };

    class wait_and_return
    {
      wait m_Wait;
    public:
      wait_and_return(const std::chrono::milliseconds millisecs) : m_Wait{millisecs} {}

      int operator()() const
      {
        m_Wait();
        return 42;
      }
    };

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

    /** A waiting_task whose tasks sleep, paired with a baseline.

        The baseline is a waiting_task with a model of its own, of the same
        kind, and as many tasks. None of its tasks sleeps.
     */
    template<std::invocable Task, class ThreadModel>
    class waiting_task_with_baseline
    {
    public:
      using waiting_task_type = waiting_task<Task, ThreadModel>;
      using by_reference_type = task_with_baseline<std::reference_wrapper<waiting_task_type>,
                                                   std::reference_wrapper<waiting_task_type>>;

      template<class... ModelArgs>
      waiting_task_with_baseline(const std::size_t nTasks,
                                 const std::chrono::milliseconds sleepDuration,
                                 const ModelArgs&... args)
        : m_Task{nTasks, Task{sleepDuration}, args...}
        , m_Baseline{nTasks, Task{std::chrono::milliseconds::zero()}, args...}
      {}

      [[nodiscard]]
      by_reference_type by_reference() { return {.task{m_Task}, .baseline{m_Baseline}}; }
    private:
      waiting_task_type m_Task,
                        m_Baseline;
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

  /* Each check judges the speed-up of the tasks net of their baselines. The
     purpose of a baseline is to dispatch its tasks and collect their results
     as its waiting_task does, so that the cost of doing so is not judged.

     A baseline pushes each of its tasks while no task sleeps. A waiting_task
     pushes all but its first task while earlier tasks sleep, and so hides the
     cost of those pushes. The baseline subtracts that cost anyway. So a model
     whose pushes are costly measures a speed-up above the ideal. The excess
     grows as the sleep shortens.

     Each pool is constructed before its checks. The checks take the pool's
     waiting_task by reference, so no trial times the start or join of the
     pool's threads.

     The lower bounds sit further below the ideal speed-up than a symmetric
     tolerance would put them, because a pool of N on a machine with N hardware
     threads has no headroom: the pool's threads and the thread waiting on them
     are N+1 runnable entities on N cores, and instrumentation costs the rest.
     A tighter bound does not distinguish a pool which failed to parallelise
     from a machine with nothing spare, which is the distinction these checks
     exist to make.

     The upper bounds are not slack: exceeding the ideal by more than the cost
     of the hidden pushes would mean the serial tasks' durations were wrong.

     This is a holding position. Bounds which have to be widened whenever the
     hardware changes are measuring the machine as much as the code.
  */
  void threading_models_performance_test::test_waiting_task(const std::chrono::milliseconds millisecs)
  {
    {
      waiting_task_with_baseline<wait, thread_pool<void>>        threadPoolTask{2u, millisecs, 2u};
      waiting_task_with_baseline<wait, thread_pool<void, false>> threadPoolMonoTask{2u, millisecs, 2u};
      waiting_task_with_baseline<wait, serial<void>>             nullThreadTask{2u, millisecs};
      waiting_task_with_baseline<wait, asynchronous<void>>       asyncTask{2u, millisecs};

      check_relative_performance("Two Waiting tasks; pool_2/null",
                                 threadPoolTask.by_reference(), nullThreadTask.by_reference(),
                                 {.prediction{.lower{1.8}, .upper{2.1}}});
      check_relative_performance("Two Waiting tasks; pool_2M/null",
                                 threadPoolMonoTask.by_reference(), nullThreadTask.by_reference(),
                                 {.prediction{.lower{1.8}, .upper{2.1}}});
      check_relative_performance("Two Waiting tasks; async/null",
                                 asyncTask.by_reference(), nullThreadTask.by_reference(),
                                 {.prediction{.lower{1.8}, .upper{2.1}}});
    }

    {
      waiting_task_with_baseline<wait, thread_pool<void>>        threadPoolTask{4u, millisecs, 2u};
      waiting_task_with_baseline<wait, thread_pool<void, false>> threadPoolMonoTask{4u, millisecs, 2u};
      waiting_task_with_baseline<wait, serial<void>>             nullThreadTask{4u, millisecs};

      check_relative_performance("Four Waiting tasks; pool_2/null",
                                 threadPoolTask.by_reference(), nullThreadTask.by_reference(),
                                 {.prediction{.lower{1.8}, .upper{2.1}}});
      check_relative_performance("Four Waiting tasks; pool_2M/null",
                                 threadPoolMonoTask.by_reference(), nullThreadTask.by_reference(),
                                 {.prediction{.lower{1.8}, .upper{2.1}}});
    }

    {
      waiting_task_with_baseline<wait, thread_pool<void>>        threadPoolTask{4u, millisecs, 4u};
      waiting_task_with_baseline<wait, thread_pool<void, false>> threadPoolMonoTask{4u, millisecs, 4u};
      waiting_task_with_baseline<wait, serial<void>>             nullThreadTask{4u, millisecs};

      check_relative_performance("Four Waiting tasks; pool_4/null",
                                 threadPoolTask.by_reference(), nullThreadTask.by_reference(),
                                 {.prediction{.lower{3.5}, .upper{4.1}}});
      check_relative_performance("Four Waiting tasks; pool_4M/null",
                                 threadPoolMonoTask.by_reference(), nullThreadTask.by_reference(),
                                 {.prediction{.lower{3.5}, .upper{4.1}}});
    }
  }

  void threading_models_performance_test::test_waiting_task_return(const std::chrono::milliseconds millisecs)
  {
    {
      waiting_task_with_baseline<wait_and_return, thread_pool<int>>        threadPoolTask{2u, millisecs, 2u};
      waiting_task_with_baseline<wait_and_return, thread_pool<int, false>> threadPoolMonoTask{2u, millisecs, 2u};
      waiting_task_with_baseline<wait_and_return, serial<int>>             nullThreadTask{2u, millisecs};
      waiting_task_with_baseline<wait_and_return, asynchronous<int>>       asyncTask{2u, millisecs};

      check_relative_performance("Two Waiting tasks; pool_2/null",
                                 threadPoolTask.by_reference(), nullThreadTask.by_reference(),
                                 {.prediction{.lower{1.8}, .upper{2.1}}});
      check_relative_performance("Two Waiting tasks; pool_2M/null",
                                 threadPoolMonoTask.by_reference(), nullThreadTask.by_reference(),
                                 {.prediction{.lower{1.8}, .upper{2.1}}});
      check_relative_performance("Two Waiting tasks; async/null",
                                 asyncTask.by_reference(), nullThreadTask.by_reference(),
                                 {.prediction{.lower{1.8}, .upper{2.1}}});
    }
  }
}
