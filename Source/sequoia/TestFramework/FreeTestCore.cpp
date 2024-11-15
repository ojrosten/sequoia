////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for FreeTestCore.hpp
*/

#include "sequoia/TestFramework/FreeTestCore.hpp"
#include "sequoia/TestFramework/FileEditors.hpp"

#include <fstream>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    void serialize(const fs::path& file, const failure_output& output)
    {
      fs::create_directories(file.parent_path());
      if(std::ofstream ofile{file})
      {
        ofile << output;
      }
      else
      {
        throw std::runtime_error{report_failed_write(file)};
      }
    }
  }

  void test_base::write_instability_analysis_output(const normal_path& srcFile, std::optional<std::size_t> index, const failure_output& output) const
  {
    if(index.has_value())
    {
      const auto file{output_paths::instability_analysis_file(project_root(), srcFile, name(), index.value())};
      serialize(file, output);
    }
  }
  
  [[nodiscard]]
  fs::path test_base::test_summary_filename(const fs::path& sourceFile, const std::optional<std::string>& discriminator) const
  {
    const auto name{
        [&]() {
          auto summaryFile{fs::path{sourceFile}.replace_extension(".txt")};
          if(discriminator && !discriminator->empty())
            summaryFile.replace_filename(summaryFile.stem().concat("_" + discriminator.value()).concat(summaryFile.extension().string()));

          return summaryFile;
        }()
    };

    if(name.empty())
      throw std::logic_error("Source files should have a non-trivial name!");

    if(!name.is_absolute())
    {
      if(const auto testRepo{m_ProjectPaths.tests().repo()}; !testRepo.empty())
      {
        return m_ProjectPaths.output().test_summaries() / back(testRepo) / rebase_from(name, testRepo);
      }
    }
    else
    {
      auto summaryFile{m_ProjectPaths.output().test_summaries()};
      auto iters{std::ranges::mismatch(name, summaryFile)};

      while(iters.in1 != name.end())
        summaryFile /= *iters.in1++;

      return summaryFile;
    }

    return name;
  }

  timer::timer()
    : m_Start{std::chrono::steady_clock::now()}
  {}

  [[nodiscard]]
  std::chrono::nanoseconds timer::time_elapsed() const
  {
    return std::chrono::steady_clock::now() - m_Start;
  }
}
