////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "DependencyAnalyzerFreeTest.hpp"
#include "Parsing/CommandLineArgumentsTestingUtilities.hpp"

#include "sequoia/TestFramework/StateTransitionUtilities.hpp"
#include "sequoia/TextProcessing/Patterns.hpp"

#include <fstream>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    constexpr auto earlyExecutableOffset{std::chrono::seconds{-1}};
    constexpr auto resetOffset{std::chrono::seconds{0}};
    constexpr auto earlyPassOffset{std::chrono::seconds{1}}; // very_early
    constexpr auto earlyEditOffset{std::chrono::seconds{2}};  // early
    constexpr auto lateExecutableOffset{std::chrono::seconds{3}};
    constexpr auto latePassOffset{std::chrono::seconds{4}};   // late
    constexpr auto lateEditOffset{std::chrono::seconds{5}};   // very_late

        enum node_names : std::size_t
    {
      null_fails_null_passes,
      empty_fails_null_passes,
      empty_fails_empty_passes,
      house_fails_null_passes,
      house_fails_empty_passes,
      empty_fails_house_passes,
      house_prob_fails_null_passes,
      house_prob_fails_empty_passes,
      house_fails_prob_passes,
      house_prob_maybe_fails_empty_passes,
      house_prob_fails_maybe_passes,
      house_fails_maybe_prob_passes,
      empty_fails_maybe_house_prob_passes,
      house_fails_late_empty_passes
    };
  }

  dependency_analyzer_free_test::test_outcomes::test_outcomes(opt_prune_records fail, opt_prune_records pass)
    : failures{std::move(fail)}
    , passes{std::move(pass)}
  {
    if(failures) std::ranges::sort(*failures);
    if(passes)   std::ranges::sort(*passes);
  }

  [[nodiscard]]
  std::chrono::seconds dependency_analyzer_free_test::to_duration(modification_time modTime)
  {
    using enum modification_time;
    switch(modTime)
    {
    case very_early:
      return earlyPassOffset;
    case early:
      return earlyEditOffset;
    case late:
      return latePassOffset;
    case very_late:
      return lateEditOffset;
    }

    throw std::logic_error{"Unrecognized option for modification_time"};
  }

  auto dependency_analyzer_free_test::read(const fs::path& file) -> opt_prune_records
  {
    if(fs::exists(file)) return read_tests(file);

    return std::nullopt;
  }

  void dependency_analyzer_free_test::write_or_remove(const project_paths& projPaths, const fs::path& file, const opt_prune_records& tests)
  {
    if(tests) write_tests(projPaths, file, tests.value());
    else      fs::remove(file);
  }

  void dependency_analyzer_free_test::write_or_remove(const project_paths& projPaths, const fs::path& failureFile, const fs::path& passesFile, const test_outcomes& d)
  {
    write_or_remove(projPaths, failureFile, d.failures);
    write_or_remove(projPaths, passesFile , d.passes);
  }

  [[nodiscard]]
  std::filesystem::path dependency_analyzer_free_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void dependency_analyzer_free_test::check_tests_to_run(const reporter& description,
                                                         const project_paths& projPaths,
                                                         std::string_view cutoff,
                                                         const file_states& fileStates,
                                                         std::vector<prune_record> failures,
                                                         std::vector<prune_record> passes)
  {
    std::ranges::sort(failures);
    std::ranges::sort(passes);

    const auto prune{projPaths.prune()};
    const auto failureFile{prune.failures(std::nullopt)};
    const auto passesFile{prune.selected_passes(std::nullopt)};
    write_tests(projPaths, failureFile, failures);
    write_tests(projPaths, passesFile, passes);

    if(!passes.empty())
      fs::last_write_time(passesFile, std::ranges::max(passes, {}, [](const prune_record& r){ return r.time_stamp; }).time_stamp);

    for(const auto& f : fileStates.stale)
    {
      fs::last_write_time(f.file, m_ResetTime + to_duration(f.modification));
    }

    opt_test_list actual{tests_to_run(projPaths, cutoff)};

    opt_test_list prediction{fileStates.to_run};
    std::ranges::sort(*prediction);

    check(equality, description, actual, prediction);

    for(const auto& f : fileStates.stale)
    {
      fs::last_write_time(f.file, m_ResetTime);
    }

    fs::remove(failureFile);
    fs::remove(passesFile);

    check(equality, append_lines(description.message(), "Nothing Stale"), tests_to_run(projPaths, cutoff), opt_test_list{test_list{}});
  }

  void dependency_analyzer_free_test::run_tests()
  {
    m_ResetTime = std::chrono::file_clock::now() + resetOffset;

    const auto fake{auxiliary_materials() /= "FakeProject"};
    const main_paths main{fake / main_paths::default_main_cpp_from_root()};
    commandline_arguments args{{(fake / "build/CMade/TestAll/TestAll").generic_string()}};
    const project_paths projPaths{args.size(), args.get(), {.additional_dependency_analysis_paths{{"TestUtilities"}, {"dependencies/foo/Source"}}, .main_cpp{main.file()}, .common_includes{main.file()}}};

    check(equality, "No timestamp", tests_to_run(projPaths, ""), opt_test_list{});

    const auto prunePaths{projPaths.prune()};
    fs::create_directories(prunePaths.dir());
    { std::ofstream s{prunePaths.stamp()}; }

    for(auto& entry : fs::recursive_directory_iterator(fake))
    {
      fs::last_write_time(entry.path(), m_ResetTime);
    }

    test_exceptions(projPaths);
    test_dependencies(projPaths);
    test_prune_update(projPaths);
    test_instability_analysis_prune_upate(projPaths);
  }

  void dependency_analyzer_free_test::test_exceptions(const project_paths& projPaths)
  {
    check_exception_thrown<std::runtime_error>(
      "Executable out of date",
      [this, projPaths]() {
        fs::last_write_time(projPaths.executable(), m_ResetTime + earlyExecutableOffset);
        return tests_to_run(projPaths, "");
      },
      [](const project_paths& paths, std::string message) {
        message = default_exception_message_postprocessor{}(paths, std::move(message));
        {
          const auto [first, last]{find_sandwiched_text(message, "fakeProject", "time")};
          if(first < last)
            message.replace(first, last - first, "/xxFILExx ");
        }

        std::string::size_type pos{};
        while(pos < message.size())
        {
          std::string_view remaining{message.data() + pos, message.size() - pos};
          const auto [first, last]{find_sandwiched_text(remaining, ":", "\n")};
          if(first >= last)
            break;

          pos += first;
          message.replace(pos, last - first, "****");
          pos += 4;
        }

        return message;
      }
    );
  }

  void dependency_analyzer_free_test::test_dependencies(const project_paths& projPaths)
  {
    fs::last_write_time(projPaths.executable(), m_ResetTime + lateExecutableOffset);

    const auto& testRepo{projPaths.tests().repo()};
    const auto& sourceRepo{projPaths.source().project()};
    const auto testUtilsPath{projPaths.project_root() / "TestUtilities"};
    const auto fooPath{projPaths.project_root() / "dependencies" / "foo" / "Source"};
    const auto& materials{projPaths.test_materials().repo()};

    check_tests_to_run("Nothing stale", projPaths, "", {}, {}, {});

    fs::copy(projPaths.prune().external_dependencies(), working_materials());
    check(weak_equivalence, "External Dependencies", working_materials(), predictive_materials());

    check_tests_to_run("Test cpp stale (no cutoff)",
                       projPaths,
                       "",
                       {.stale{{{testRepo / "HouseAllocationTest.cpp"}, modification_time::early}}, .to_run{{"HouseAllocationTest.cpp"}}},
                       {},
                       {});

    check_tests_to_run("Test cpp naively stale, but has passed (when selected)",
                       projPaths,
                       "namespace",
                       {.stale{{{testRepo / "HouseAllocationTest.cpp"}, modification_time::early}}, .to_run{}},
                       {},
                       {{"HouseAllocationTest.cpp", m_ResetTime + to_duration(modification_time::late)}});

    check_tests_to_run("Test cpp stale; has previously passed (when selected), but this should be ignored",
                       projPaths,
                       "namespace",
                       {.stale{{{testRepo / "HouseAllocationTest.cpp"}, modification_time::early}}, .to_run{{"HouseAllocationTest.cpp"}}},
                       {},
                       {{"HouseAllocationTest.cpp", m_ResetTime + to_duration(modification_time::very_early)}});

    check_tests_to_run("Test hpp stale (no cutoff)",
                       projPaths,
                       "",
                       {.stale{{{testRepo / "HouseAllocationTest.hpp"}, modification_time::early}}, .to_run{{"HouseAllocationTest.cpp"}}},
                       {},
                       {});

    check_tests_to_run("Test hpp stale",
                       projPaths,
                       "namespace",
                       {.stale{{{testRepo / "HouseAllocationTest.hpp"}, modification_time::early}}, .to_run{{"HouseAllocationTest.cpp"}}},
                       {},
                       {});

    check_tests_to_run("Test utils stale",
                       projPaths,
                       "namespace",
                       {.stale{{{testRepo / "Maths" / "ProbabilityTestingUtilities.hpp"}, modification_time::early}},
                         .to_run{{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}}},
                       {},
                       {});

    check_tests_to_run("Reused utils stale",
                       projPaths,
                       "namespace",
                       {.stale{{{testRepo / "Stuff" / "OldschoolTestingUtilities.hpp"}, modification_time::early}},
                         .to_run{{"Maybe/MaybeTest.cpp"}, {"Stuff/OldschoolTest.cpp"}, {"Stuff/OldschoolTestingDiagnostics.cpp"}}},
                       {},
                       {});

    check_tests_to_run("Reused utils stale, but one of the tests has passed",
                       projPaths,
                       "namespace",
                       {.stale{{{testRepo / "Stuff" / "OldschoolTestingUtilities.hpp"}, modification_time::early}},
                         .to_run{{"Stuff/OldschoolTest.cpp"}, {"Stuff/OldschoolTestingDiagnostics.cpp"}}},
                       {},
                       {{"Maybe/MaybeTest.cpp", m_ResetTime + to_duration(modification_time::late)}});

    check_tests_to_run("Reused utils stale, but two of the tests have passed",
                       projPaths,
                       "namespace",
                       {.stale{{{testRepo / "Stuff" / "OldschoolTestingUtilities.hpp"}, modification_time::early}},
                         .to_run{{"Stuff/OldschoolTestingDiagnostics.cpp"}}},
                       {},
                       {{"Maybe/MaybeTest.cpp"    , m_ResetTime + to_duration(modification_time::late)},
                        {"Stuff/OldschoolTest.cpp", m_ResetTime + to_duration(modification_time::late)}});

    check_tests_to_run("Reused utils stale, but two of the tests have passed and a different one has failed",
                       projPaths,
                       "namespace",
                       {.stale{{{testRepo / "Stuff" / "OldschoolTestingUtilities.hpp"}, modification_time::early}},
                         .to_run{{"Stuff/OldschoolTestingDiagnostics.cpp"}, {"HouseAllocationTest.cpp"}}},
                       {{"HouseAllocationTest.cpp" , m_ResetTime + to_duration(modification_time::late)}},
                       {{"Maybe/MaybeTest.cpp"    , m_ResetTime + to_duration(modification_time::late)},
                        {"Stuff/OldschoolTest.cpp", m_ResetTime + to_duration(modification_time::late)}});

    check_tests_to_run("Reused utils stale, relative path",
                       projPaths,
                       "namespace",
                       {.stale{{{testRepo / "Stuff" / "FooTestingUtilities.hpp"}, modification_time::early}},
                         .to_run{{"Stuff/FooTest.cpp"}, {"Stuff/FooTestingDiagnostics.cpp"}, {"Utilities/Thing/UniqueThingTest.cpp"}, {"Utilities/Thing/UniqueThingTestingDiagnostics.cpp"}}},
                       {},
                       {});

    check_tests_to_run("Source cpp stale",
                       projPaths,
                       "namespace",
                       {.stale{{{sourceRepo / "Maths" / "Probability.cpp"}, modification_time::early}},
                         .to_run{{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}}},
                       {},
                       {});

    check_tests_to_run("Source hpp stale",
                       projPaths,
                       "namespace",
                       {.stale{{{sourceRepo / "Maths" / "Probability.hpp"}, modification_time::early}},
                         .to_run{{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}}},
                       {},
                       {});

    check_tests_to_run("Source hpp stale, following a previously successful run",
                       projPaths,
                       "namespace",
                       {.stale{{{sourceRepo / "Maths" / "Probability.hpp"}, modification_time::early}},
                         .to_run{{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}}},
                       {},
                       {{"Maths/ProbabilityTest.cpp"              , m_ResetTime + to_duration(modification_time::very_early)},
                        {"Maths/ProbabilityTestingDiagnostics.cpp", m_ResetTime + to_duration(modification_time::very_early)}});

    check_tests_to_run("Source cpp stale, following a previously successful run",
                       projPaths,
                       "namespace",
                       {.stale{{{sourceRepo / "Maths" / "Probability.cpp"}, modification_time::early}},
                         .to_run{{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}}},
                       {},
                       {{"Maths/ProbabilityTest.cpp"              , m_ResetTime + to_duration(modification_time::very_early)},
                        {"Maths/ProbabilityTestingDiagnostics.cpp", m_ResetTime + to_duration(modification_time::very_early)}});

    check_tests_to_run("Source cpp indirectly stale via included header",
                       projPaths,
                       "namespace",
                       {.stale{{{sourceRepo / "Maths" / "Helper.hpp"}, modification_time::early}},
                         .to_run{{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}}},
                       {},
                       {});

    check_tests_to_run("Source cpp indirectly stale via cpp definitions for included header",
                       projPaths,
                       "namespace",
                       {.stale{{{sourceRepo / "Maths" / "Helper.cpp"}, modification_time::early}},
                         .to_run{{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}}},
                       {},
                       {});

    check_tests_to_run("Source cpps indirectly stale via cpp from dependencies with the same name as a project cpp",
                       projPaths,
                       "namespace",
                       {.stale{{{fooPath / "foo" / "Utilities" / "Helper.cpp"}, modification_time::early}},
                         .to_run{{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}, {"Utilities/UsefulThingsFreeTest.cpp"}}},
                       {},
                       {});

    check_tests_to_run("Stale header in additional project",
                       projPaths,
                       "namespace",
                       {.stale{{{testUtilsPath / "myLib" / "Utils.hpp"}, modification_time::early}},
                        .to_run{{"Maybe/MaybeTest.cpp"}, {"Stuff/OldschoolTest.cpp"}, {"Stuff/OldschoolTestingDiagnostics.cpp"}}},
                       {},
                       {});

    check_tests_to_run("Stale cpp in additional project",
                       projPaths,
                       "namespace",
                       {.stale{{{testUtilsPath / "myLib" / "Utils.cpp"}, modification_time::early}},
                        .to_run{{"Maybe/MaybeTest.cpp"}, {"Stuff/OldschoolTest.cpp"}, {"Stuff/OldschoolTestingDiagnostics.cpp"}}},
                       {},
                       {});

    check_tests_to_run("Materials stale",
                       projPaths,
                       "namespace",
                       {.stale{{{materials / "Stuff" / "FooTest" / "Prediction" / "RepresentativeCasesTemp" / "NoSeqpat" / "baz.txt"}, modification_time::early}},
                         .to_run{{"Stuff/FooTest.cpp"}}},
                       {},
                       {});

    check_tests_to_run("Materials naively stale, but test previously passed (when selected)",
                       projPaths,
                       "namespace",
                       {.stale{{{materials / "Stuff" / "FooTest" / "Prediction" / "RepresentativeCasesTemp" / "NoSeqpat" / "baz.txt"}, modification_time::early}},
                         .to_run{}},
                       {},
                       {{"Stuff/FooTest.cpp", m_ResetTime + to_duration(modification_time::late)}});

    check_tests_to_run("Materials stale; test previously passed (when selected), but materials subsequently modified",
                       projPaths,
                       "namespace",
                       {.stale{{{materials / "Stuff" / "FooTest" / "Prediction" / "RepresentativeCasesTemp" / "NoSeqpat" / "baz.txt"}, modification_time::early}},
                         .to_run{{"Stuff/FooTest.cpp"}}},
                       {},
                       {{"Stuff/FooTest.cpp", m_ResetTime + to_duration(modification_time::very_early)}});

    check_tests_to_run("Materials stale; test previously passed (when selected); materials subsequently modified some early some late",
                       projPaths,
                       "namespace",
                       {.stale{{{materials / "Stuff" / "FooTest" / "Prediction" / "RepresentativeCasesTemp" / "NoSeqpat" / "baz.txt"}, modification_time::early},
                                {{materials / "Stuff" / "FooTest" / "Prediction" / "RepresentativeCasesTemp" / "NoSeqpat" / "baz2.txt"}, modification_time::very_late}},
                         .to_run{{"Stuff/FooTest.cpp"}}},
                       {},
                       {{"Stuff/FooTest.cpp", m_ResetTime + to_duration(modification_time::late)}});

    check_tests_to_run("Nothing stale, but a previous failure",
                       projPaths,
                       "namespace",
                       {.stale{}, .to_run{{"Maths/ProbabilityTest.cpp"}}},
                       {{"Maths/ProbabilityTest.cpp", m_ResetTime + to_duration(modification_time::early)}},
                       {});

    check_tests_to_run("Inconsistency: both passed and failed; failure wins",
                       projPaths,
                       "namespace",
                       {.stale{}, .to_run{{"Maths/ProbabilityTest.cpp"}}},
                       {{"Maths/ProbabilityTest.cpp" , m_ResetTime + to_duration(modification_time::late)}},
                       {{"Maths/ProbabilityTest.cpp", m_ResetTime + to_duration(modification_time::late)}});

    check_tests_to_run("Stale and a previous failure",
                       projPaths,
                       "namespace",
                       {.stale{{{testRepo / "Maths/ProbabilityTest.cpp"}, modification_time::early}}, .to_run{{"Maths/ProbabilityTest.cpp"}}},
                       {{"Maths/ProbabilityTest.cpp", m_ResetTime + to_duration(modification_time::late)}},
                       {});

    check_tests_to_run("Nothing stale, but two previous failures",
                       projPaths,
                       "namespace",
                       {.stale{}, .to_run{{"HouseAllocationTest.cpp"}, {"Maths/ProbabilityTest.cpp"}}},
                       {{"HouseAllocationTest.cpp"  , m_ResetTime + to_duration(modification_time::late)},
                        {"Maths/ProbabilityTest.cpp", m_ResetTime + to_duration(modification_time::late)}},
                       {});

    check_tests_to_run("Ensure that the staleness of a cpp isn't masked by a cpp which has freshly passed",
                       projPaths,
                       "namespace",
                       {
                         .stale{{{testRepo / "HouseAllocationTest.cpp"}  , modification_time::early},
                                {{testRepo / "Maths/ProbabilityTest.cpp"}, modification_time::early}},
                         .to_run{{"HouseAllocationTest.cpp"}}
                       },
                       {},
                       {{"HouseAllocationTest.cpp"  , m_ResetTime + to_duration(modification_time::very_early)},
                        {"Maths/ProbabilityTest.cpp", m_ResetTime + to_duration(modification_time::late)}});

  }

  void dependency_analyzer_free_test::test_prune_update(const project_paths& projPaths)
  {
    const auto updateTime{m_ResetTime};
    const auto lateUpdateTime{m_ResetTime + std::chrono::seconds{1}};
    const auto prune{projPaths.prune()};
    const auto failureFile{prune.failures(std::nullopt)};
    const auto passesFile{prune.selected_passes(std::nullopt)};

    using prune_graph = transition_checker<test_outcomes>::transition_graph;
    using edge_t = transition_checker<test_outcomes>::edge;

    auto update_unfiltered{
      [&](const test_outcomes& d, test_list failures) {
        write_or_remove(projPaths, failureFile, passesFile, d);

        update_prune_files(projPaths, std::move(failures), updateTime, std::nullopt);
        return test_outcomes{read(failureFile), {read(passesFile)}};
      }
    };

    auto update_filtered{
      [&](const test_outcomes& d, test_list executed, test_list failures, std::filesystem::file_time_type targetTime) {
        write_or_remove(projPaths, failureFile, passesFile, d);

        update_prune_files(projPaths, std::move(executed), std::move(failures), targetTime, std::nullopt);
        return test_outcomes{read(failureFile), {read(passesFile)}};
      }
    };

    const prune_graph g{
      { // Begin null_fails_null_passes
        { edge_t{empty_fails_null_passes,
                 "Nothing executed, unfiltered",
                 [update_unfiltered](const test_outcomes& d) { return update_unfiltered(d, {}); }
          },
          edge_t{empty_fails_empty_passes,
                 "Nothing executed, filtered",
                 [update_filtered, updateTime](const test_outcomes& d) { return update_filtered(d, {}, {}, updateTime); }
          },
          edge_t{house_fails_null_passes,
                 "A single failure, unfiltered",
                 [update_unfiltered](const test_outcomes& d) { return update_unfiltered(d, {{"HouseAllocationTest.cpp"}}); }
          },
          edge_t{house_fails_empty_passes,
                 "A single failure, filtered",
                 [update_filtered, updateTime](const test_outcomes& d) { return update_filtered(d, {{"HouseAllocationTest.cpp"}}, {{"HouseAllocationTest.cpp"}}, updateTime); }
          },
          edge_t{empty_fails_house_passes,
                 "A single pass, filtered",
                 [update_filtered, updateTime](const test_outcomes& d) { return update_filtered(d, {{"HouseAllocationTest.cpp"}}, {}, updateTime); }
          },
          edge_t{house_prob_fails_null_passes,
                 "Two failures, unfiltered",
                 [update_unfiltered](const test_outcomes& d) { return update_unfiltered(d, {{"HouseAllocationTest.cpp"}, {"Maths/ProbabilityTest.cpp"}}); }
          }
        }, // End   null_fails_null_passes
        {  // Begin empty_fails_null_passes
        }, // End   empty_fails_null_passes
        {  // Begin empty_fails_empty_passes
        }, // End   empty_fails_empty_passes
        {  // Begin house_fails_null_passes
          edge_t{house_prob_fails_empty_passes,
                 "An additional failure, filtered",
                 [update_filtered, updateTime](const test_outcomes& d) { return update_filtered(d, {{"Maths/ProbabilityTest.cpp"}}, {{"Maths/ProbabilityTest.cpp"}}, updateTime); }
          },
          edge_t{house_prob_maybe_fails_empty_passes,
                 "Two additional failures, filtered",
                 [update_filtered, updateTime](const test_outcomes& d) { return update_filtered(d, {{"Maths/ProbabilityTest.cpp"}, {"Maybe/MaybeTest.cpp"}}, {{"Maths/ProbabilityTest.cpp"}, {"Maybe/MaybeTest.cpp"}}, updateTime); }
          },
          edge_t{house_prob_fails_maybe_passes,
                 "One additional failure, one pass, filtered",
                 [update_filtered, updateTime](const test_outcomes& d) { return update_filtered(d, {{"Maths/ProbabilityTest.cpp"}, {"Maybe/MaybeTest.cpp"}}, {{"Maths/ProbabilityTest.cpp"}}, updateTime); }
          },
          edge_t{house_fails_maybe_prob_passes,
                 "Two additional passes, filtered",
                 [update_filtered, updateTime](const test_outcomes& d) { return update_filtered(d, {{"Maths/ProbabilityTest.cpp"}, {"Maybe/MaybeTest.cpp"}}, {}, updateTime); }
          },
          edge_t{house_fails_late_empty_passes,
                 "The same test, failing later",
                 [update_filtered, lateUpdateTime](const test_outcomes& d) {
                   return update_filtered(d, {{"HouseAllocationTest.cpp"}}, {{"HouseAllocationTest.cpp"}}, lateUpdateTime);
                 }
          },
        }, // End   house_fails_null_passes
        {  // Begin house_fails_empty_passes
        }, // End   house_fails_empty_passes
        {  // Begin empty_fails_house_passes
        }, // End   empty_fails_house_passes
        {  // Begin house_prob_fails_null_passes
          edge_t{house_fails_null_passes,
                 "One failure fewer, unfiltered",
                 [update_unfiltered](const test_outcomes& d) { return update_unfiltered(d, {{"HouseAllocationTest.cpp"}}); }
          }
        }, // End   house_prob_fails_null_passes
        {  // Begin house_prob_fails_empty_passes
          edge_t{house_fails_prob_passes,
                 "One failure fewer, filtered",
                 [update_filtered, updateTime](const test_outcomes& d) { return update_filtered(d, {{"Maths/ProbabilityTest.cpp"}}, {}, updateTime); }
          },
          edge_t{house_prob_maybe_fails_empty_passes,
                 "One more failure, filtered",
                 [update_filtered, updateTime](const test_outcomes& d) { return update_filtered(d, {{"Maybe/MaybeTest.cpp"}}, {{"Maybe/MaybeTest.cpp"}}, updateTime); }
          }
        }, // End   house_prob_fails_empty_passes
        {  // Begin house_fails_prob_passes
          edge_t{empty_fails_null_passes ,
                 "No failures, unfiltered",
                 [update_unfiltered](const test_outcomes& d) { return update_unfiltered(d, {}); }
          },
          edge_t{house_prob_fails_empty_passes,
                 "Add a failure, filtered",
                 [update_filtered, updateTime](const test_outcomes& d) { return update_filtered(d, {{"Maths/ProbabilityTest.cpp"}}, {{"Maths/ProbabilityTest.cpp"}}, updateTime); }
          }
        }, // End   house_fails_prob_passes
        {  // Begin house_prob_maybe_fails_empty_passes
          edge_t{empty_fails_null_passes,
                "Three failures all pass, unfiltered",
                [update_unfiltered](const test_outcomes& d) { return update_unfiltered(d, {}); }
          }
        }, // End   house_prob_maybe_fails_empty_passes
        {  // Begin house_prob_fails_maybe_passes
          edge_t{house_fails_maybe_prob_passes,
                 "One of two failures becomes a pass, filtered",
                 [update_filtered, updateTime](const test_outcomes& d) { return update_filtered(d, {{"Maths/ProbabilityTest.cpp"}}, {}, updateTime); }
          }
        }, // End   house_prob_fails_maybe_passes
        {  // Begin house_fails_maybe_prob_passes
          edge_t{empty_fails_maybe_house_prob_passes,
                 "Only failure becomes a pass, filtered",
                 [update_filtered, updateTime](const test_outcomes& d) { return update_filtered(d, {{"HouseAllocationTest.cpp"}}, {}, updateTime); }
          }
        }, // End   house_fails_maybe_prob_passes
        {  // Begin empty_fails_maybe_house_prob_passes
          edge_t{house_fails_maybe_prob_passes,
                 "One pass becomes a failure, filtered",
                 [update_filtered, updateTime](const test_outcomes& d) { return update_filtered(d, {{"HouseAllocationTest.cpp"}}, {{"HouseAllocationTest.cpp"}}, updateTime); }
          },
          edge_t{house_prob_fails_maybe_passes,
                 "Two passes becomes failures, filtered",
                 [update_filtered, updateTime](const test_outcomes& d) { return update_filtered(d, {{"HouseAllocationTest.cpp"}, {"Maths/ProbabilityTest.cpp"}}, {{"HouseAllocationTest.cpp"}, {"Maths/ProbabilityTest.cpp"}}, updateTime); }
          }
        }, // End   empty_fails_maybe_house_prob_passes
        {
        }
      },
      {
        test_outcomes{std::nullopt, std::nullopt},
        test_outcomes{prune_records{}, std::nullopt},
        test_outcomes{prune_records{}, prune_records{}},
        test_outcomes{{{{"HouseAllocationTest.cpp", updateTime}}}, std::nullopt},
        test_outcomes{{{{"HouseAllocationTest.cpp", updateTime}}}, prune_records{}},
        test_outcomes{prune_records{}, {{{"HouseAllocationTest.cpp", updateTime}}}},
        test_outcomes{{{{"HouseAllocationTest.cpp", updateTime}, {"Maths/ProbabilityTest.cpp", updateTime}}}, std::nullopt},
        test_outcomes{{{{"HouseAllocationTest.cpp", updateTime}, {"Maths/ProbabilityTest.cpp", updateTime}}}, prune_records{}},
        test_outcomes{{{{"HouseAllocationTest.cpp", updateTime}}}, {{{"Maths/ProbabilityTest.cpp", updateTime}}}},
        test_outcomes{{{{"HouseAllocationTest.cpp", updateTime}, {"Maybe/MaybeTest.cpp", updateTime}, {"Maths/ProbabilityTest.cpp", updateTime}}}, prune_records{}},
        test_outcomes{{{{"HouseAllocationTest.cpp", updateTime}, {"Maths/ProbabilityTest.cpp", updateTime}}}, {{{"Maybe/MaybeTest.cpp", updateTime}}}},
        test_outcomes{{{{"HouseAllocationTest.cpp", updateTime}}}, {{{"Maybe/MaybeTest.cpp", updateTime}, {"Maths/ProbabilityTest.cpp", updateTime}}}},
        test_outcomes{prune_records{}, {{{"Maybe/MaybeTest.cpp", updateTime}, {"HouseAllocationTest.cpp", updateTime}, {"Maths/ProbabilityTest.cpp", updateTime}}}},
        test_outcomes{{{{"HouseAllocationTest.cpp", lateUpdateTime}}}, prune_records{}}
      }
    };


    auto checkerFn{
        [this](std::string_view description, const test_outcomes& obtained, const test_outcomes& prediction) {
          check_data(description, obtained, prediction);
        }
    };

    transition_checker<test_outcomes>::check(report(""), g, checkerFn);
  }

  void dependency_analyzer_free_test::test_instability_analysis_prune_upate(const project_paths& projPaths)
  {
    const auto updateTime{m_ResetTime};
    const auto prune{projPaths.prune()};
    const auto failureFile{prune.failures(std::nullopt)};
    const auto passesFile{prune.selected_passes(std::nullopt)};

    fs::remove_all(prune.dir());
    fs::create_directory(prune.dir());

    using prune_graph = transition_checker<test_outcomes>::transition_graph;
    using edge_t = transition_checker<test_outcomes>::edge;

    auto update_unfiltered{
      [&](const test_outcomes& d, multi_test_list failures) -> test_outcomes {
        setup_instability_analysis_prune_folder(projPaths);

        write_or_remove(projPaths, failureFile, passesFile, d);

        for(auto i : std::views::iota(0uz, failures.size()))
        {
          update_prune_files(projPaths, std::move(failures[i]), updateTime, i);
        }

        aggregate_instability_analysis_prune_files(projPaths, prune_mode::active, updateTime, failures.size());

        return {read(failureFile), read(passesFile)};
      }
    };

    auto update_filtered{
      [&](const test_outcomes& d, test_list executed, multi_test_list failures) -> test_outcomes {

        setup_instability_analysis_prune_folder(projPaths);

        write_or_remove(projPaths, failureFile, passesFile, d);

        for(auto i : std::views::iota(0uz, failures.size()))
        {
          update_prune_files(projPaths, executed, std::move(failures[i]), updateTime, i);
        }

        aggregate_instability_analysis_prune_files(projPaths, prune_mode::passive, updateTime, failures.size());

        return {read(failureFile), read(passesFile)};
      }
    };

    const prune_graph g{
      {
        { // Begin null_fails_null_passes
          edge_t{empty_fails_null_passes,
                 "Nothing executed, unfiltered",
                 [update_unfiltered](const test_outcomes& d) { return update_unfiltered(d, {}); }
          },
          edge_t{empty_fails_empty_passes,
                 "Nothing executed, filtered",
                 [update_filtered](const test_outcomes& d) { return update_filtered(d, {}, {{}}); }
          },
          edge_t{house_fails_null_passes,
                 "A single failure in only the first of two instances, unfiltered",
                 [update_unfiltered](const test_outcomes& d) { return update_unfiltered(d, {{{"HouseAllocationTest.cpp"}}, {}}); }
          },
          edge_t{house_fails_null_passes,
                 "A single failure in only the second of two instances, unfiltered",
                 [update_unfiltered](const test_outcomes& d) { return update_unfiltered(d, {{{"HouseAllocationTest.cpp"}}, {}}); }
          },
          edge_t{house_fails_null_passes,
                 "A single failure in both instances, unfiltered",
                 [update_unfiltered](const test_outcomes& d) { return update_unfiltered(d, {{{"HouseAllocationTest.cpp"}}, {}}); }
          },
          edge_t{house_fails_empty_passes,
                 "A single failure in only the first of two instances, filtered",
                 [update_filtered](const test_outcomes& d) { return update_filtered(d, {{"HouseAllocationTest.cpp"}}, {{{"HouseAllocationTest.cpp"}}, {}}); }
          },
          edge_t{house_fails_empty_passes,
                 "A single failure in only the second of two instances, filtered",
                 [update_filtered](const test_outcomes& d) { return update_filtered(d, {{"HouseAllocationTest.cpp"}}, {{}, {{"HouseAllocationTest.cpp"}}}); }
          },
          edge_t{house_fails_empty_passes,
                 "A single failure in both instances, filtered",
                 [update_filtered](const test_outcomes& d) { return update_filtered(d, {{"HouseAllocationTest.cpp"}}, {{{"HouseAllocationTest.cpp"}}, {{"HouseAllocationTest.cpp"}}}); }
          },
          edge_t{empty_fails_house_passes,
                 "Passes in both instances, filtered",
                 [update_filtered](const test_outcomes& d) { return update_filtered(d, {{"HouseAllocationTest.cpp"}}, {{}, {}}); }
          }
        }, // End   null_fails_null_passes
        {  // Begin empty_fails_null_passes
        }, // End   empty_fails_null_passes
        {  // Begin empty_fails_empty_passes
        }, // End   empty_fails_empty_passes
        {  // Begin house_fails_null_passes
        }, // End   house_fails_null_passes
        {  // Begin house_fails_empty_passes
        }, // End   house_fails_empty_passes
        {  // Begin empty_fails_house_passes
          edge_t{house_prob_fails_null_passes,
                 "Two failures, from differing instances, unfiltered",
                 [update_unfiltered](const test_outcomes& d) { return update_unfiltered(d, {{{"HouseAllocationTest.cpp"}}, {{"Maths/ProbabilityTest.cpp"}}}); }
          },
          edge_t{house_prob_fails_empty_passes,
                 "Two failures, from three instances, filtered",
                 [update_filtered](const test_outcomes& d) {
                    return update_filtered(d,
                                             {{"HouseAllocationTest.cpp"}, {"Maths/ProbabilityTest.cpp"}},
                                             {{{"Maths/ProbabilityTest.cpp"}}, {}, {"HouseAllocationTest.cpp"}}); }
          }
        }, // End   empty_fails_house_passes
        {  // Begin house_prob_fails_null_passes
        }, // End   house_prob_fails_null_passes
        {  // Begin house_prob_fails_empty_passes
        }  // End   house_prob_fails_empty_passes
      },
      {
        test_outcomes{std::nullopt, std::nullopt},
        test_outcomes{prune_records{}, std::nullopt},
        test_outcomes{prune_records{}, prune_records{}},
        test_outcomes{{{{"HouseAllocationTest.cpp", updateTime}}}, std::nullopt},
        test_outcomes{{{{"HouseAllocationTest.cpp", updateTime}}}, prune_records{}},
        test_outcomes{prune_records{}, {{{"HouseAllocationTest.cpp", updateTime}}}},
        test_outcomes{{{{"HouseAllocationTest.cpp", updateTime}, {"Maths/ProbabilityTest.cpp", updateTime}}}, std::nullopt},
        test_outcomes{{{{"HouseAllocationTest.cpp", updateTime}, {"Maths/ProbabilityTest.cpp", updateTime}}}, prune_records{}}
      }
    };

    auto checkerFn{
        [this](std::string_view description, const test_outcomes& obtained, const test_outcomes& prediction) {
          check_data(description, obtained, prediction);
        }
    };

    transition_checker<test_outcomes>::check(report(""), g, checkerFn);
  }

  void dependency_analyzer_free_test::check_data(std::string_view description, const test_outcomes& obtained, const test_outcomes& prediction)
  {
    check(equality, std::string{description}.append(": failures"), obtained.failures, prediction.failures);
    check(equality, std::string{description}.append(": passes"), obtained.passes, prediction.passes);
  }

}
