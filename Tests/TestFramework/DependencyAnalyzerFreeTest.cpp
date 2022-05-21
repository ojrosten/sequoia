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

#include <fstream>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    void write_tests(const fs::path& file, std::vector<fs::path>& tests)
    {
      std::sort(tests.begin(), tests.end());
      if(std::ofstream ofile{file})
      {
        for(const auto& f : tests)
          ofile << f.generic_string() << "\n";
      }
      else
      {
        throw std::runtime_error{file.generic_string().append(" not found")};
      }
    }
  }

  using test_list     = std::vector<fs::path>;
  using opt_test_list = std::optional<test_list>;

  [[nodiscard]]
  std::string_view dependency_analyzer_free_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void dependency_analyzer_free_test::check_tests_to_run(std::string_view description,
                                                         const project_paths& projPaths,
                                                         std::string_view cutoff,
                                                         const std::vector<std::filesystem::path>& makeStale,
                                                         std::vector<std::filesystem::path> failures,
                                                         std::vector<std::filesystem::path> passes,
                                                         const std::vector<std::filesystem::path>& toRun)
  {
    namespace fs = std::filesystem;

    std::sort(failures.begin(), failures.end());

    const auto prune{projPaths.prune()};
    const auto failureFile{prune.failures(std::nullopt)};
    const auto passesFile{prune.selected_passes(std::nullopt)};
    write_tests(failureFile, failures);
    write_tests(passesFile, passes);

    const auto staleTime{m_ResetTime + std::chrono::seconds{1}};
    for(const auto& f : makeStale)
    {
      fs::last_write_time(f, staleTime);
    }

    opt_test_list actual{tests_to_run(projPaths, cutoff)};

    opt_test_list prediction{toRun};
    std::sort(prediction->begin(), prediction->end());

    check(equality, description, actual, prediction);

    for(const auto& f : makeStale)
    {
      fs::last_write_time(f, m_ResetTime);
    }

    fs::remove(failureFile);
    fs::remove(passesFile);

    check(equality, append_lines(description, "Nothing Stale"), tests_to_run(projPaths, cutoff), opt_test_list{test_list{}});
  }

  void dependency_analyzer_free_test::run_tests()
  {
    m_ResetTime = std::chrono::file_clock::now() - std::chrono::seconds(1);

    const auto fake{working_materials() / "FakeProject"};
    const auto mainDir{fake / "TestAll"};
    commandline_arguments args{(project_paths::cmade_build_dir(fake, mainDir) /= "TestAll").generic_string()};
    const project_paths projPaths{args.size(), args.get(), {{"TestAll/TestAllMain.cpp"}, {}, {"TestAll/TestAllMain.cpp"}}};

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
  }

  void dependency_analyzer_free_test::test_exceptions(const project_paths& projPaths)
  {
    check_exception_thrown<std::runtime_error>(LINE("Executable out of date"), 
      [this, projPaths]() {
        fs::last_write_time(projPaths.executable(), m_ResetTime - std::chrono::seconds{3});
        return tests_to_run(projPaths, "");
      });
  }
#
  void dependency_analyzer_free_test::test_dependencies(const project_paths& projPaths)
  {
    fs::last_write_time(projPaths.executable(), m_ResetTime + std::chrono::seconds{3});

    const auto& testRepo{projPaths.tests()};
    const auto& sourceRepo{projPaths.source()};
    const auto& materials{projPaths.test_materials()};

    check_tests_to_run(LINE("Nothing stale"), projPaths, "", {}, {}, {}, {});

    check_tests_to_run(LINE("Test cpp stale (no cutoff)"),
                       projPaths,
                       "",
                       {{testRepo / "HouseAllocationTest.cpp"}},
                       {},
                       {},
                       {"HouseAllocationTest.cpp"});

    check_tests_to_run(LINE("Test cpp older than stamp, but has passed (when selected)"),
                       projPaths,
                       "namespace",
                       {{testRepo / "HouseAllocationTest.cpp"}},
                       {},
                       {{"HouseAllocationTest.cpp"}},
                       {});

    check_tests_to_run(LINE("Test hpp stale (no cutoff)"),
                       projPaths,
                       "",
                       {{testRepo / "HouseAllocationTest.hpp"}},
                       {},
                       {},
                       {{"HouseAllocationTest.cpp"}});

    check_tests_to_run(LINE("Test hpp stale"),
                       projPaths,
                       "namespace",
                       {{testRepo / "HouseAllocationTest.hpp"}},
                       {},
                       {},
                       {{"HouseAllocationTest.cpp"}});

    check_tests_to_run(LINE("Test utils stale"),
                       projPaths,
                       "namespace",
                       {{testRepo / "Maths" / "ProbabilityTestingUtilities.hpp"}},
                       {},
                       {},
                       {{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}});

    check_tests_to_run(LINE("Reused utils stale"),
                       projPaths,
                       "namespace",
                       {{testRepo / "Stuff" / "OldSchoolTestingUtilities.hpp"}},
                       {},
                       {},
                       {{"Maybe/MaybeTest.cpp"}, {"Stuff/OldschoolTest.cpp"}, {"Stuff/OldschoolTestingDiagnostics.cpp"}});

    check_tests_to_run(LINE("Reused utils stale, but one of the tests has passed"),
                       projPaths,
                       "namespace",
                       {{testRepo / "Stuff" / "OldSchoolTestingUtilities.hpp"}},
                       {},
                       {{"Maybe/MaybeTest.cpp"}},
                       {{"Stuff/OldschoolTest.cpp"}, {"Stuff/OldschoolTestingDiagnostics.cpp"}});

    check_tests_to_run(LINE("Reused utils stale, but two of the tests have passed"),
                       projPaths,
                       "namespace",
                       {{testRepo / "Stuff" / "OldSchoolTestingUtilities.hpp"}},
                       {},
                       {{"Maybe/MaybeTest.cpp"}, {"Stuff/OldschoolTest.cpp"}},
                       {{"Stuff/OldschoolTestingDiagnostics.cpp"}});

    check_tests_to_run(LINE("Reused utils stale, but two of the tests have passed and a different one has failed"),
                       projPaths,
                       "namespace",
                       {{testRepo / "Stuff" / "OldSchoolTestingUtilities.hpp"}},
                       {{"HouseAllocationTest.cpp"}},
                       {{"Maybe/MaybeTest.cpp"}, {"Stuff/OldschoolTest.cpp"}},
                       {{"Stuff/OldschoolTestingDiagnostics.cpp"}, {"HouseAllocationTest.cpp"}});

    check_tests_to_run(LINE("Reused utils stale, relative path"),
                       projPaths,
                       "namespace",
                       {{testRepo / "Stuff" / "FooTestingUtilities.hpp"}},
                       {},
                       {},
                       {{"Stuff/FooTest.cpp"}, {"Stuff/FooTestingDiagnostics.cpp"}, {"Utilities/Thing/UniqueThingTest.cpp"}, {"Utilities/Thing/UniqueThingTestingDiagnostics.cpp"}});

    check_tests_to_run(LINE("Source cpp stale"),
                       projPaths,
                       "namespace",
                       {{sourceRepo / "Maths" / "Probability.cpp"}},
                       {},
                       {},
                       {{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}});

    check_tests_to_run(LINE("Source hpp stale"),
                       projPaths,
                       "namespace",
                       {{sourceRepo / "Maths" / "Probability.hpp"}},
                       {},
                       {},
                       {{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}});

    check_tests_to_run(LINE("Source cpp indirectly stale via included header"),
                       projPaths,
                       "namespace",
                       {{sourceRepo / "Maths" / "Helper.hpp"}},
                       {},
                       {},
                       {{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}});

    check_tests_to_run(LINE("Source cpp indirectly stale via cpp definitions for included header"),
                       projPaths,
                       "namespace",
                       {{sourceRepo / "Maths" / "Helper.cpp"}},
                       {},
                       {},
                       {{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}});

    check_tests_to_run(LINE("Materials stale"),
                       projPaths,
                       "namespace",
                       {{materials / "Stuff" / "FooTest" / "Prediction" / "RepresentativeCasesTemp" / "NoSeqpat" / "baz.txt"}},
                       {},
                       {},
                       {{"Stuff/FooTest.cpp"}});

    check_tests_to_run(LINE("Nothing stale, but a previous failure"),
                       projPaths,
                       "namespace",
                       {},
                       {{"Maths/ProbabilityTest.cpp"}},
                       {},
                       {{"Maths/ProbabilityTest.cpp"}});

    check_tests_to_run(LINE("Both stale and a previous failure"),
                       projPaths,
                       "namespace",
                       {{testRepo / "Maths/ProbabilityTest.cpp"}},
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

}
