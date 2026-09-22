////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

export module sequoia.file_system;

import std;

/** \file
    \brief Extensions to the std::filesystem library
 */

export namespace sequoia
{
  class normal_path
  {
  public:
    normal_path() = default;

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

    [[nodiscard]]
    friend auto operator<=>(const normal_path&, const normal_path&) = default;
  private:
    std::filesystem::path m_Path;
  };

  [[nodiscard]]
  inline std::filesystem::path back(const std::filesystem::path& p)
  {
    if(p.empty())
      throw std::runtime_error{"Cannot extract final element from an empty path"};

    return *--p.end();
  }
}
