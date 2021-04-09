////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/TestFramework/RegularTestCore.hpp"

#include <vector>

namespace sequoia::testing
{
  class threading_models_test final : public regular_test
  {
  public:
    using regular_test::regular_test;

    [[nodiscard]]
    std::string_view source_file() const noexcept final;
  private:
    void run_tests() final;

    void test_task_queue();

    template<class ThreadModel, class Exception, class... Args>
    void test_exceptions(std::string_view message, Args&&... args);

    template<class Model> void test_functor_update();
  };

  class updatable
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
