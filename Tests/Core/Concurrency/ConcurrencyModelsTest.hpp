////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "sequoia/TestFramework/RegularTestCore.hpp"

#include <vector>

namespace sequoia::testing
{
  class threading_models_test final : public free_test
  {
  public:
    using free_test::free_test;

    [[nodiscard]]
    static std::filesystem::path source_file();

    void run_tests();
  private:

    void test_task_queue();

    void test_try_lock_successes();

    void test_try_lock_failures();

    void test_pushes_wake_a_waiting_pop();

    void test_pushes_to_a_finished_queue_throw();

    void test_thread_pools_refuse_zero_threads();

    template<class ThreadPool>
    void test_every_task_pushed_from_several_threads_runs(std::string_view poolKind);

    template<class ThreadPool>
    void test_every_push_during_join_runs_or_throws(std::string_view poolKind);

    template<class ThreadPool>
    void test_a_push_after_join_throws(std::string_view poolKind);

    template<class ThreadModel, class... Args>
    void test_exceptions(std::string_view message, Args&&... args);

    template<class ThreadModel, class... Args>
    void test_execution(std::string_view message, Args&&... args);

    void test_serial_exceptions();
    void test_serial_execution();
  };
}
