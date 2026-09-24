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
    [[nodiscard]]
    std::string no_materials_paths(std::string_view testName)
    {
      return std::format("Test '{}' has no materials paths: it was not constructed with any", testName);
    }

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
    return materials_or_throw("materials", m_Materials.original_materials_root(), m_Materials.working());
  }

  [[nodiscard]]
  fs::path test_base::predictive_materials() const
  {
    return materials_or_throw("predictions", m_Materials.prediction(), m_Materials.prediction());
  }

  [[nodiscard]]
  fs::path test_base::auxiliary_materials() const
  {
    return materials_or_throw("auxiliary materials", m_Materials.original_auxiliary(), m_Materials.auxiliary());
  }

  [[nodiscard]]
  fs::path test_base::scratchpad_materials() const
  {
    if(m_Materials.temporary_materials_root().empty())
      throw std::logic_error{no_materials_paths(m_Name)};

    return m_Materials.temporary_materials_root();
  }

  /// `usable` if the materials `committed` names are committed; which kind they are is for the message
  [[nodiscard]]
  fs::path test_base::materials_or_throw(std::string_view kind, const fs::path& committed, fs::path usable) const
  {
    if(committed.empty())
      throw std::logic_error{no_materials_paths(m_Name)};

    if(!fs::exists(committed))
      throw std::runtime_error{
        std::format("No {} for test '{}'; they would be committed in {}", kind, m_Name, committed.generic_string())
      };

    return usable;
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
