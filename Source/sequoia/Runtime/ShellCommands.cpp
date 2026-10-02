////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/Runtime/ShellCommands.hpp"

#include "sequoia/PlatformSpecific/Preprocessor.hpp"
#include "sequoia/TextProcessing/Characters.hpp"

#include <cstdint>
#include <format>
#include <iostream>
#include <stdexcept>

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

    /** \brief The absolute path of `cmd.exe` in the Windows system directory.

        \returns
        -# The path, if Windows reports the path of the system directory;
        -# Otherwise, an empty string.
     */
    [[nodiscard]]
    std::wstring command_interpreter()
    {
      std::wstring directory(MAX_PATH, L'\0');
      const auto length{GetSystemDirectoryW(directory.data(), static_cast<UINT>(directory.size()))};
      if(!length || (length > directory.size()))
        return {};

      directory.resize(length);
      return directory.append(L"\\cmd.exe");
    }

    /** \brief Runs `command` through `cmd.exe`, and waits for the command to finish. The child
               inherits at most the standard streams.

        \returns
        -# -1, if the child fails to start, or its exit status is unavailable;
        -# Otherwise, the child's exit status.

        `std::system` would make the child inherit every inheritable handle the process holds, and
        the MSVC runtime opens files inheritably by default. sequoia runs tests concurrently, so a
        child spawned on one thread could hold a duplicate of a handle to a file another thread has
        open. The duplicate stays open after that thread closes the file. On Windows, a file cannot
        be deleted while any handle to it remains, so a later delete of the file would fail with a
        sharing violation, intermittently.
     */
    [[nodiscard]]
    int spawn_and_wait(const std::string& command)
    {
      const auto interpreter{command_interpreter()};
      if(interpreter.empty())
        return -1;

      const std::array<HANDLE, 3> standardStreams{GetStdHandle(STD_INPUT_HANDLE),
                                                  GetStdHandle(STD_OUTPUT_HANDLE),
                                                  GetStdHandle(STD_ERROR_HANDLE)};

      // The child inherits either all three standard streams or none of them. With
      // STARTF_USESTDHANDLES set, the child gets no handle at all for a stream missing from the
      // list. A child which inherits no streams uses its console.
      // Not const: UpdateProcThreadAttribute takes the handle list through a PVOID.
      auto inherited{
        [&standardStreams]() -> std::vector<HANDLE> {
          if(!std::ranges::all_of(standardStreams, [](HANDLE handle) { return is_inheritable(handle); }))
            return {};

          // The attribute list refuses a duplicate handle. stdout and stderr share a handle when
          // one is redirected onto the other.
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
          // An initialized list needs a matching DeleteProcThreadAttributeList, whatever happens
          // next. `attributes` holds the list only once the list is initialized.
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

      // The child inherits handles only through the attribute list. Without the list, the child
      // inherits no handles, rather than every inheritable one.
      const BOOL inheritHandles{startupInfo.lpAttributeList != nullptr};

      // cmd.exe receives everything after /c verbatim, so a composite command keeps its quotes
      // and redirections. `path::wstring` reverses the conversion `path::string` made when the
      // command was built, so no character changes.
      // Not const: CreateProcessW may write to this buffer.
      auto commandLine{std::wstring{L"cmd.exe /c "}.append(std::filesystem::path{command}.wstring())};

      // `interpreter` names cmd.exe by its absolute path. Given only the command line, CreateProcessW
      // looks for the interpreter in the current directory before the system directory. sequoia
      // runs commands from directories it has just generated, so a stray `cmd.exe` there would run
      // in place of the shell.
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

      if(attributes)
        DeleteProcThreadAttributeList(attributes);

      if(!created)
        return -1;

      const auto waited{WaitForSingleObject(processInfo.hProcess, INFINITE) == WAIT_OBJECT_0};

      // `reported` requires a successful wait. Before the child ends, GetExitCodeProcess reports
      // STILL_ACTIVE, and that value would pass for the child's status.
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

    /** \brief Describes how a command ended, by a POSIX shell's conventions.

        The platform numbers its signals from 1 to `highestSignal`.
     */
    [[nodiscard]]
    std::string describe_posix_exit_status(const int status, const int highestSignal)
    {
      if(status == 0)
        return "succeeded (exit status 0)";

      if(status < 0)
        return "did not run to completion";

      if(status == 126)
        return "could not be executed by the shell (exit status 126)";

      if(status == 127)
        return "was not found by the shell (exit status 127)";

      constexpr int signalOffset{128};
      if((status > signalOffset) && (status - signalOffset <= highestSignal))
        return std::format("failed with exit status {}, which may mean it was killed by signal {}",
                           status,
                           status - signalOffset);

      return std::format("failed with exit status {}", status);
    }
  }

  shell_command::shell_command(std::string cmd, const std::filesystem::path& output, append_mode app)
    : m_Command{std::move(cmd)}
  {
    if(!output.empty())
    {
      if(!m_Command.empty() && ascii::is_digit(m_Command.back()))
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
    if(cmd.empty())
      return 0;

    return spawn_and_wait(cmd.m_Command);
  }

  [[nodiscard]]
  shell_command cd_cmd(const std::filesystem::path& dir)
  {
    return std::string{with_windows_v ? "cd /d " : "cd "}.append(dir.string());
  }

  [[nodiscard]]
  std::string describe_exit_status(const int status)
  {
    return describe_exit_status(status, platform_constant{});
  }

  [[nodiscard]]
  std::string describe_exit_status(const int status, windows_type)
  {
    if(status == 0)
      return "succeeded (exit status 0)";

    if(status == -1)
      return "did not run to completion, or exited with status 0xFFFFFFFF, which cannot be told apart";

    if(status < 0)
      return std::format("failed with exit status 0x{:08X}", static_cast<std::uint32_t>(status));

    return std::format("failed with exit status {}", status);
  }

  [[nodiscard]]
  std::string describe_exit_status(const int status, macos_type)
  {
    // macOS numbers its signals up to 31: NSIG is 32.
    constexpr int highestSignal{31};
    return describe_posix_exit_status(status, highestSignal);
  }

  [[nodiscard]]
  std::string describe_exit_status(const int status, linux_type)
  {
    // Linux numbers its signals up to SIGRTMAX: 64 with glibc on x86-64 and AArch64, but 127 on MIPS.
    constexpr int highestSignal{64};
    return describe_posix_exit_status(status, highestSignal);
  }

  [[nodiscard]]
  std::string describe_exit_status(const int status, other_os_type)
  {
    // An exit status has 8 bits, so 128 plus a signal is at most 255, and the signal at most 127.
    constexpr int highestSignal{127};
    return describe_posix_exit_status(status, highestSignal);
  }

  void throw_unless_succeeded(const int status, std::string_view step, std::string_view advice)
  {
    if(status == 0)
      return;

    throw std::runtime_error{std::format("{} {}\n{}\n", step, describe_exit_status(status), advice)};
  }

  [[nodiscard]]
  std::string describe_output_location(const std::filesystem::path& dir, const std::filesystem::path& output)
  {
    return output.empty() ? std::string{"on the console, above"}
                          : std::format("in {}", (dir / output).generic_string());
  }
}
