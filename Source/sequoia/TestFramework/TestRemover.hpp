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

#include <iosfwd>
#include <string>
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

  /** \brief Removes each registered test a request names - by its class, or by its source file - with its source
      file and the header of the same stem, every line elsewhere in the project naming them, its materials and its
      versioned output for every configuration.

      A request names a class if it has no directory separator or extension. Otherwise it names each test whose
      source file's path ends in the request's path, which must pick out a single source file.

      The lines naming a test are its registrations in the mains, its entry in each `CMakeLists.txt`, and the
      includes of its header. A directory left empty goes too. The type under test, and the companions `create`
      wrote for it, are untouched.

      `registered` is every test the runner registered. It decides which versioned output is a removed test's, since
      the name of one test may begin that of another.

      Every test to remove is expected to be registered, as `create` writes the registration, in the runner's own
      main, and its source listed in the `CMakeLists.txt` beside that main.

      \throws std::runtime_error, before anything is removed, if a request names no test in `registered`, or tests in
      more than one source file; or if a test to remove:
      -# has its source file outside the tests repository;
      -# shares its source file with a test in `registered` which is not to be removed;
      -# is not registered in the runner's own main, or its source is not listed beside it, as `create` writes them.
   */
  void remove_tests(const project_paths& projPaths,
                    const std::vector<std::string>& requests,
                    const std::vector<test_registration>& registered,
                    std::ostream& stream);
}
