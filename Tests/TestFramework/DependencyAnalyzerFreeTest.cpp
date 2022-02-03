////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "DependencyAnalyzerFreeTest.hpp"
#include "sequoia/TestFramework/DependencyAnalyzer.hpp"

#include <fstream>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  [[nodiscard]]
  std::string_view dependency_analyzer_free_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void dependency_analyzer_free_test::check_tests_to_run(std::string_view description, const information& info, std::string_view cutoff, const std::vector<std::filesystem::path>& makeStale, const std::vector<std::filesystem::path>& toRun)
  {
    namespace fs = std::filesystem;
    for(const auto& f : makeStale)
    {
      fs::last_write_time(f, std::chrono::file_clock::now());
    }

    test_list actual{tests_to_run(info.source_repo, info.tests_repo, info.materials, info.prune_file, info.reset_time, info.exe_time_stamp, cutoff)};
    if(actual.has_value())
      std::sort(actual->begin(), actual->end());

    test_list prediction{toRun};
    std::sort(prediction->begin(), prediction->end());

    check(equality, description, actual, prediction);

    for(const auto& f : makeStale)
    {
      fs::last_write_time(f, info.reset_time);
    }

    check(equality, append_lines(description, "Nothing Stale"), tests_to_run(info.source_repo, info.tests_repo, info.materials, "", info.reset_time, info.exe_time_stamp, cutoff), test_list{{}});
  }

  void dependency_analyzer_free_test::run_tests()
  {
    const auto fake{working_materials() / "FakeProject"};

    m_Info = {fake / "Source", fake / "Tests", fake / "TestMaterials", fake / "output/TestAll.prune", std::chrono::file_clock::now()};

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
        return tests_to_run(m_Info.source_repo, m_Info.tests_repo, m_Info.materials, "", now, m_Info.reset_time, "");
      });
  }
#
  void dependency_analyzer_free_test::test_dependencies()
  {
    check(equality, LINE("No timestamp"), tests_to_run(m_Info.source_repo, m_Info.tests_repo, m_Info.materials, "", std::nullopt, std::nullopt, ""), test_list{});
    check(equality, LINE("Nothing stale"), tests_to_run(m_Info.source_repo, m_Info.tests_repo, m_Info.materials, "", std::chrono::file_clock::now(), std::nullopt, ""), test_list{{}});

    check_tests_to_run(LINE("Test cpp stale (no cutoff)"),
                       m_Info,
                       "",
                       {{m_Info.tests_repo / "HouseAllocationTest.cpp"}},
                       {{"HouseAllocationTest.cpp"}});

    check_tests_to_run(LINE("Test cpp stale"),
                       m_Info,
                       "namespace",
                       {{m_Info.tests_repo / "HouseAllocationTest.cpp"}},
                       {{"HouseAllocationTest.cpp"}});

    check_tests_to_run(LINE("Test hpp stale (no cutoff)"),
                       m_Info,
                       "",
                       {{m_Info.tests_repo / "HouseAllocationTest.hpp"}},
                       {{"HouseAllocationTest.cpp"}});

    check_tests_to_run(LINE("Test hpp stale"),
                       m_Info,
                       "namespace",
                       {{m_Info.tests_repo / "HouseAllocationTest.hpp"}},
                       {{"HouseAllocationTest.cpp"}});

    check_tests_to_run(LINE("Test utils stale"),
                       m_Info,
                       "namespace",
                       {{m_Info.tests_repo / "Maths" / "ProbabilityTestingUtilities.hpp"}},
                       {{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}});

    check_tests_to_run(LINE("Reused utils stale"),
                       m_Info,
                       "namespace",
                       {{m_Info.tests_repo / "Stuff" / "OldSchoolTestingUtilities.hpp"}},
                       {{"Maybe/MaybeTest.cpp"}, {"Stuff/OldschoolTest.cpp"}, {"Stuff/OldschoolTestingDiagnostics.cpp"}});

    check_tests_to_run(LINE("Reused utils stale, relative path"),
                       m_Info,
                       "namespace",
                       {{m_Info.tests_repo / "Stuff" / "FooTestingUtilities.hpp"}},
                       {{"Stuff/FooTest.cpp"}, {"Stuff/FooTestingDiagnostics.cpp"}, {"Utilities/Thing/UniqueThingTest.cpp"}, {"Utilities/Thing/UniqueThingTestingDiagnostics.cpp"}});

    check_tests_to_run(LINE("Source cpp stale"),
                       m_Info,
                      "namespace",
                       {{m_Info.source_repo / "generatedProject" / "Maths" / "Probability.cpp"}},
                       {{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}});

    check_tests_to_run(LINE("Source hpp stale"),
                       m_Info,
                      "namespace",
                       {{m_Info.source_repo / "generatedProject" / "Maths" / "Probability.hpp"}},
                       {{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}});

    check_tests_to_run(LINE("Source cpp indirectly stale via included header"),
                       m_Info,
                      "namespace",
                       {{m_Info.source_repo / "generatedProject" / "Maths" / "Helper.hpp"}},
                       {{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}});

    check_tests_to_run(LINE("Source cpp indirectly stale via cpp definitions for included header"),
                       m_Info,
                       "namespace",
                       {{m_Info.source_repo / "generatedProject" / "Maths" / "Helper.cpp"}},
                       {{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}});

    check_tests_to_run(LINE("Materials stale"),
                       m_Info,
                       "namespace",
                       {{m_Info.materials / "Stuff" / "FooTest" / "Prediction" / "RepresentativeCasesTemp" / "NoSeqpat" / "baz.txt"}},
                       {{"Stuff/FooTest.cpp"}});

    // Now populate the .prune file
    fs::create_directory(m_Info.prune_file.parent_path());
    if(std::ofstream ofile{m_Info.prune_file})
    {
      ofile << m_Info.tests_repo / "Maths" / "ProbabilityTest.cpp" << "\n";
    }
    else
    {
      throw std::runtime_error{m_Info.prune_file.generic_string().append(" not found")};
    }

    check_tests_to_run(LINE("Nothing stale, but a previous failure"),
                       m_Info,
                       "namespace",
                       {},
                       {{"Maths/ProbabilityTest.cpp"}});

    check_tests_to_run(LINE("Both stale and a previous failure"),
                       m_Info,
                       "namespace",
                       {{m_Info.tests_repo / "Maths" / "ProbabilityTest.cpp"}},
                       {{"Maths/ProbabilityTest.cpp"}});

    // Add another entry to the .prune file
    if(std::ofstream ofile{m_Info.prune_file, std::ios_base::app})
    {
      ofile << m_Info.tests_repo / "HouseAllocationTest.cpp" << "\n";
    }
    else
    {
      throw std::runtime_error{m_Info.prune_file.generic_string().append(" not found")};
    }

    check_tests_to_run(LINE("Nothing stale, but two previous failures"),
                       m_Info,
                       "namespace",
                       {},
                       {{"HouseAllocationTest.cpp"}, {"Maths/ProbabilityTest.cpp"}});
  }
}
