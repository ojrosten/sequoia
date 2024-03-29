////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/PerformanceTestCore.hpp"

namespace sequoia::testing
{
  class threading_models_performance_test final : public performance_test
  {
  public:
    using performance_test::performance_test;

    [[nodiscard]]
    std::filesystem::path source_file() const;

    void run_tests();
  private:

    void test_waiting_task(const std::chrono::milliseconds millisecs);
    void test_waiting_task_return(const std::chrono::milliseconds millisecs);
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
