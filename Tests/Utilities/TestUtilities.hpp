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

  /** \brief A file with the given contents and no write permissions, which exists for precisely the lifetime of the
             object.

      Opening the file to write it fails, for any user but root. The object restores the permissions before removing
      the file, which Windows refuses to remove while it is read-only.
   */
  class read_only_file
  {
  public:
    read_only_file(std::filesystem::path file, std::string_view contents)
      : m_File{std::move(file), contents}
    {
      std::filesystem::permissions(m_File.path(), st_WritePermissions, std::filesystem::perm_options::remove);
    }

    ~read_only_file()
    {
      std::error_code ignored{};
      std::filesystem::permissions(m_File.path(), st_WritePermissions, std::filesystem::perm_options::add, ignored);
    }

    [[nodiscard]]
    const std::filesystem::path& path() const noexcept { return m_File.path(); }
  private:
    constexpr static auto st_WritePermissions{
      std::filesystem::perms::owner_write | std::filesystem::perms::group_write | std::filesystem::perms::others_write
    };

    transient_file m_File;
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
