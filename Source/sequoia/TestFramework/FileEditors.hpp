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
#include "sequoia/TextProcessing/Indent.hpp"

#include <filesystem>
#include <optional>
#include <vector>

namespace sequoia::testing
{
  void add_include(const std::filesystem::path& file, std::string_view includePath);

  void add_to_cmake(const std::filesystem::path& cmakeDir,
                    const std::filesystem::path& hostDir,
                    const std::filesystem::path& file,
                    std::string_view patternOpen,
                    std::string_view patternClose,
                    std::string_view cmakeEntryPrexfix);

  void add_to_family(const std::filesystem::path& file, std::string_view familyName, indentation indent, const std::vector<std::string>& tests);


  struct reduced_file_contents
  {
    std::optional<std::string> working, prediction;
  };

  [[nodiscard]]
  reduced_file_contents get_reduced_file_content(const std::filesystem::path& file, const std::filesystem::path& prediction);

}
