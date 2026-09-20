////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "sequoia/TestFramework/FreeTestCore.hpp"

namespace sequoia::testing
{
  /** \brief Checks that `invoke` names the command interpreter absolutely.

      Asked to resolve the interpreter from a command line alone, Windows searches the calling
      executable's directory and then the current one, both ahead of the system directory - so an
      impostor sitting in either would be run in preference to the shell. sequoia spawns from
      directories it has itself generated, which is precisely where such a file could arrive.
      On POSIX the shell is reached through `std::system`, which has no such search, so the check
      is satisfied whatever `invoke` does.
   */
  class invoke_interpreter_resolution_free_test final : public free_test
  {
  public:
    using free_test::free_test;

    [[nodiscard]]
    static std::filesystem::path source_file();

    void run_tests();
  };
}
