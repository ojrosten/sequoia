////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "DependencyAnalyzerFreeTest.hpp"
#include "sequoia/TestFramework/DependencyAnalyzer.hpp"

namespace sequoia::testing
{
  namespace
  {
    class tests_to_run_checker
    {
    public:
      tests_to_run_checker(const std::filesystem::path& sourceRepo,
                           const std::filesystem::path& testRepo,
                           const std::filesystem::path& materialsRepo,
                           const std::filesystem::file_time_type resetTime)
        : m_SourceRepo{sourceRepo}
        , m_TestsRepo{testRepo}
        , m_Materials{materialsRepo}
        , m_ResetTime{resetTime}
      {}
    private:
      const std::vector<std::filesystem::path>& m_SourceRepo, m_TestsRepo, m_Materials;
      std::filesystem::file_time_type m_ResetTime;
    };
  }

  [[nodiscard]]
  std::string_view dependency_analyzer_free_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void dependency_analyzer_free_test::check_tests_to_run(std::string_view description, const information& info, const std::vector<std::filesystem::path>& makeStale, const std::vector<std::filesystem::path>& toRun)
  {
    namespace fs = std::filesystem;
    for(const auto& f : makeStale)
    {
      fs::last_write_time(f, std::chrono::file_clock::now());
    }

    test_list list{{}};
    std::transform(toRun.begin(), toRun.end(), std::back_inserter(*list), [](const std::filesystem::path& file) { return file.generic_string(); });
    std::sort(list->begin(), list->end());

    check_equality(description, tests_to_run(info.source_repo, info.tests_repo, info.materials, info.reset_time), list);

    for(const auto& f : makeStale)
    {
      fs::last_write_time(f, info.reset_time);
    }

    check_equality(append_lines(description, "Nothing Stale"), tests_to_run(info.source_repo, info.tests_repo, info.materials, info.reset_time), test_list{{}});
  }

  void dependency_analyzer_free_test::run_tests()
  {
    namespace fs = std::filesystem;
    const auto fake{working_materials() / "FakeProject"};

    information info{fake / "Source", fake / "Tests", fake / "TestMaterials", std::chrono::file_clock::now()};

    for(auto& entry : fs::recursive_directory_iterator(fake))
    {
      fs::last_write_time(entry.path(), info.reset_time);
    }

    check_equality(LINE("No timestamp"), tests_to_run(info.source_repo, info.tests_repo, info.materials, std::nullopt), test_list{});
    check_equality(LINE("Nothing stale"), tests_to_run(info.source_repo, info.tests_repo, info.materials, std::chrono::file_clock::now()), test_list{{}});

    check_tests_to_run(LINE("Test cpp stale"), info, {{info.tests_repo / "HouseAllocationTest.cpp"}}, {{"HouseAllocationTest.cpp"}});
    check_tests_to_run(LINE("Test hpp stale"), info, {{info.tests_repo / "HouseAllocationTest.cpp"}}, {{"HouseAllocationTest.cpp"}});
    check_tests_to_run(LINE("Test utils stale"),
                       info,
                       {{info.tests_repo / "Maths" / "ProbabilityTestingUtilities.hpp"}},
                       {{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}});
    check_tests_to_run(LINE("Reused utils stale"),
                       info,
                       {{info.tests_repo / "Stuff" / "OldSchoolTestingUtilities.hpp"}},
                       {{"Maybe/MaybeTest.cpp"}, {"Stuff/OldschoolTest.cpp"}, {"Stuff/OldschoolTestingDiagnostics.cpp"}});
    check_tests_to_run(LINE("Reused utils stale, relative path"),
                       info,
                       {{info.tests_repo / "Stuff" / "FooTestingUtilities.hpp"}},
                       {{"Stuff/FooTest.cpp"}, {"Stuff/FooTestingDiagnostics.cpp"}, {"Utilities/Thing/UniqueThingTest.cpp"}, {"Utilities/Thing/UniqueThingTestingDiagnostics.cpp"}});
    check_tests_to_run(LINE("Source cpp stale"),
                       info,
                       {{info.source_repo / "generatedProject" / "Maths" / "Probability.cpp"}},
                       {{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}});
    check_tests_to_run(LINE("Source hpp stale"),
                       info,
                       {{info.source_repo / "generatedProject" / "Maths" / "Probability.hpp"}},
                       {{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}});
    check_tests_to_run(LINE("Materials stale"),
                       info,
                       {{info.materials / "Stuff" / "FooTest" / "Prediction" / "RepresentativeCasesTemp" / "NoSeqpat" / "baz.txt"}},
                       {{"Stuff/FooTest.cpp"}});
   
  }
}
