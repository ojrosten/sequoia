////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Utilities for use in tests.
*/

#include "sequoia/Streaming/Streaming.hpp"
#include "sequoia/TestFramework/ProjectPaths.hpp"
#include "sequoia/TextProcessing/Substitutions.hpp"

#include <chrono>
#include <filesystem>
#include <format>
#include <string>

namespace sequoia::testing
{
  /** \brief `message`, with every path beneath the project root made relative to the root. */
  [[nodiscard]]
  inline std::string relative_to_root(const project_paths& projPaths, std::string message)
  {
    replace_all(message, projPaths.project_root().generic_string() + "/", "");
    return message;
  }

  /** \brief Sets the modification time of `executable` to a day from now.

      A runner refuses to start from an executable no newer than one of sequoia's own files. prune refuses to analyse
      a build whose executable is no newer than a file the build read. A fake executable dated by this function passes
      both checks, whatever is edited while the tests run.
   */
  inline void date_after_every_edit(const std::filesystem::path& executable)
  {
    std::filesystem::last_write_time(executable, std::chrono::file_clock::now() + std::chrono::days{1});
  }

  /** \brief A file with the given contents, which exists for precisely the lifetime of the object. */
  class transient_file
  {
  public:
    transient_file(std::filesystem::path file, std::string_view contents)
      : m_File{std::move(file)}
    {
      write_to_file(m_File, contents, std::ios_base::out | std::ios_base::binary);
    }

    transient_file(const transient_file&) = delete;

    transient_file& operator=(const transient_file&) = delete;

    ~transient_file()
    {
      std::error_code ignored{};
      std::filesystem::remove(m_File, ignored);
    }

    [[nodiscard]]
    const std::filesystem::path& path() const noexcept { return m_File; }
  private:
    std::filesystem::path m_File{};
  };

  /** \brief A directory which is removed, with its contents, when the object is destroyed. The object
             does not create the directory.
   */
  class transient_directory
  {
  public:
    explicit transient_directory(std::filesystem::path dir)
      : m_Dir{std::move(dir)}
    {}

    transient_directory(const transient_directory&) = delete;

    transient_directory& operator=(const transient_directory&) = delete;

    ~transient_directory()
    {
      std::error_code ignored{};
      std::filesystem::remove_all(m_Dir, ignored);
    }

    [[nodiscard]]
    const std::filesystem::path& path() const noexcept { return m_Dir; }
  private:
    std::filesystem::path m_Dir{};
  };

  class no_default_constructor
  {
  public:
    constexpr explicit no_default_constructor(int i) : m_i{i} {}

    [[nodiscard]]
    constexpr int get() const noexcept { return m_i; }

    [[nodiscard]]
    constexpr friend bool operator==(const no_default_constructor&, const no_default_constructor&) noexcept = default;
  private:
    int m_i{};
  };
}

namespace std {
  template<>
  struct formatter<sequoia::testing::no_default_constructor>
  {
    constexpr auto parse(auto& ctx) { return ctx.begin(); }

    auto format(const sequoia::testing::no_default_constructor& ndc, auto& ctx) const
    {
      return std::format_to(ctx.out(), "{}", ndc.get());
    }
  };
}
