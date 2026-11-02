////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "sequoia/TestFramework/FileSystemUtilities.hpp"
#include "sequoia/TestFramework/ProjectPaths.hpp"

#include <stdexcept>

namespace sequoia::testing
{
  /** \throws std::runtime_error if `projPaths` holds no reference root, or
      if the reference root holds no regular file
      `Source/sequoia/CMakeLists.txt`
   */
  [[nodiscard]]
  inline std::filesystem::path reference_sequoia_root(const project_paths& projPaths)
  {
    const auto& root{projPaths.reference_root()};
    if(!root)
      throw std::runtime_error{
        "No reference root given: run with --reference-root <path>, the root of a sequoia tree to compare against"
      };

    throw_unless_regular_file(source_paths{*root, "sequoia"}.cmake_lists(),
                              "\nThe reference root must be the root of a sequoia tree");

    return *root;
  }
}
