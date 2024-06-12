////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief A class template representation angles, plus trigonometric functions
 */

#include "sequoia/Core/Meta/TypeTraits.hpp"

#include <numbers>

namespace sequoia::maths
{
  template<std::floating_point T, T Period>
  class angle
  {
  public:
      using value_type = T;
      constexpr static T period = Period;

      constexpr angle() noexcept = default;

      constexpr explicit angle(T angle) noexcept : m_Angle{angle} {}

      template<class U>
          requires is_initializable_v<T, U>
      constexpr angle& operator+=(angle<U, Period> rhs) noexcept
      {
          m_Angle += rhs.value();
          return *this;
      }

      template<class U>
          requires is_initializable_v<T, U>
      constexpr angle& operator-=(angle<U, Period> rhs) noexcept
      {
          m_Angle -= rhs.value();
          return *this;
      }

      [[nodiscard]]
      constexpr angle operator+() const noexcept
      {
        return *this;
      }

      [[nodiscard]]
      constexpr angle operator-() const noexcept
      {
          return angle{-value()};
      }

      template<std::floating_point U>
      [[nodiscard]]
      friend constexpr angle<std::common_type_t<T, U>, Period> operator+(angle<T, Period> lhs, angle<U, Period> rhs) noexcept
      {
          using common_t = std::common_type_t<T, U>;
          return angle<common_t, Period>{lhs.value() + rhs.value()};
      }

      template<std::floating_point U>
      [[nodiscard]]
      friend constexpr angle<std::common_type_t<T, U>, Period> operator-(angle<T, Period> lhs, angle<U, Period> rhs) noexcept
      {
          using common_t = std::common_type_t<T, U>;
          return angle<common_t, Period>{lhs.value() - rhs.value()};
      }

      template<class U>
          requires is_compatible_v<T, U>
      constexpr angle& operator*=(U rhs) noexcept
      {
          m_Angle *= rhs;
          return *this;
      }

      template<class U>
        requires is_compatible_v<T, U>
      [[nodiscard]]
      friend constexpr angle<std::common_type_t<T, U>, Period> operator*(angle<T, Period> lhs, U rhs) noexcept
      {
          using common_t = std::common_type_t<T, U>;
          return angle<common_t, Period>{lhs.value()* rhs};
      }

      template<class U>
        requires is_compatible_v<T, U>
      [[nodiscard]]
      friend constexpr angle<std::common_type_t<T, U>, Period> operator*(U lhs, angle<T, Period> rhs) noexcept
      {
          return rhs * lhs;
      }

      template<class U>
          requires is_compatible_v<T, U>
      constexpr angle& operator/=(U rhs)
      {
          m_Angle /= rhs;
          return *this;
      }

      template<class U>
        requires is_compatible_v<T, U>
      [[nodiscard]]
      friend constexpr angle<std::common_type_t<T, U>, Period> operator/(angle<T, Period> lhs, U rhs)
      {
          using common_t = std::common_type_t<T, U>;
          return angle<common_t, Period>{lhs.value() / rhs};
      }

      [[nodiscard]]
      constexpr T value() const noexcept { return m_Angle; }

      [[nodiscard]]
      constexpr angle principal_angle() const noexcept { return angle{std::fmod(m_Angle, period)}; }

      [[nodiscard]]
      friend constexpr auto operator<=>(const angle&, const angle&) noexcept = default;
  private:
      T m_Angle{};
  };

  template<std::floating_point T>
  using radians = angle<T, 2 * std::numbers::pi_v<T>>;

  template<std::floating_point T>
  using degrees = angle<T, T(360)>;

  template<auto ToPeriod, std::floating_point T, T FromPeriod>
    requires std::is_floating_point_v<decltype(ToPeriod)>
  [[nodiscard]]
  constexpr angle<decltype(ToPeriod), ToPeriod> convert(angle<T, FromPeriod> theta) noexcept
  {
    using U = decltype(ToPeriod);
    return angle<U, ToPeriod>{static_cast<U>(theta.value() * ToPeriod / FromPeriod)};
  }

  template<std::floating_point T>
  [[nodiscard]]
  constexpr radians<T> to_radians(degrees<T> theta) noexcept
  {
    return convert<radians<T>::period>(theta);
  }

  template<std::floating_point T>
  [[nodiscard]]
  constexpr degrees<T> to_degrees(radians<T> theta) noexcept
  {
    return convert<degrees<T>::period>(theta);
  }

  template<std::floating_point T, T Period>
  [[nodiscard]]
  constexpr T sin(angle<T, Period> theta) { return std::sin(convert<radians<T>::period>(theta).value()); }

  template<std::floating_point T, T Period>
  [[nodiscard]]
  constexpr T cos(angle<T, Period> theta) { return std::cos(convert<radians<T>::period>(theta).value()); }

  template<std::floating_point T, T Period>
  [[nodiscard]]
  constexpr T tan(angle<T, Period> theta) { return std::tan(convert<radians<T>::period>(theta).value()); }

  template<std::floating_point T>
  radians<T> asin(T x) { return radians<T>{std::asin(x)}; }

  template<std::floating_point T>
  radians<T> acos(T x) { return radians<T>{std::acos(x)}; }

  template<std::floating_point T>
  radians<T> atan(T x) { return radians<T>{std::atan(x)}; }
}
