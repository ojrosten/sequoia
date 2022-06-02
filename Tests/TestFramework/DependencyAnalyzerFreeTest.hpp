////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/FreeTestCore.hpp"

namespace sequoia::testing
{
  class dependency_analyzer_free_test final : public free_test
  {
  public:
    using free_test::free_test;

    using test_list           = std::vector<std::filesystem::path>;
    using opt_test_list       = std::optional<test_list>;
    using multi_test_list     = std::vector<test_list>;

  private:
    enum class status { stale, fresh };

    struct passing_tests
    {
      std::vector<std::filesystem::path> tests{};
      status status{status::stale};
    };

    struct data
    {
      data(opt_test_list fail, opt_test_list pass);

      opt_test_list failures{}, passes{};
    };

    std::filesystem::file_time_type m_ResetTime{};

    [[nodiscard]]
    std::string_view source_file() const noexcept final;

    void run_tests() final;

    void test_exceptions(const project_paths& projPaths);

    void test_dependencies(const project_paths& projPaths);

    void test_prune_update(const project_paths& projPaths);

    void test_instability_analysis_prune_upate(const project_paths& projPaths);

    void check_tests_to_run(std::string_view description,
                            const project_paths& projPaths,
                            std::string_view cutoff,
                            const std::vector<std::filesystem::path>& makeStale,
                            std::vector<std::filesystem::path> failures,
                            passing_tests passes,
                            const std::vector<std::filesystem::path>& toRun);

    void check_data(std::string_view description, const data& obtained, const data& prediction);

    static auto read(const std::filesystem::path& file) -> opt_test_list;

    static void write_or_remove(const project_paths& projPaths, const std::filesystem::path& file, const opt_test_list& tests);

    static void write_or_remove(const project_paths& projPaths, const std::filesystem::path& failureFile, const std::filesystem::path& passesFile, const data& d);
  };
}
