////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "sequoia/PlatformSpecific/Macros.hpp"
#include "sequoia/TestFramework/Macros.hpp"

import std;
import sequoia.test_framework;

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

    template<class ThreadModel, class... Args>
    void test_exceptions(std::string_view message, Args&&... args);

    template<class ThreadModel, class... Args>
    void test_execution(std::string_view message, Args&&... args);

    void test_serial_exceptions();
    void test_serial_execution();
  };
}
