////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "PerformanceTestCore.hpp"

namespace sequoia::unit_testing
{
  class threading_models_performance_test final : public performance_test
  {      
  public:
    using performance_test::performance_test;

    [[nodiscard]]
    std::string_view source_file_name() const noexcept final;
  private:
    void run_tests() final;

    void test_waiting_task(const std::chrono::milliseconds millisecs);
    void test_waiting_task_return(const std::chrono::milliseconds millisecs);    
    void check_return_values(std::string_view message, performance_results<std::vector<int>>&& futures);        


    template<class ThreadModel, class... Args>
    void waiting_task(const std::size_t nTasks, const std::chrono::milliseconds millisecs, Args&&... args);

    template<class ThreadModel, class... Args>
    std::vector<int> waiting_task_return(const std::size_t nTasks, const std::chrono::milliseconds millisecs, Args&&... args);    
  };

  class wait
  {
    std::chrono::milliseconds m_Wait;
  public:      
    wait(const std::chrono::milliseconds millisecs) : m_Wait{millisecs} {}
      
    void operator()() const
    {
      std::this_thread::sleep_for(m_Wait);
    }
  };
}
