////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/TestFramework/FreeTestCore.hpp"

namespace sequoia::testing
{
  class dependency_analyzer_free_test final : public free_test
  {
  public:
    using free_test::free_test;

  private:
    struct information
    {
      std::filesystem::path source_repo{}, tests_repo{}, materials{}, prune_file{};
      std::filesystem::file_time_type reset_time{};
      std::optional<std::filesystem::file_time_type> exe_time_stamp{};
    };

    information m_Info{};

    using test_list = std::optional<std::vector<std::filesystem::path>>;

    [[nodiscard]]
    std::string_view source_file() const noexcept final;

    void run_tests() final;

    void test_exceptions();

    void test_dependencies();

    void check_tests_to_run(std::string_view description, const information& info, std::string_view cutoff, const std::vector<std::filesystem::path>& makeStale, const std::vector<std::filesystem::path>& toRun);
  };
}
