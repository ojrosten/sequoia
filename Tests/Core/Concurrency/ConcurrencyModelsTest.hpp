////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/RegularTestCore.hpp"

#include <vector>

namespace sequoia::testing
{
  class threading_models_test final : public free_test
  {
  public:
    using free_test::free_test;

    [[nodiscard]]
    std::filesystem::path source_file() const;

    void run_tests();
  private:

    void test_task_queue();

    template<class ThreadModel, class... Args>
    void test_exceptions(std::string_view message, Args&&... args);

    template<class ThreadModel, class... Args>
    void test_execution(std::string_view message, Args&&... args);

    void test_serial_exceptions();
    void test_serial_execution();
  };
}
