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
  /** \throws std::runtime_error if `referenceRoot` holds no root, or holds a
      root with no regular file `Source/sequoia/CMakeLists.txt`
   */
  [[nodiscard]]
  inline std::filesystem::path reference_sequoia_root(const std::optional<std::filesystem::path>& referenceRoot)
  {
    if(!referenceRoot)
      throw std::runtime_error{
        "No reference root given: run with --reference-root <path>, the root of a sequoia tree to compare against"
      };

    throw_unless_regular_file(source_paths{*referenceRoot, "sequoia"}.cmake_lists(),
                              "\nThe reference root must be the root of a sequoia tree");

    return *referenceRoot;
  }
}
