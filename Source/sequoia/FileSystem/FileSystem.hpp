////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Extensions to the std::filesystem library
 */

#include <filesystem>

namespace sequoia
{
  class normal_path
  {
  public:
    normal_path(const std::filesystem::path& path)
      : m_Path{path.lexically_normal()}
    {}

    [[nodiscard]]
    operator const std::filesystem::path& () const
    {
      return m_Path;
    }

    [[nodiscard]]
    const std::filesystem::path& path() const noexcept
    {
      return m_Path;
    }

    // TO DO: use spaceship when supported by libc++ for fs::path
    //[[nodiscard]]
    //friend auto operator<=>(const normal_path&, const normal_path&) = default;

    [[nodiscard]]
    friend bool operator==(const normal_path&, const normal_path&) = default;

    [[nodiscard]]
    friend bool operator!=(const normal_path&, const normal_path&) = default;
  private:
    std::filesystem::path m_Path;
  };

  /// precondition: a non-empty path; return value: the last element of the path
  [[nodiscard]]
  inline std::filesystem::path back(const std::filesystem::path& p)
  {
    return *--p.end();
  }
}