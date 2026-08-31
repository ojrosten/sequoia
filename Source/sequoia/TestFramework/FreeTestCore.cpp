////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

module;

#include "sequoia/PlatformSpecific/Macros.hpp"
#include "sequoia/TestFramework/Macros.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <concepts>
#include <execution>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <scoped_allocator>
#include <source_location>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

module sequoia.test_framework;

import sequoia.streaming;

/** \file
    \brief Definitions for FreeTestCore.hpp
*/



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
