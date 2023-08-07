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

  void test_base::initialize(test_mode mode, std::string_view suiteName, const normal_path& srcFile, const project_paths& projPaths, individual_materials_paths materials)
  {
    m_ProjectPaths = projPaths;
    m_Materials   = std::move(materials);
    m_Diagnostics = {project_root(), suiteName, srcFile, to_tag(mode)};
    std::filesystem::create_directories(m_Diagnostics.diagnostics_file().parent_path());
  }

  void test_base::write_instability_analysis_output(const normal_path& srcFile, std::optional<std::size_t> index, const failure_output& output) const
  {
    if(index.has_value())
    {
      const auto file{output_paths::instability_analysis_file(project_root(), srcFile, name(), index.value())};
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
