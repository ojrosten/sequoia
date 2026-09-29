////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Utilities for removing tests, especially from the commandline: the inverse of creating them.
 */

#include "sequoia/TestFramework/ProjectPaths.hpp"

#include <vector>

namespace sequoia::testing
{
  /** \brief A test as a runner registered it: the name of its class, and its source file. */
  struct test_registration
  {
    std::string name{};
    std::filesystem::path source{};

    [[nodiscard]]
    friend bool operator==(const test_registration&, const test_registration&) noexcept = default;
  };

  /** \brief Removes each of `testsToRemove`, with everything the project holds for it; what it tests is untouched.

      A test's source file goes, with the header of the same stem, and so does every line elsewhere in the project
      naming either: registrations in the mains, entries in each `CMakeLists.txt`, and includes. The test's
      materials go, and its versioned output for every configuration. A directory left empty goes too.

      `registered` is every test the runner registered. It decides which versioned output is a removed test's, since
      the name of one test may begin that of another.

      \throws std::runtime_error, before anything is removed, if a test in `testsToRemove` shares its source file
      with a test in `registered` which is not to be removed.
   */
  void remove_tests(const project_paths& projPaths,
                    const std::vector<test_registration>& testsToRemove,
                    const std::vector<test_registration>& registered,
                    std::ostream& stream);
}
