////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "DependencyAnalyzerFreeTest.hpp"
#include "Parsing/CommandLineArgumentsTestingUtilities.hpp"

#include "sequoia/TestFramework/DependencyAnalyzer.hpp"
#include "sequoia/TestFramework/StateTransitionUtilities.hpp"

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
    constexpr auto updatePruneOffset{std::chrono::seconds{5}};
  }

  dependency_analyzer_free_test::data::data(opt_test_list fail, opt_test_list pass)
    : failures{std::move(fail)}
    , passes{std::move(pass)}
  {
    if(failures) std::sort(failures->begin(), failures->end());
    if(passes)   std::sort(passes->begin(), passes->end());
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

  auto dependency_analyzer_free_test::read(const fs::path& file) -> opt_test_list
  {
    if(fs::exists(file)) return read_tests(file);

    return std::nullopt;
  }

  void dependency_analyzer_free_test::write_or_remove(const project_paths& projPaths, const fs::path& file, const opt_test_list& tests)
  {
    if(tests) write_tests(projPaths, file, tests.value());
    else      fs::remove(file);
  }

  void dependency_analyzer_free_test::write_or_remove(const project_paths& projPaths, const fs::path& failureFile, const fs::path& passesFile, const data& d)
  {
    write_or_remove(projPaths, failureFile, d.failures);
    write_or_remove(projPaths, passesFile, d.passes);
  }

  [[nodiscard]]
  std::string_view dependency_analyzer_free_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void dependency_analyzer_free_test::check_tests_to_run(std::string_view description,
                                                         const project_paths& projPaths,
                                                         std::string_view cutoff,
                                                         const std::vector<updated_file>& makeStale,
                                                         std::vector<fs::path> failures,
                                                         passing_tests passes,
                                                         const std::vector<fs::path>& toRun)
  {
    std::sort(failures.begin(), failures.end());
    std::sort(passes.tests.begin(), passes.tests.end());

    const auto prune{projPaths.prune()};
    const auto failureFile{prune.failures(std::nullopt)};
    const auto passesFile{prune.selected_passes(std::nullopt)};
    write_tests(projPaths, failureFile, failures);
    write_tests(projPaths, passesFile, passes.tests);

    fs::last_write_time(passesFile, m_ResetTime + to_duration(passes.modification));

    for(const auto& f : makeStale)
    {
      fs::last_write_time(f.file, m_ResetTime + to_duration(f.modification));
    }

    opt_test_list actual{tests_to_run(projPaths, cutoff)};

    opt_test_list prediction{toRun};
    std::sort(prediction->begin(), prediction->end());

    check(equality, description, actual, prediction);

    for(const auto& f : makeStale)
    {
      fs::last_write_time(f.file, m_ResetTime);
    }

    fs::remove(failureFile);
    fs::remove(passesFile);

    check(equality, append_lines(description, "Nothing Stale"), tests_to_run(projPaths, cutoff), opt_test_list{test_list{}});
  }

  void dependency_analyzer_free_test::run_tests()
  {
    m_ResetTime = std::chrono::file_clock::now() + resetOffset;

    const auto fake{auxiliary_materials() /= "FakeProject"};
    const main_paths main{fake / main_paths::default_main_cpp_from_root()};
    commandline_arguments args{(build_paths{fake, main}.cmade_dir() / "TestAll").generic_string()};
    const project_paths projPaths{args.size(), args.get(), {main.file(), {}, main.file()}};

    check(equality, LINE("No timestamp"), tests_to_run(projPaths, ""), opt_test_list{});

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
    check_exception_thrown<std::runtime_error>(LINE("Executable out of date"), 
      [this, projPaths]() {
        fs::last_write_time(projPaths.executable(), m_ResetTime + earlyExecutableOffset);
        return tests_to_run(projPaths, "");
      });
  }

  void dependency_analyzer_free_test::test_dependencies(const project_paths& projPaths)
  {
    fs::last_write_time(projPaths.executable(), m_ResetTime + lateExecutableOffset);

    const auto& testRepo{projPaths.tests().repo()};
    const auto& sourceRepo{projPaths.source().project()};
    const auto& materials{projPaths.test_materials().repo()};

    check_tests_to_run(LINE("Nothing stale"), projPaths, "", {}, {}, {}, {});

    fs::copy(projPaths.prune().external_dependencies(), working_materials());
    check(weak_equivalence, LINE("External Dependencies"), working_materials(), predictive_materials());

    check_tests_to_run(LINE("Test cpp stale (no cutoff)"),
                       projPaths,
                       "",
                       {{{testRepo / "HouseAllocationTest.cpp"}, modification_time::early}},
                       {},
                       {},
                       {"HouseAllocationTest.cpp"});

    check_tests_to_run(LINE("Test cpp naively stale, but has passed (when selected)"),
                       projPaths,
                       "namespace",
                       {{{testRepo / "HouseAllocationTest.cpp"}, modification_time::early}},
                       {},
                       {{{"HouseAllocationTest.cpp"}}, modification_time::late},
                       {});

    check_tests_to_run(LINE("Test cpp stale; has previously passed (when selected), but this should be ignored"),
                       projPaths,
                       "namespace",
                       {{{testRepo / "HouseAllocationTest.cpp"}, modification_time::early}},
                       {},
                       {{{"HouseAllocationTest.cpp"}}, modification_time::very_early},
                       {{"HouseAllocationTest.cpp"}});

    check_tests_to_run(LINE("Test hpp stale (no cutoff)"),
                       projPaths,
                       "",
                       {{{testRepo / "HouseAllocationTest.hpp"}, modification_time::early}},
                       {},
                       {},
                       {{"HouseAllocationTest.cpp"}});

    check_tests_to_run(LINE("Test hpp stale"),
                       projPaths,
                       "namespace",
                       {{{testRepo / "HouseAllocationTest.hpp"}, modification_time::early}},
                       {},
                       {},
                       {{"HouseAllocationTest.cpp"}});

    check_tests_to_run(LINE("Test utils stale"),
                       projPaths,
                       "namespace",
                       {{{testRepo / "Maths" / "ProbabilityTestingUtilities.hpp"}, modification_time::early}},
                       {},
                       {},
                       {{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}});

    check_tests_to_run(LINE("Reused utils stale"),
                       projPaths,
                       "namespace",
                       {{{testRepo / "Stuff" / "OldschoolTestingUtilities.hpp"}, modification_time::early}},
                       {},
                       {},
                       {{"Maybe/MaybeTest.cpp"}, {"Stuff/OldschoolTest.cpp"}, {"Stuff/OldschoolTestingDiagnostics.cpp"}});

    check_tests_to_run(LINE("Reused utils stale, but one of the tests has passed"),
                       projPaths,
                       "namespace",
                       {{{testRepo / "Stuff" / "OldschoolTestingUtilities.hpp"}, modification_time::early}},
                       {},
                       {{{"Maybe/MaybeTest.cpp"}}, modification_time::late},
                       {{"Stuff/OldschoolTest.cpp"}, {"Stuff/OldschoolTestingDiagnostics.cpp"}});

    check_tests_to_run(LINE("Reused utils stale, but two of the tests have passed"),
                       projPaths,
                       "namespace",
                       {{{testRepo / "Stuff" / "OldschoolTestingUtilities.hpp"}, modification_time::early}},
                       {},
                       {{{"Maybe/MaybeTest.cpp"}, {"Stuff/OldschoolTest.cpp"}}, modification_time::late},
                       {{"Stuff/OldschoolTestingDiagnostics.cpp"}});

    check_tests_to_run(LINE("Reused utils stale, but two of the tests have passed and a different one has failed"),
                       projPaths,
                       "namespace",
                       {{{testRepo / "Stuff" / "OldschoolTestingUtilities.hpp"}, modification_time::early}},
                       {{"HouseAllocationTest.cpp"}},
                       {{{"Maybe/MaybeTest.cpp"}, {"Stuff/OldschoolTest.cpp"}}, modification_time::late},
                       {{"Stuff/OldschoolTestingDiagnostics.cpp"}, {"HouseAllocationTest.cpp"}});

    check_tests_to_run(LINE("Reused utils stale, relative path"),
                       projPaths,
                       "namespace",
                       {{{testRepo / "Stuff" / "FooTestingUtilities.hpp"}, modification_time::early}},
                       {},
                       {},
                       {{"Stuff/FooTest.cpp"}, {"Stuff/FooTestingDiagnostics.cpp"}, {"Utilities/Thing/UniqueThingTest.cpp"}, {"Utilities/Thing/UniqueThingTestingDiagnostics.cpp"}});

    check_tests_to_run(LINE("Source cpp stale"),
                       projPaths,
                       "namespace",
                       {{{sourceRepo / "Maths" / "Probability.cpp"}, modification_time::early}},
                       {},
                       {},
                       {{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}});

    check_tests_to_run(LINE("Source hpp stale"),
                       projPaths,
                       "namespace",
                       {{{sourceRepo / "Maths" / "Probability.hpp"}, modification_time::early}},
                       {},
                       {},
                       {{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}});

    check_tests_to_run(LINE("Source cpp indirectly stale via included header"),
                       projPaths,
                       "namespace",
                       {{{sourceRepo / "Maths" / "Helper.hpp"}, modification_time::early}},
                       {},
                       {},
                       {{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}});

    check_tests_to_run(LINE("Source cpp indirectly stale via cpp definitions for included header"),
                       projPaths,
                       "namespace",
                       {{{sourceRepo / "Maths" / "Helper.cpp"}, modification_time::early}},
                       {},
                       {},
                       {{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}});

    check_tests_to_run(LINE("Materials stale"),
                       projPaths,
                       "namespace",
                       {{{materials / "Stuff" / "FooTest" / "Prediction" / "RepresentativeCasesTemp" / "NoSeqpat" / "baz.txt"}, modification_time::early}},
                       {},
                       {},
                       {{"Stuff/FooTest.cpp"}});

    check_tests_to_run(LINE("Materials naively stale, but test previously passed (when selected)"),
                       projPaths,
                       "namespace",
                       {{{materials / "Stuff" / "FooTest" / "Prediction" / "RepresentativeCasesTemp" / "NoSeqpat" / "baz.txt"}, modification_time::early}},
                       {},
                       {{{"Stuff/FooTest.cpp"}}, modification_time::late},
                       {});

    check_tests_to_run(LINE("Materials stale; test previously passed (when selected), but materials subsequently modified"),
                       projPaths,
                       "namespace",
                       {{{materials / "Stuff" / "FooTest" / "Prediction" / "RepresentativeCasesTemp" / "NoSeqpat" / "baz.txt"}, modification_time::early}},
                       {},
                       {{{"Stuff/FooTest.cpp"}}, modification_time::very_early},
                       {{"Stuff/FooTest.cpp"}});

    check_tests_to_run(LINE("Materials stale; test previously passed (when selected); materials subsequently modified some early some late"),
                       projPaths,
                       "namespace",
                       {{{materials / "Stuff" / "FooTest" / "Prediction" / "RepresentativeCasesTemp" / "NoSeqpat" / "baz.txt"}, modification_time::early},
                        {{materials / "Stuff" / "FooTest" / "Prediction" / "RepresentativeCasesTemp" / "NoSeqpat" / "baz2.txt"}, modification_time::very_late}
                       },
                       {},
                       {{{"Stuff/FooTest.cpp"}}, modification_time::late},
                       {{"Stuff/FooTest.cpp"}});

    check_tests_to_run(LINE("Nothing stale, but a previous failure"),
                       projPaths,
                       "namespace",
                       {},
                       {{"Maths/ProbabilityTest.cpp"}},
                       {},
                       {{"Maths/ProbabilityTest.cpp"}});

    check_tests_to_run(LINE("Inconsistency: both passed and failed; failure wins"),
                       projPaths,
                       "namespace",
                       {},
                       {{"Maths/ProbabilityTest.cpp"}},
                       {{{"Maths/ProbabilityTest.cpp"}}, modification_time::late},
                       {{"Maths/ProbabilityTest.cpp"}});

    check_tests_to_run(LINE("Stale and a previous failure"),
                       projPaths,
                       "namespace",
                       {{{testRepo / "Maths/ProbabilityTest.cpp"}, modification_time::early}},
                       {{"Maths/ProbabilityTest.cpp"}},
                       {},
                       {{"Maths/ProbabilityTest.cpp"}});

    check_tests_to_run(LINE("Nothing stale, but two previous failures"),
                       projPaths,
                       "namespace",
                       {},
                       {{"HouseAllocationTest.cpp"}, {"Maths/ProbabilityTest.cpp"}},
                       {},
                       {{"HouseAllocationTest.cpp"}, {"Maths/ProbabilityTest.cpp"}});
  }

  void dependency_analyzer_free_test::test_prune_update(const project_paths& projPaths)
  {
    const auto updateTime{m_ResetTime + updatePruneOffset};
    const auto prune{projPaths.prune()};
    const auto failureFile{prune.failures(std::nullopt)};
    const auto passesFile{prune.selected_passes(std::nullopt)};

    using prune_graph = transition_checker<data>::transition_graph;
    using edge_t = transition_checker<data>::edge;

    auto update_with_prune{
      [&](const data& d, test_list failures) {
        write_or_remove(projPaths, failureFile, passesFile, d);

        update_prune_files(projPaths, std::move(failures), updateTime, std::nullopt);
        return data{read(failureFile), {read(passesFile)}};
      }
    };

    auto update_with_select{
      [&](const data& d, test_list executed, test_list failures) {
        write_or_remove(projPaths, failureFile, passesFile, d);

        update_prune_files(projPaths, std::move(executed), std::move(failures), std::nullopt);
        return data{read(failureFile), {read(passesFile)}};
      }
    };

    const prune_graph g{
      {
        { edge_t{2,
                 "Nothing executed, with select",
                 [update_with_select](const data& d) { return update_with_select(d, {}, {}); }
          },
          edge_t{3,
                 "A single failure, with prune",
                 [update_with_prune](const data& d) { return update_with_prune(d, {{"HouseAllocationTest.cpp"}}); }
          },
          edge_t{4,
                 "A single failure, with select",
                 [update_with_select](const data& d) { return update_with_select(d, {{"HouseAllocationTest.cpp"}}, {{"HouseAllocationTest.cpp"}}); }
          },
          edge_t{5,
                 "A single pass, with select",
                 [update_with_select](const data& d) { return update_with_select(d, {{"HouseAllocationTest.cpp"}}, {}); }
          },
          edge_t{6,
                 "Two failures, with prune",
                 [update_with_prune](const data& d) { return update_with_prune(d, {{"HouseAllocationTest.cpp"}, {"Maths/ProbabilityTest.cpp"}}); }
          }
        }, // end node 0 edges
        {},// end node 1 edges
        {},// end node 2 edges
        { edge_t{7,
                 "An additional failure, with select",
                 [update_with_select](const data& d) { return update_with_select(d, {{"Maths/ProbabilityTest.cpp"}}, {{"Maths/ProbabilityTest.cpp"}}); }
          },
          edge_t{9,
                 "Two additional failures, with select",
                 [update_with_select](const data& d) { return update_with_select(d, {{"Maths/ProbabilityTest.cpp"}, {"Maybe/MaybeTest.cpp"}}, {{"Maths/ProbabilityTest.cpp"}, {"Maybe/MaybeTest.cpp"}}); }
          },
          edge_t{10,
                 "One additional failure, one pass, with select",
                 [update_with_select](const data& d) { return update_with_select(d, {{"Maths/ProbabilityTest.cpp"}, {"Maybe/MaybeTest.cpp"}}, {{"Maths/ProbabilityTest.cpp"}}); }
          },
          edge_t{11,
                 "Two additional passes, with select",
                 [update_with_select](const data& d) { return update_with_select(d, {{"Maths/ProbabilityTest.cpp"}, {"Maybe/MaybeTest.cpp"}}, {}); }
          }
        }, // end node 3 edges
        {}, // end node 4 edges
        {}, // end node 5 edges
        {
          edge_t{3,
                 "One failure fewer, with prune",
                 [update_with_prune](const data& d) { return update_with_prune(d, {{"HouseAllocationTest.cpp"}}); }
          }
        }, // end node 6 edges
        {
          edge_t{8,
                 "One failure fewer, with select",
                 [update_with_select](const data& d) { return update_with_select(d, {{"Maths/ProbabilityTest.cpp"}}, {}); }
          },
          edge_t{9,
                 "One more failure, with select",
                 [update_with_select](const data& d) { return update_with_select(d, {{"Maybe/MaybeTest.cpp"}}, {{"Maybe/MaybeTest.cpp"}}); }
          }
        }, // end node 7 edges
        {
          edge_t{1,
                 "No failures, with prune",
                 [update_with_prune](const data& d) { return update_with_prune(d, {}); }
          },
          edge_t{7,
                 "Add a failure, with select",
                 [update_with_select](const data& d) { return update_with_select(d, {{"Maths/ProbabilityTest.cpp"}}, {{"Maths/ProbabilityTest.cpp"}}); }
          }
        }, // end node 8 edges
       {
         edge_t{1,
                "Three failures all pass, with prune",
                [update_with_prune](const data& d) { return update_with_prune(d, {}); }
         }
       }, // end node 9 edges
       {
         edge_t{11,
                "One of two failures becomes a pass, with select",
                [update_with_select](const data& d) { return update_with_select(d, {{"Maths/ProbabilityTest.cpp"}}, {}); }
         }
       }, // end node 10 edges
       {
         edge_t{12,
                "Only failure becomes a pass, with select",
                [update_with_select](const data& d) { return update_with_select(d, {{"HouseAllocationTest.cpp"}}, {}); }
         }
       },  // end node 11 edges
       {
         edge_t{11,
                "One pass becomes a failure, with select",
                [update_with_select](const data& d) { return update_with_select(d, {{"HouseAllocationTest.cpp"}}, {{"HouseAllocationTest.cpp"}}); }
         },
         edge_t{10,
                "Two passes becomes failures, with select",
                [update_with_select](const data& d) { return update_with_select(d, {{"HouseAllocationTest.cpp"}, {"Maths/ProbabilityTest.cpp"}}, {{"HouseAllocationTest.cpp"}, {"Maths/ProbabilityTest.cpp"}}); }
         }
       }, // end node 12 edges
      },
      {
        data{std::nullopt, std::nullopt}, // 0
        data{test_list{}, std::nullopt}, // 1
        data{test_list{}, test_list{}}, // 2
        data{{{{"HouseAllocationTest.cpp"}}}, std::nullopt}, // 3
        data{{{{"HouseAllocationTest.cpp"}}}, test_list{}}, // 4
        data{test_list{}, {{{"HouseAllocationTest.cpp"}}}}, // 5
        data{{{{"HouseAllocationTest.cpp"}, {"Maths/ProbabilityTest.cpp"}}}, std::nullopt}, // 6
        data{{{{"HouseAllocationTest.cpp"}, {"Maths/ProbabilityTest.cpp"}}}, test_list{}}, // 7
        data{{{{"HouseAllocationTest.cpp"}}}, {{{"Maths/ProbabilityTest.cpp"}}}}, // 8
        data{{{{"HouseAllocationTest.cpp"}, {"Maybe/MaybeTest.cpp"}, {"Maths/ProbabilityTest.cpp"}}}, test_list{}}, // 9
        data{{{{"HouseAllocationTest.cpp"}, {"Maths/ProbabilityTest.cpp"}}}, {{{"Maybe/MaybeTest.cpp"}}}}, // 10
        data{{{{"HouseAllocationTest.cpp"}}}, {{{"Maybe/MaybeTest.cpp"}, {"Maths/ProbabilityTest.cpp"}}}}, // 11
        data{test_list{}, {{{"Maybe/MaybeTest.cpp"}, {"HouseAllocationTest.cpp"}, {"Maths/ProbabilityTest.cpp"}}}} // 12
      }
    };


    auto checker{
        [this](std::string_view description, const data& obtained, const data& prediction) {
          check_data(description, obtained, prediction);
        }
    };

    transition_checker<data>::check(LINE(""), g, checker);
  }

  void dependency_analyzer_free_test::test_instability_analysis_prune_upate(const project_paths& projPaths)
  {
    const auto updateTime{m_ResetTime + updatePruneOffset};
    const auto prune{projPaths.prune()};
    const auto failureFile{prune.failures(std::nullopt)};
    const auto passesFile{prune.selected_passes(std::nullopt)};

    fs::remove_all(prune.dir());
    fs::create_directory(prune.dir());

    using prune_graph = transition_checker<data>::transition_graph;
    using edge_t = transition_checker<data>::edge;

    auto update_with_prune{
      [&](const data& d, multi_test_list failures) -> data {
        setup_instability_analysis_prune_folder(projPaths);

        write_or_remove(projPaths, failureFile, passesFile, d);

        for(std::size_t i{}; i < failures.size(); ++i)
        {
          update_prune_files(projPaths, std::move(failures[i]), updateTime, i);
        }

        aggregate_instability_analysis_prune_files(projPaths, prune_mode::active, updateTime, failures.size());

        return {read(failureFile), read(passesFile)};
      }
    };

    auto update_with_select{
      [&](const data& d, test_list executed, multi_test_list failures) -> data {

        setup_instability_analysis_prune_folder(projPaths);

        write_or_remove(projPaths, failureFile, passesFile, d);

        for(std::size_t i{}; i < failures.size(); ++i)
        {
          update_prune_files(projPaths, executed, std::move(failures[i]), i);
        }

        aggregate_instability_analysis_prune_files(projPaths, prune_mode::passive, updateTime, failures.size());

        return {read(failureFile), read(passesFile)};
      }
    };

    const prune_graph g{
      {
        {
          edge_t{2,
                 "Nothing executed, with select",
                 [update_with_select](const data& d) { return update_with_select(d, {{}}, {{}}); }
          },
          edge_t{3,
                 "A single failure in only the first of two instances, with prune",
                 [update_with_prune](const data& d) { return update_with_prune(d, {{{"HouseAllocationTest.cpp"}}, {}}); }
          },
          edge_t{3,
                 "A single failure in only the second of two instances, with prune",
                 [update_with_prune](const data& d) { return update_with_prune(d, {{{"HouseAllocationTest.cpp"}}, {}}); }
          },
          edge_t{3,
                 "A single failure in both instances, with prune",
                 [update_with_prune](const data& d) { return update_with_prune(d, {{{"HouseAllocationTest.cpp"}}, {}}); }
          },
          edge_t{4,
                 "A single failure in only the first of two instances, with select",
                 [update_with_select](const data& d) { return update_with_select(d, {{"HouseAllocationTest.cpp"}}, {{{"HouseAllocationTest.cpp"}}, {}}); }
          },
          edge_t{4,
                 "A single failure in only the second of two instances, with select",
                 [update_with_select](const data& d) { return update_with_select(d, {{"HouseAllocationTest.cpp"}}, {{}, {{"HouseAllocationTest.cpp"}}}); }
          },
          edge_t{4,
                 "A single failure in both instances, with select",
                 [update_with_select](const data& d) { return update_with_select(d, {{"HouseAllocationTest.cpp"}}, {{{"HouseAllocationTest.cpp"}}, {{"HouseAllocationTest.cpp"}}}); }
          },
          edge_t{5,
                 "Passes in both instances, with select",
                 [update_with_select](const data& d) { return update_with_select(d, {{"HouseAllocationTest.cpp"}}, {{}, {}}); }
          }
        }, // 0
        {}, // 1
        {}, // 2
        {}, // 3
        {}, // 4
        {
          edge_t{6,
                 "Two failures, from differing instances, with prune",
                 [update_with_prune](const data& d) { return update_with_prune(d, {{{"HouseAllocationTest.cpp"}}, {{"Maths/ProbabilityTest.cpp"}}}); }
          },
          edge_t{7,
                 "Two failures, from three instances, with select",
                 [update_with_select](const data& d) {
                    return update_with_select(d,
                                             {{"HouseAllocationTest.cpp"}, {"Maths/ProbabilityTest.cpp"}},
                                             {{{"Maths/ProbabilityTest.cpp"}}, {}, {"HouseAllocationTest.cpp"}}); }
          }
        }, // 5
        {}, // 6
        {} // 7
      },
      {
        data{std::nullopt, std::nullopt}, //0
        data{test_list{}, std::nullopt}, // 1
        data{test_list{}, test_list{}}, // 2
        data{{{{"HouseAllocationTest.cpp"}}}, std::nullopt}, // 3
        data{{{{"HouseAllocationTest.cpp"}}}, test_list{}}, // 4
        data{test_list{}, {{{"HouseAllocationTest.cpp"}}}}, // 5
        data{{{{"HouseAllocationTest.cpp"}, {"Maths/ProbabilityTest.cpp"}}}, std::nullopt}, // 6
        data{{{{"HouseAllocationTest.cpp"}, {"Maths/ProbabilityTest.cpp"}}}, test_list{}} // 7
      }
    };

    auto checker{
        [this](std::string_view description, const data& obtained, const data& prediction) {
          check_data(description, obtained, prediction);
        }
    };

    transition_checker<data>::check(LINE(""), g, checker);
  }

  void dependency_analyzer_free_test::check_data(std::string_view description, const data& obtained, const data& prediction)
  {
    check(equality, std::string{description}.append(": failures"), obtained.failures, prediction.failures);
    check(equality, std::string{description}.append(": passes"), obtained.passes, prediction.passes);
  }

}
