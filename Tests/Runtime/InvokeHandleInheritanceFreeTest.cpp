////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "InvokeHandleInheritanceFreeTest.hpp"

#include "sequoia/PlatformSpecific/Preprocessor.hpp"
#include "sequoia/Runtime/ShellCommands.hpp"

#include <fstream>

namespace sequoia::testing
{
  using namespace runtime;

  namespace
  {
    enum class spawn_point { none, before_opening, while_open };

    /** \brief A command whose invocation returns at once, leaving a *grandchild* running behind it.

        invoke waits for the process it launches, so nothing invoke spawns directly can outlive the
        call; only a detached grandchild can. That is what makes the ordering forced rather than
        racy: by the time invoke returns, the grandchild is known to exist, and to hold whatever its
        parent handed it.
     */
    [[nodiscard]]
    shell_command lingering_grandchild_cmd()
    {
      return with_windows_v ? shell_command{"start /b ping -n 3 127.0.0.1 > nul"}
                            : shell_command{"sleep 2 &"};
    }

    /// What one trial observed, so a trial which never ran cannot be mistaken for one which passed.
    struct trial_outcome
    {
      bool written{}, removed{};
      std::error_code error{};
      int spawnStatus{};
    };

    /// Rewrites `file`, spawns at `when`, closes, and reports what removing the file did.
    [[nodiscard]]
    trial_outcome trial(const std::filesystem::path& file, spawn_point when)
    {
      trial_outcome outcome{};

      if(when == spawn_point::before_opening) outcome.spawnStatus = invoke(lingering_grandchild_cmd());

      {
        std::ofstream stream{file};
        stream << "payload";
        stream.flush();

        if(when == spawn_point::while_open) outcome.spawnStatus = invoke(lingering_grandchild_cmd());
      }

      outcome.written = std::filesystem::exists(file);
      outcome.removed = std::filesystem::remove(file, outcome.error);

      return outcome;
    }
  }

  [[nodiscard]]
  std::filesystem::path invoke_handle_inheritance_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void invoke_handle_inheritance_free_test::run_tests()
  {
    // An unfound materials directory yields an empty path, which would quietly put the scratch
    // files in the working directory and let every trial below pass without exercising anything.
    if(!check("Test materials are in place", std::filesystem::exists(working_materials()))) return;

    const auto noSpawn             {trial(working_materials() / "NoSpawn.txt",              spawn_point::none)},
               spawnedBeforeOpening{trial(working_materials() / "SpawnedBeforeOpening.txt", spawn_point::before_opening)},
               spawnedWhileOpen    {trial(working_materials() / "SpawnedWhileOpen.txt",     spawn_point::while_open)};

    // A trial must have got as far as a written file, or its removal proves nothing: a file which
    // was never created is trivially gone.
    check("File written, no process spawned",                   noSpawn.written);
    check("File written, process spawned before it was opened", spawnedBeforeOpening.written);
    check("File written, process spawned while it was open",    spawnedWhileOpen.written);

    // Likewise the spawns have to have succeeded, or there is no lingering process to be the
    // difference between the trials.
    check("Spawn succeeded, before the file was opened", spawnedBeforeOpening.spawnStatus == 0);
    check("Spawn succeeded, while the file was open",    spawnedWhileOpen.spawnStatus    == 0);

    auto advice{[](const trial_outcome& outcome) { return tutor{[msg{outcome.error.message()}](bool, bool) { return msg; }}; }};

    // The first two are controls: removal works, and merely having spawned a lingering process
    // does not impede it. Only the ordering of the spawn differs in the third.
    check("Closed file removable, no process spawned",                   noSpawn.removed,              advice(noSpawn));
    check("Closed file removable, process spawned before it was opened", spawnedBeforeOpening.removed, advice(spawnedBeforeOpening));
    check("Closed file removable, process spawned while it was open",    spawnedWhileOpen.removed,     advice(spawnedWhileOpen));
  }
}
