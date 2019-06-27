////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "UnitTestCore.hpp"

namespace sequoia::unit_testing
{
  class threading_models_test : public unit_test
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

    void check_return_values(std::string_view message, performance_results<std::vector<int>>&& futures);        

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
