////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "sequoia/TestFramework/DependencyAnalyzer.hpp"
#include "sequoia/TestFramework/FreeTestCore.hpp"

namespace sequoia::testing
{
  class dependency_analyzer_free_test final : public free_test
  {
  public:
    using free_test::free_test;

    [[nodiscard]]
    static std::filesystem::path source_file();

    void run_tests();
  private:
    using test_list       = std::vector<std::filesystem::path>;
    using test_selection  = std::variant<test_list, prune_fallback_reason>;
    using multi_test_list = std::vector<test_list>;

    using prune_records     = std::vector<prune_record>;
    using opt_prune_records = std::optional<prune_records>;

    enum class modification_time { very_early, early, late, very_late};

    struct updated_file
    {
      std::filesystem::path file;
      modification_time modification{modification_time::early};
    };

    struct test_outcomes
    {
      test_outcomes(opt_prune_records fail, opt_prune_records pass);

      opt_prune_records failures{}, passes{};
    };

    struct file_states
    {
      std::vector<updated_file> stale;
      std::vector<std::filesystem::path> to_run;
    };

    std::filesystem::file_time_type m_ResetTime{};

    void test_staleness_threshold();

    [[nodiscard]]
    static std::string normalise_out_of_date_message(const project_paths& paths, std::string message);

    void test_exceptions(const project_paths& projPaths);

    void test_dependencies(const project_paths& projPaths);

    void test_stamp_on_second_boundary(const project_paths& projPaths);

    void test_pass_recorded_in_the_modification_second(const project_paths& projPaths);

    void test_prune_record_round_trip(const project_paths& projPaths);

    void test_prune_update(const project_paths& projPaths);

    void test_instability_analysis_prune_upate(const project_paths& projPaths);

    void check_tests_to_run(const reporter& description,
                            const project_paths& projPaths,
                            const file_states& fileStates,
                            std::vector<prune_record> failures,
                            std::vector<prune_record> passes);

    void check_data(std::string_view description, const test_outcomes& obtained, const test_outcomes& prediction);

    void check_round_trip(const reporter& description,
                          const project_paths& projPaths,
                          const prune_records& records);

    [[nodiscard]]
    static std::chrono::seconds to_duration(modification_time modTime);

    static auto read(const std::filesystem::path& file) -> opt_prune_records;

    enum class build_system { ninja, ninja_with_msvc, visual_studio };

    /// The directory, relative to the fake build tree, in which the fake build puts its objects, and their extension
    struct object_layout
    {
      std::filesystem::path directory{};
      std::string_view extension{};
    };

    [[nodiscard]]
    static object_layout objects_of(build_system system);

    /// Which of the fake project's sources the build's record names, and where it says they are
    enum class recorded_sources { all, all_but_the_tests, all_under_another_root, sequoia_relative };

    void write_build_artefacts(const std::filesystem::path& fake, build_system system, recorded_sources sources);

    void test_recorded_sources(const project_paths& projPaths);

    /// A file of the fake project, and the file's modification time as an offset from the reset time
    struct timed_edit
    {
      std::filesystem::path file{};
      std::chrono::seconds offset{};
    };

    /// The outcome of checking sequoia on the fake project: the normalised refusal, if there is one, and the warnings
    struct sequoia_check
    {
      std::optional<std::string> refusal{};
      std::string warnings{};
    };

    [[nodiscard]]
    static sequoia_check check_sequoia(const project_paths& projPaths, const std::filesystem::path& sequoiaSources);

    [[nodiscard]]
    static std::string normalise_sequoia_message(const project_paths& projPaths, std::string message);

    [[nodiscard]]
    static std::string sequoia_refusal_message(build_system system, std::string_view file, std::string_view source);

    void check_sequoia_change(const reporter& description,
                              const project_paths& projPaths,
                              const std::vector<timed_edit>& edits,
                              const std::optional<std::string>& refusal);

    void test_sequoia_sources();

    void test_sequoia_change(const project_paths& projPaths, build_system system);

    void test_sequoia_change_without_configuration(const project_paths& projPaths);

    void test_sequoia_file_gone(const project_paths& projPaths);

    void test_sequoia_recorded_relative(const std::filesystem::path& fake, const project_paths& projPaths);

    void test_sequoia_change_not_checked(const project_paths& projPaths);

    void test_sequoia_target_within_build_tree(const std::filesystem::path& fake);

    static void write_or_remove(const project_paths& projPaths, const std::filesystem::path& file, const opt_prune_records& tests);

    static void write_or_remove(const project_paths& projPaths, const std::filesystem::path& failureFile, const std::filesystem::path& passesFile, const test_outcomes& d);
  };
}
