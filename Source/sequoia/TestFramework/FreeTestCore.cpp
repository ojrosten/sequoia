////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/TestFramework/FreeTestCore.hpp"
#include "sequoia/TestFramework/FileEditors.hpp"

namespace sequoia::testing
{
  void test::set_filesystem_data(std::filesystem::path testRepo, const std::filesystem::path& outputDir, std::string_view familyName)
  {
    m_TestRepo = std::move(testRepo);
    m_DiagnosticsOutput = make_output_filepath(outputDir, familyName, "Output");
    m_CaughtExceptionsOutput = make_output_filepath(outputDir, familyName, "Exceptions");

    namespace fs = std::filesystem;
    fs::create_directories(m_DiagnosticsOutput.parent_path());
  }

  void test::set_materials(std::filesystem::path workingMaterials, std::filesystem::path predictiveMaterials, std::filesystem::path auxiliaryMaterials)
  {
    m_WorkingMaterials    = std::move(workingMaterials);
    m_PredictiveMaterials = std::move(predictiveMaterials);
    m_AuxiliaryMaterials  = std::move(auxiliaryMaterials);
  }

  void test::set_recovery_paths(recovery_paths paths)
  {
    do_set_recovery_paths(std::move(paths));
  }

  [[nodiscard]]
  std::string test::report_line(const std::filesystem::path& file, int line, std::string_view message)
  {
    return testing::report_line(file, line, message, test_repository());
  }

  [[nodiscard]]
  log_summary test::execute()
  {
    using namespace std::chrono;
    const auto start{steady_clock::now()};

    try
    {
      run_tests();
    }
    catch(const std::exception& e)
    {
      log_critical_failure("Unexpected", e.what());
    }
    catch(...)
    {
      log_critical_failure("Unknown", "");
    }

    const auto end{steady_clock::now()};
    return write_versioned_output(summarize(end - start));
  };

  [[nodiscard]]
  std::filesystem::path test::make_output_filepath(const std::filesystem::path& outputDir, std::string_view familyName, std::string_view suffix) const
  {
    namespace fs = std::filesystem;

    auto makeDirName{
      [](std::string_view name) -> fs::path {
        std::string n{name};
        for(auto& c : n) if(c == ' ') c = '_';

        return n;
      }
    };

    return diagnostics_output_path(outputDir) / makeDirName(familyName) / output_filename(suffix);
  }

  const log_summary& test::write_versioned_output(const log_summary& summary) const
  {
    write(diagnostics_output_filename(), summary.diagnostics_output());
    write(caught_exceptions_output_filename(), summary.caught_exceptions_output());

    return summary;
  }

  void test::write(const std::filesystem::path& file, std::string_view text)
  {
    if(!text.empty() || std::filesystem::exists(file))
    {
      write_to_file(file, text);
    }
  }

  [[nodiscard]]
  std::filesystem::path test::output_filename(std::string_view suffix) const
  {
    namespace fs = std::filesystem;
    return fs::path{source_file()}.filename().replace_extension().concat("_").concat(suffix).concat(".txt");
  }

  [[nodiscard]]
  std::string to_tag(test_mode mode)
  {
    switch(mode)
    {
    case test_mode::false_negative:
      return "FN";
    case test_mode::false_positive:
      return "FP";
    case test_mode::standard:
      return "";
    }

    throw std::logic_error{"Unrecognized case for test_mode"};
  }
}
