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

    [[nodiscard]]
    std::filesystem::path source_file() const;

    void run_tests();
  private:
    using test_list       = std::vector<std::filesystem::path>;
    using opt_test_list   = std::optional<test_list>;
    using multi_test_list = std::vector<test_list>;

    enum class modification_time { very_early, early, late, very_late};

    struct updated_file
    {
      std::filesystem::path file;
      modification_time modification{modification_time::early};
    };

    struct passing_tests
    {
      std::vector<std::filesystem::path> tests{};
      modification_time modification{modification_time::early};
    };

    struct test_outcomes
    {
      test_outcomes(opt_test_list fail, opt_test_list pass);

      opt_test_list failures{}, passes{};
    };

    struct file_states
    {
      std::vector<updated_file> stale;
      std::vector<std::filesystem::path> to_run;
    };

    std::filesystem::file_time_type m_ResetTime{};

    void test_exceptions(const project_paths& projPaths);

    void test_dependencies(const project_paths& projPaths);

    void test_prune_update(const project_paths& projPaths);

    void test_instability_analysis_prune_upate(const project_paths& projPaths);

    void check_tests_to_run(const reporter& description,
                            const project_paths& projPaths,
                            std::string_view cutoff,
                            const file_states& fileStates,
                            std::vector<std::filesystem::path> failures,
                            passing_tests passes);

    void check_data(std::string_view description, const test_outcomes& obtained, const test_outcomes& prediction);

    [[nodiscard]]
    static std::chrono::seconds to_duration(modification_time modTime);

    static auto read(const std::filesystem::path& file) -> opt_test_list;

    static void write_or_remove(const project_paths& projPaths, const std::filesystem::path& file, const opt_test_list& tests);

    static void write_or_remove(const project_paths& projPaths, const std::filesystem::path& failureFile, const std::filesystem::path& passesFile, const test_outcomes& d);
  };
}
