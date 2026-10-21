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

#include <filesystem>
#include <format>
#include <fstream>
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

  /** \brief An RAII wrapper to make a directory which cannot be removed while
             the object lives.

      The object makes the directory, keeps a file within it open, and removes
      the directory's write permissions. Under Windows the open file stops the
      directory's removal; elsewhere the permissions do, for any user but root.
      The object restores the permissions on destruction, and leaves the
      directory in place.
   */
  class unremovable_directory
  {
  public:
    explicit unremovable_directory(std::filesystem::path dir)
      : m_Dir{std::move(dir)}
      , m_OpenFile{open_file_in(m_Dir)}
      , m_Permissions{std::filesystem::status(m_Dir).permissions()}
    {
      std::filesystem::permissions(m_Dir, st_WritePermissions, std::filesystem::perm_options::remove);
    }

    unremovable_directory(const unremovable_directory&) = delete;

    unremovable_directory& operator=(const unremovable_directory&) = delete;

    ~unremovable_directory()
    {
      std::error_code ignored{};
      std::filesystem::permissions(m_Dir, m_Permissions, std::filesystem::perm_options::replace, ignored);
    }

    [[nodiscard]]
    const std::filesystem::path& path() const noexcept { return m_Dir; }
  private:
    constexpr static auto st_WritePermissions{
      std::filesystem::perms::owner_write | std::filesystem::perms::group_write | std::filesystem::perms::others_write
    };

    std::filesystem::path  m_Dir{};
    std::ofstream          m_OpenFile{};
    std::filesystem::perms m_Permissions{};

    [[nodiscard]]
    static std::ofstream open_file_in(const std::filesystem::path& dir)
    {
      std::filesystem::create_directories(dir);
      return std::ofstream{dir / "Open.txt"};
    }
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
