////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Contains utilities for updating test materials.
 */

#include <filesystem>

namespace sequoia::testing
{
  void copy_special_files_to_working_copy(const std::filesystem::path& predictions, const std::filesystem::path& working); // TO DO: hide this!
  void soft_update(const std::filesystem::path& working, const std::filesystem::path& prediction);
}