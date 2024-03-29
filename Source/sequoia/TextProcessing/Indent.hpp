////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief A collection of functions for formatting test output.
 */

#include <algorithm>
#include <string>
#include <utility>

namespace sequoia
{
  /*! \brief Type-safe mechanism for indentations */
  class indentation
  {
  public:
    using size_type = std::string::size_type;

    indentation() = default;

    explicit indentation(std::string s)
      : m_Data{std::move(s)}
    {}

    [[nodiscard]]
    operator std::string_view() const noexcept
    {
      return m_Data;
    }

    indentation& append(size_type count, char c)
    {
      m_Data.append(count, c);
      return *this;
    }

    indentation& append(const std::string& s)
    {
      m_Data.append(s);
      return *this;
    }

    indentation& trim(size_type count)
    {
      const auto pos{m_Data.size() - std::ranges::min(count, m_Data.size())};
      m_Data.erase(pos);
      return *this;
    }

    [[nodiscard]]
    friend indentation operator+(const indentation& lhs, const indentation& rhs)
    {
      return indentation{lhs.m_Data + rhs.m_Data};
    }

    [[nodiscard]]
    friend bool operator==(const indentation&, const indentation&) noexcept = default;
  private:
    std::string m_Data;
  };

  const indentation tab{"\t"};
  const indentation no_indent{""};

  /// For a non-empty string_view prepends with an indentation; otherwise returns an empty string
  [[nodiscard]]
  std::string indent(std::string_view sv, indentation ind);

  /*! \param s1 The target for appending
      \param s2 The text to append
      \param indentation The absolute (not relative) indentation of s2

      If s1 and s2 are both non-empty, a new line is appended to s1, followed by the indentation
      and then s2.

      If s1 is empty, then s1 is set equal to s2, with no indentation.

      If s2 is empty, no action is taken.
   */
  std::string& append_indented(std::string& s1, std::string_view s2, indentation ind);

  [[nodiscard]]
  std::string append_indented(std::string_view sv1, std::string_view sv2, indentation ind);

  namespace impl
  {
    template<class... Ts, std::size_t... I>
    std::string& append_indented(std::string& s, std::tuple<Ts...> strs, std::index_sequence<I...>)
    {
      (append_indented(s, std::get<I>(strs), std::get<sizeof...(Ts) - 1>(strs)), ...);
      return s;
    }

    template<class... Ts>
    std::string& append_indented(std::string& s, const std::tuple<Ts...>& strs)
    {
      return append_indented(s, strs, std::make_index_sequence<sizeof...(Ts) - 1>{});
    }

    template<class... Ts>
    [[nodiscard]]
    std::string indent(std::string_view sv, const std::tuple<Ts...>& strs)
    {
      auto s{indent(sv, std::get<sizeof...(Ts) - 1>(strs))};
      return append_indented(s, strs);
    }
  }

  template<class... Ts>
    requires (sizeof...(Ts) > 2)
  std::string& append_indented(std::string& s, Ts... strs)
  {
    return sequoia::impl::append_indented(s, std::tuple<Ts...>{strs...});
  }

  template<class... Ts>
    requires (sizeof...(Ts) > 2)
  [[nodiscard]]
  std::string append_indented(std::string_view sv, Ts... strs)
  {
    std::string str{sv};
    return append_indented(str, std::forward<Ts>(strs)...);
  }

  template<class... Ts>
    requires (sizeof...(Ts) > 1)
  [[nodiscard]]
  std::string indent(std::string_view sv, Ts&&... strs)
  {
    return sequoia::impl::indent(sv, std::tuple<Ts...>{std::forward<Ts>(strs)...});
  }

  template<class... Ts>
    requires (sizeof...(Ts) > 0)
  std::string& append_lines(std::string& s, Ts&&... strs)
  {
    return append_indented(s, std::forward<Ts>(strs)..., no_indent);
  }

  template<class... Ts>
    requires (sizeof...(Ts) > 0)
  [[nodiscard]]
  std::string append_lines(std::string_view sv, Ts&&... strs)
  {
    std::string str{sv};
    return append_lines(str, std::forward<Ts>(strs)...);
  }

  std::string& tabs_to_spacing(std::string& text, std::string_view spacing);
}
