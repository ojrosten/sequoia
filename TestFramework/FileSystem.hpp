////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief File paths and related utilities.
 */


#include <filesystem>

namespace sequoia::testing
{
  const static auto working_path_v{std::filesystem::current_path()};

  [[nodiscard]]
  inline std::filesystem::path working_path()
  {
    return working_path_v;
  }

  [[nodiscard]]
  inline std::filesystem::path parent_path()
  {
    return working_path().parent_path();
  }
  
  [[nodiscard]]
  inline std::filesystem::path get_output_path(std::string_view subDirectory)
  {
    return parent_path().append("output").append(subDirectory);
  }  

  [[nodiscard]]
  inline std::filesystem::path get_aux_path(std::string_view subDirectory)
  {
    return parent_path().append("aux_files").append(subDirectory);
  }
}
