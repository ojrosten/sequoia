////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "Concepts.hpp"

namespace sequoia::maths
{
  template<class T>
  concept floating_point = std::is_floating_point_v<T>;

  template<floating_point T>
  class probability
  {
  public:
    explicit probability(T p)
      : m_Prob{p}
    {
      if(m_Prob < T{} || m_Prob > T(1))
        throw std::logic_error{"Out of range"};
    }

    [[nodiscard]]
    double raw_value() const noexcept
    {
      return m_Prob;
    }

    [[nodiscard]]
    friend auto operator<=>(const probability&, const probability&) noexcept = default;
  private:
    T m_Prob;
  };
}
