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
#include <functional>

namespace sequoia
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

  /*! This function has slightly peculiar semantics, due to the fact that path::iterator isn't
      strictly bidirectional - which MSVC exploits. Therefore, it returns a forward iterator
      to the last instance of a pattern or end, otherwise.
  */
  template<class Path, class Pattern, class Proj=std::identity>
    requires std::is_same_v<std::remove_cvref_t<Path>, std::filesystem::path> && std::predicate<std::ranges::equal_to, std::invoke_result_t<Proj, std::filesystem::path> , Pattern>
  [[nodiscard]]
  std::conditional_t<std::is_const_v<std::remove_reference_t<Path>>, std::filesystem::path::const_iterator, std::filesystem::path::iterator> 
    rfind(Path&& p, Pattern pattern, Proj proj = {})
  {
    auto i{p.end()};
    while(i != p.begin())
    {
      --i;
      if(std::ranges::equal_to{}(proj(*i), pattern)) return i;
    }

    return p.end();
  }
}
