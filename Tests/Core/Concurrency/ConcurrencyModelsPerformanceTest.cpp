////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "ConcurrencyModelsPerformanceTest.hpp"
#include "sequoia/Core/Concurrency/ConcurrencyModels.hpp"

namespace sequoia::testing
{
  using namespace concurrency;

  namespace
  {
    template<std::invocable Task, class ThreadModel>
    class waiting_task
    {
    public:
      using return_type = typename ThreadModel::return_type;

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
  std::filesystem::path threading_models_performance_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void threading_models_performance_test::run_tests()
  {
    test_waiting_task(std::chrono::milliseconds{15});
    test_waiting_task_return(std::chrono::milliseconds{15});
  }

  void threading_models_performance_test::test_waiting_task(const std::chrono::milliseconds millisecs)
  {
    {
      auto threadPoolFn{[millisecs](){ waiting_task<wait, thread_pool<void>>{2u, wait{millisecs}, 2u}(); }};

      auto threadPoolMonoFn{[millisecs]() { waiting_task<wait, thread_pool<void, false>>{2u, wait{millisecs}, 2u}(); }};

      auto nullThreadFn{[millisecs]() { waiting_task<wait, serial<void>>{2u, wait{millisecs}}(); }};

      auto asyncFn{[millisecs]() { waiting_task<wait, asynchronous<void>>{2u, wait{millisecs}}(); }};

      check_relative_performance(report("Two Waiting tasks; pool_2/null"), threadPoolFn, nullThreadFn, 1.9, 2.1);
      check_relative_performance(report("Two Waiting tasks; pool_2M/null"), threadPoolMonoFn, nullThreadFn, 1.9, 2.1);
      check_relative_performance(report("Two Waiting tasks; async/null"), asyncFn, nullThreadFn, 1.9, 2.1);
    }

    {
      auto threadPoolFn{[millisecs](){ waiting_task<wait, thread_pool<void>>{4u, wait{millisecs}, 2u}(); }};

      auto threadPoolMonoFn{[millisecs]() { waiting_task<wait, thread_pool<void, false>>{4u, wait{millisecs}, 2u}(); }};

      auto nullThreadFn{[millisecs]() { waiting_task<wait, serial<void>>{4u, wait{millisecs}}(); }};

      check_relative_performance(report("Four Waiting tasks; pool_2/null"), threadPoolFn, nullThreadFn, 1.9, 2.1);
      check_relative_performance(report("Four Waiting tasks; pool_2M/null"), threadPoolMonoFn, nullThreadFn, 1.9, 2.1);
    }

    {
      auto threadPoolFn{[millisecs](){ waiting_task<wait, thread_pool<void>>{4u, wait{millisecs}, 4u}(); }};

      auto threadPoolMonoFn{[millisecs]() { waiting_task<wait, thread_pool<void, false>>{4u, wait{millisecs}, 4u}(); }};

      auto nullThreadFn{[millisecs]() { waiting_task<wait, serial<void>>{4u, wait{millisecs}}(); }};

      check_relative_performance(report("Four Waiting tasks; pool_4/null"), threadPoolFn, nullThreadFn, 3.9, 4.1);
      check_relative_performance(report("Four Waiting tasks; pool_4M/null"), threadPoolMonoFn, nullThreadFn, 3.9, 4.1);
    }
  }

  void threading_models_performance_test::test_waiting_task_return(const std::chrono::milliseconds millisecs)
  {
    {
      auto waitReturnVal{ [millisecs](){ wait{millisecs}(); return 42; } };
      using wait_return = decltype(waitReturnVal);

      auto threadPoolFn{[waitReturnVal](){ return waiting_task<wait_return, thread_pool<int>>{2u, waitReturnVal, 2u}(); }};

      auto threadPoolMonoFn{[waitReturnVal](){ return waiting_task<wait_return, thread_pool<int, false>>{2u, waitReturnVal, 2u}(); }};

      auto nullThreadFn{[waitReturnVal]() { return waiting_task<wait_return, serial<int>>{2u, waitReturnVal}(); }};

      auto asyncFn{[waitReturnVal]() { return waiting_task<wait_return, asynchronous<int>>{2u, waitReturnVal}(); }};

      check_relative_performance(report("Two Waiting tasks; pool_2/null"), threadPoolFn, nullThreadFn, 1.9, 2.1);
      check_relative_performance(report("Two Waiting tasks; pool_2M/null"), threadPoolMonoFn, nullThreadFn, 1.9, 2.1);
      check_relative_performance(report("Two Waiting tasks; async/null"), asyncFn, nullThreadFn, 1.9, 2.1);
    }
  }
}
