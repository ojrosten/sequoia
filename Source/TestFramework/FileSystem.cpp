////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for FileSystem.hpp
 */

#include "FileSystem.hpp"

namespace sequoia::testing
{
  void throw_unless_directory(const std::filesystem::path& p, std::string_view prefix)
  {
    namespace fs = std::filesystem;

    throw_if(p, prefix, " does not exist", [](const fs::path& p){ return !fs::exists(p); });
    throw_if(p, prefix, " is not a directory", [](const fs::path& p){ return !fs::is_directory(p); });
  }

  void throw_unless_regular_file(const std::filesystem::path& p, std::string_view prefix)
  {
    namespace fs = std::filesystem;

    throw_if(p, prefix, " does not exist", [](const fs::path& p){ return !fs::exists(p); });
    throw_if(p, prefix, " is not a regular file", [](const fs::path& p){ return !fs::is_regular_file(p); });
  }
}
