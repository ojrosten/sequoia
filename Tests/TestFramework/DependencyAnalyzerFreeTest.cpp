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
  namespace fs = std::filesystem;

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

    test_list actual{tests_to_run(info.source_repo, info.tests_repo, info.materials, info.reset_time, info.exe_time_stamp)};
    if(actual.has_value())
      std::sort(actual->begin(), actual->end());

    test_list prediction{toRun};
    std::sort(prediction->begin(), prediction->end());

    check_equality(description, actual, prediction);

    for(const auto& f : makeStale)
    {
      fs::last_write_time(f, info.reset_time);
    }

    check_equality(append_lines(description, "Nothing Stale"), tests_to_run(info.source_repo, info.tests_repo, info.materials, info.reset_time, info.exe_time_stamp), test_list{{}});
  }

  void dependency_analyzer_free_test::run_tests()
  {
    const auto fake{working_materials() / "FakeProject"};

    m_Info = {fake / "Source", fake / "Tests", fake / "TestMaterials", std::chrono::file_clock::now()};

    for(auto& entry : fs::recursive_directory_iterator(fake))
    {
      fs::last_write_time(entry.path(), m_Info.reset_time);
    }

    test_exceptions();
    test_dependencies();
  }

  void dependency_analyzer_free_test::test_exceptions()
  {
    check_exception_thrown<std::runtime_error>(LINE("Executable out of date"), 
      [this]() {
        const auto now{std::chrono::file_clock::now()};
        return tests_to_run(m_Info.source_repo, m_Info.tests_repo, m_Info.materials, now, m_Info.reset_time);
      });
  }
#
  void dependency_analyzer_free_test::test_dependencies()
  {
   

    check_equality(LINE("No timestamp"), tests_to_run(m_Info.source_repo, m_Info.tests_repo, m_Info.materials, std::nullopt, std::nullopt), test_list{});
    check_equality(LINE("Nothing stale"), tests_to_run(m_Info.source_repo, m_Info.tests_repo, m_Info.materials, std::chrono::file_clock::now(), std::nullopt), test_list{{}});

    check_tests_to_run(LINE("Test cpp stale"), m_Info, {{m_Info.tests_repo / "HouseAllocationTest.cpp"}}, {{"HouseAllocationTest.cpp"}});
    check_tests_to_run(LINE("Test hpp stale"), m_Info, {{m_Info.tests_repo / "HouseAllocationTest.cpp"}}, {{"HouseAllocationTest.cpp"}});
    check_tests_to_run(LINE("Test utils stale"),
                       m_Info,
                       {{m_Info.tests_repo / "Maths" / "ProbabilityTestingUtilities.hpp"}},
                       {{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}});
    check_tests_to_run(LINE("Reused utils stale"),
                       m_Info,
                       {{m_Info.tests_repo / "Stuff" / "OldSchoolTestingUtilities.hpp"}},
                       {{"Maybe/MaybeTest.cpp"}, {"Stuff/OldschoolTest.cpp"}, {"Stuff/OldschoolTestingDiagnostics.cpp"}});
    check_tests_to_run(LINE("Reused utils stale, relative path"),
                       m_Info,
                       {{m_Info.tests_repo / "Stuff" / "FooTestingUtilities.hpp"}},
                       {{"Stuff/FooTest.cpp"}, {"Stuff/FooTestingDiagnostics.cpp"}, {"Utilities/Thing/UniqueThingTest.cpp"}, {"Utilities/Thing/UniqueThingTestingDiagnostics.cpp"}});
    check_tests_to_run(LINE("Source cpp stale"),
                       m_Info,
                       {{m_Info.source_repo / "generatedProject" / "Maths" / "Probability.cpp"}},
                       {{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}});
    check_tests_to_run(LINE("Source hpp stale"),
                       m_Info,
                       {{m_Info.source_repo / "generatedProject" / "Maths" / "Probability.hpp"}},
                       {{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}});
    check_tests_to_run(LINE("Source cpp indirectly stale via included header"),
                       m_Info,
                       {{m_Info.source_repo / "generatedProject" / "Maths" / "Helper.hpp"}},
                       {{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}});
    check_tests_to_run(LINE("Source cpp indirectly stale via cpp definitions for included header"),
                       m_Info,
                       {{m_Info.source_repo / "generatedProject" / "Maths" / "Helper.cpp"}},
                       {{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}});
    check_tests_to_run(LINE("Materials stale"),
                       m_Info,
                       {{m_Info.materials / "Stuff" / "FooTest" / "Prediction" / "RepresentativeCasesTemp" / "NoSeqpat" / "baz.txt"}},
                       {{"Stuff/FooTest.cpp"}});
   
  }
}
