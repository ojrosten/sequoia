////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/Runtime/ShellCommands.hpp"

#include "sequoia/PlatformSpecific/Preprocessor.hpp"

#include <iostream>

#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #include "Windows.h"

  #include <algorithm>
  #include <array>
  #include <cstddef>
  #include <ranges>
  #include <vector>
#else
  #include <cstdlib>

  #include "sys/wait.h"
#endif

namespace sequoia::runtime
{
  namespace
  {
  #ifdef _WIN32
    [[nodiscard]]
    bool is_inheritable(HANDLE handle)
    {
      DWORD flags{};
      return (handle != nullptr)
          && (handle != INVALID_HANDLE_VALUE)
          &&  GetHandleInformation(handle, &flags)
          && (flags & HANDLE_FLAG_INHERIT);
    }

    /** \brief The command interpreter, named absolutely.

        Supplying this as the application name matters. Left to resolve the interpreter from the
        command line alone, CreateProcessW searches the **current directory** ahead of the system
        one, and sequoia spawns from directories it has itself just generated - so a stray
        executable among a generated project's artefacts would be run in preference to the shell.
        The runtime's own system() had no such exposure, resolving through COMSPEC.
     */
    [[nodiscard]]
    std::wstring command_interpreter()
    {
      std::wstring directory(MAX_PATH, L'\0');
      const auto length{GetSystemDirectoryW(directory.data(), static_cast<UINT>(directory.size()))};
      if(!length || (length > directory.size())) return {};

      directory.resize(length);
      return directory.append(L"\\cmd.exe");
    }

    /** \brief Runs a command through the interpreter, passing on the standard streams and nothing else.

        std::system would do the same job in a line, but spawns with bInheritHandles = TRUE, which
        hands the child a duplicate of *every* inheritable handle the process holds at that instant
        - and the MSVC runtime opens files inheritably by default. Since sequoia runs its top-level
        tests concurrently, a spawn on one thread can therefore capture a file another thread has
        open, and on Windows a file cannot be deleted while any handle to it remains. The duplicate
        outlives the stream that created it, so a delete issued long afterwards fails with a
        sharing violation - intermittently, according to which thread was where.

        Restricting inheritance to an explicit list closes that off at the only point where sequoia
        creates a process, rather than at the delete sites where it happened to show.
     */
    [[nodiscard]]
    int spawn_and_wait(const std::string& command)
    {
      const auto interpreter{command_interpreter()};
      if(interpreter.empty()) return -1;

      const std::array<HANDLE, 3> standardStreams{GetStdHandle(STD_INPUT_HANDLE),
                                                  GetStdHandle(STD_OUTPUT_HANDLE),
                                                  GetStdHandle(STD_ERROR_HANDLE)};

      // All three or none. Passing only some, with STARTF_USESTDHANDLES set, would leave the child
      // holding nothing at all for the remainder, whereas inheriting nothing lets it fall back to
      // the console it is attached to.
      // Not const: UpdateProcThreadAttribute takes the handle list through a PVOID.
      auto inherited{
        [&standardStreams]() -> std::vector<HANDLE> {
          if(!std::ranges::all_of(standardStreams, is_inheritable)) return {};

          // A handle may appear more than once - stdout and stderr are the same when one is
          // redirected onto the other - and the attribute list will not accept a duplicate.
          auto handles{standardStreams | std::ranges::to<std::vector>()};
          std::ranges::sort(handles);
          handles.erase(std::ranges::unique(handles).begin(), handles.end());

          return handles;
        }()
      };

      STARTUPINFOEXW startupInfo{.StartupInfo{.cb = sizeof(STARTUPINFOW)}};

      std::vector<std::byte> attributeStorage{};
      LPPROC_THREAD_ATTRIBUTE_LIST attributes{};

      if(!inherited.empty())
      {
        SIZE_T size{};
        InitializeProcThreadAttributeList(nullptr, 1, 0, &size);
        attributeStorage.resize(size);

        if(auto* candidate{reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attributeStorage.data())};
           InitializeProcThreadAttributeList(candidate, 1, 0, &size))
        {
          // Initialized, and so owed a matching deletion however the rest of this turns out.
          attributes = candidate;

          if(UpdateProcThreadAttribute(attributes, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, inherited.data(), inherited.size() * sizeof(HANDLE), nullptr, nullptr))
          {
            startupInfo.lpAttributeList        = attributes;
            startupInfo.StartupInfo.cb         = sizeof(startupInfo);
            startupInfo.StartupInfo.dwFlags    = STARTF_USESTDHANDLES;
            startupInfo.StartupInfo.hStdInput  = standardStreams[0];
            startupInfo.StartupInfo.hStdOutput = standardStreams[1];
            startupInfo.StartupInfo.hStdError  = standardStreams[2];
          }
        }
      }

      // With no attribute list there is nothing to inherit selectively, so inherit nothing at all.
      const BOOL inheritHandles{startupInfo.lpAttributeList != nullptr};

      // Everything after /c reaches the interpreter verbatim, which is what lets a composite
      // command carrying quotes and redirections through unaltered. Widening via path is exactly
      // the inverse of the conversion which built the narrow string, so the two cannot disagree.
      // Not const: CreateProcessW is documented to modify this buffer in place.
      auto commandLine{std::wstring{L"cmd.exe /c "}.append(std::filesystem::path{command}.wstring())};

      PROCESS_INFORMATION processInfo{};
      const auto created{
        CreateProcessW(interpreter.data(),
                       commandLine.data(),
                       nullptr,
                       nullptr,
                       inheritHandles,
                       inheritHandles ? EXTENDED_STARTUPINFO_PRESENT : 0,
                       nullptr,
                       nullptr,
                       &startupInfo.StartupInfo,
                       &processInfo)
      };

      if(attributes) DeleteProcThreadAttributeList(attributes);

      if(!created) return -1;

      const auto waited{WaitForSingleObject(processInfo.hProcess, INFINITE) == WAIT_OBJECT_0};

      // Without a successful wait the exit code reads as STILL_ACTIVE, which would otherwise be
      // returned as though it were the child's own status.
      DWORD exitCode{};
      const auto reported{waited && GetExitCodeProcess(processInfo.hProcess, &exitCode)};

      CloseHandle(processInfo.hThread);
      CloseHandle(processInfo.hProcess);

      return reported ? static_cast<int>(exitCode) : -1;
    }
  #else
    [[nodiscard]]
    int spawn_and_wait(const std::string& command)
    {
      const auto status{std::system(command.data())};
      return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    }
  #endif
  }

  shell_command::shell_command(std::string cmd, const std::filesystem::path& output, append_mode app)
    : m_Command{std::move(cmd)}
  {
    if(!output.empty())
    {
      if(!m_Command.empty() && std::isdigit(m_Command.back()))
        m_Command.append(" ");

      m_Command.append(app == append_mode::no ? "> " : ">> ");
      m_Command.append(output.string()).append(" 2>&1");
    }
  }

  shell_command::shell_command(std::string_view preamble, std::string cmd, const std::filesystem::path& output, append_mode app)
  {
    const auto pre{
      [&]() -> shell_command {
        if(!preamble.empty())
        {
          const std::string newline{with_windows_v ? "echo/" : "echo"};
          return shell_command{newline, output, app}
              && shell_command{std::string{"echo "}.append(preamble), output, append_mode::yes}
              && shell_command{newline, output, append_mode::yes};
        }

        return {};
      }()
    };

    *this = pre && shell_command{std::move(cmd), output, !pre.empty() ? append_mode::yes : app};
  }

  int invoke(const shell_command& cmd)
  {
    std::cout << std::flush;
    if(cmd.empty()) return 0;

    return spawn_and_wait(cmd.m_Command);
  }

  [[nodiscard]]
  shell_command cd_cmd(const std::filesystem::path& dir)
  {
    return std::string{"cd "}.append(dir.string());
  }
}
