////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Contains utilities for automatically editing certain files as part of the test creation process.
 */

#include "sequoia/Core/Meta/Concepts.hpp"

#include <filesystem>
#include <vector>

namespace sequoia::testing
{
  [[nodiscard]]
  std::string read_to_string(const std::filesystem::path& file);

  void write_to_file(const std::filesystem::path& file, std::string_view text);

  template<invocable<std::string&> Fn>
  void read_modify_write(const std::filesystem::path& file, Fn fn)
  {
    auto text{read_to_string(file)};
    fn(text);
    write_to_file(file, text);
  }

  void add_include(const std::filesystem::path& file, std::string_view include);

  void add_to_cmake(const std::filesystem::path& cmakeDir, const std::filesystem::path& testDir, const std::filesystem::path& file);

  void add_to_family(const std::filesystem::path& file, std::string_view familyName, const std::vector<std::string>& tests);
}
