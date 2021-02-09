////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "FreeTestCore.hpp"

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

  void test::set_materials(std::filesystem::path workingMaterials, std::filesystem::path predictiveMaterials)
  {
    m_WorkingMaterials = std::move(workingMaterials);
    m_PredictiveMaterials = std::move(predictiveMaterials);
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
    const auto time{steady_clock::now()};
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

    return write_versioned_output(summarize(steady_clock::now() - time));
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

  void test::write(const std::filesystem::path& file, std::string_view text)
  {
    if(std::ofstream ofile{file})
    {
      ofile.write(text.data(), text.size());
    }
    else
    {
      throw std::runtime_error{report_failed_write(file)};
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
