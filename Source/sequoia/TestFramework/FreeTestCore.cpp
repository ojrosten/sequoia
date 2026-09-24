////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/TestFramework/FreeTestCore.hpp"
#include "sequoia/TestFramework/FileEditors.hpp"
#include "sequoia/Streaming/Streaming.hpp"

#include <format>
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

  [[nodiscard]]
  fs::path test_base::working_materials() const
  {
    if(const auto working{m_Materials.working()}; fs::exists(working))
      return working;

    throw std::runtime_error{
      std::format("No materials for test '{}'; they would be committed in {}",
                  m_Name,
                  m_Materials.original_materials_root().generic_string())
    };
  }

  [[nodiscard]]
  fs::path test_base::predictive_materials() const
  {
    if(const auto prediction{m_Materials.prediction()}; fs::exists(prediction))
      return prediction;

    throw std::runtime_error{
      std::format("No predictions for test '{}'; they would be committed in {}",
                  m_Name,
                  m_Materials.prediction().generic_string())
    };
  }

  [[nodiscard]]
  fs::path test_base::auxiliary_materials() const
  {
    if(const auto auxiliary{m_Materials.auxiliary()}; fs::exists(auxiliary))
      return auxiliary;

    throw std::runtime_error{
      std::format("No auxiliary materials for test '{}'; they would be committed in {}",
                  m_Name,
                  m_Materials.original_auxiliary().generic_string())
    };
  }

  [[nodiscard]]
  fs::path test_base::scratchpad_materials() const
  {
    if(const auto scratchpad{m_Materials.scratchpad()}; fs::exists(scratchpad))
      return scratchpad;

    throw std::logic_error{std::format("No scratchpad for test '{}': its materials have not been staged", m_Name)};
  }

  void test_base::write_instability_analysis_output(const normal_path& srcFile, std::optional<std::size_t> index, const failure_output& output) const
  {
    if(index.has_value())
    {
      const auto file{output_paths::instability_analysis_file(get_project_paths().project_root(), srcFile, name(), index.value())};
      serialize(file, output);
    }
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
