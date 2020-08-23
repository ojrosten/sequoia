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

#include <filesystem>
#include <vector>

namespace sequoia::testing
{
  void add_include(const std::filesystem::path& filePath, std::string_view include);

  void add_to_family(const std::filesystem::path& filePath, std::string_view familyName, const std::vector<std::string>& tests);
}
