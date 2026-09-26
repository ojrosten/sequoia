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

#include <filesystem>
#include <format>

namespace sequoia::testing
{
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
    std::filesystem::path m_File;
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
