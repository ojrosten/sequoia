#pragma once

#include "UnitTestUtils.hpp"

namespace sequoia::unit_testing
{
  class test_threading_models : public unit_test
  {      
  public:
    using unit_test::unit_test;
  private:
    void run_tests();

    void test_task_queue();

    void test_waiting_task(const std::chrono::milliseconds millisecs);
    void test_waiting_task_return(const std::chrono::milliseconds millisecs);

    template<class ThreadModel, class... Args>
    void waiting_task(const std::size_t nTasks, const std::chrono::milliseconds millisecs, Args&&... args);

    template<class ThreadModel, class... Args>
    std::vector<int> waiting_task_return(const std::size_t nTasks, const std::chrono::milliseconds millisecs, Args&&... args);

    void check_return_values(performance_results<std::vector<int>>&& futures, std::string_view message);        

    template<class ThreadModel, class Exception, class... Args>
    void test_exceptions(std::string_view message, Args&&... args);

    template<class Model> void test_functor_update();
  };

  class Wait
  {
    std::chrono::milliseconds m_Wait;
  public:      
    Wait(const std::chrono::milliseconds millisecs) : m_Wait{millisecs} {}
      
    void operator()() const
    {
      std::this_thread::sleep_for(m_Wait);
    }
  };

  class UpdatableFunctor
  {
  public:
    void operator()(const int x)
    {
      m_Data.push_back(x);
    }

    const auto& get_data() const { return m_Data; }
  private:
    std::vector<int> m_Data;
  };  
}
