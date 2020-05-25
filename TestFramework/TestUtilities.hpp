////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Utilities for use in tests.
*/

namespace sequoia::testing
{
  class no_default_constructor
  {
  public:
    constexpr explicit no_default_constructor(int i) : m_i{i} {}

    [[nodiscard]]
    constexpr int get() const noexcept { return m_i; }

    [[nodiscard]]
    constexpr friend bool operator==(const no_default_constructor& lhs, const no_default_constructor& rhs) noexcept
    {
      return lhs.get() == rhs.get();
    }

    [[nodiscard]]
    constexpr friend bool operator!=(const no_default_constructor& lhs, const no_default_constructor& rhs) noexcept
    {
      return !(lhs == rhs);
    }

    template<class Stream>
    friend Stream& operator<<(Stream& stream, const no_default_constructor& ndc)
    {
      stream << ndc.get();
      return stream;
    }
  private:
    int m_i{};
  };
}
