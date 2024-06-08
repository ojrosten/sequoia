////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include <compare>
#include <numbers>

namespace sequoia::maths
{
  template<std::floating_point T, class Scalar>
  inline constexpr bool is_compatible_scalar{
      requires {
          requires ((std::floating_point<Scalar> && is_initializable_v<T, Scalar>) || std::is_arithmetic_v<Scalar>);
      }
  };

  template<std::floating_point T, std::floating_point U>
  struct common_fp_type : std::common_type<std::remove_cvref_t<T>, std::remove_cvref_t<U>>
  {};

  template<std::floating_point T, std::floating_point U>
  using common_fp_type_t = typename common_fp_type<T, U>::type;

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
      constexpr angle(angle<U, Period> other) noexcept : m_Angle{other.value()} {}

      template<class U>
          requires is_initializable_v<T, U>
      constexpr angle& operator=(const angle<U, Period>& other) noexcept
      {
          m_Angle = other.value();
          return *this;
      }

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
      constexpr angle operator-() const noexcept
      {
          return angle{-value()};
      }

      template<std::floating_point U>
      [[nodiscard]]
      friend constexpr angle<common_fp_type_t<T, U>, Period> operator+(angle<T, Period> lhs, angle<U, Period> rhs) noexcept
      {
          using common_t = common_fp_type_t<T, U>;
          return angle<common_t>{lhs.value() + rhs.value()};
      }

      template<std::floating_point U>
      [[nodiscard]]
      friend constexpr angle<common_fp_type_t<T, U>, Period> operator-(angle<T, Period> lhs, angle<U, Period> rhs) noexcept
      {
          using common_t = common_fp_type_t<T, U>;
          return angle<common_t, Period>{lhs.value() - rhs.value()};
      }

      template<class U>
          requires is_compatible_scalar<T, U>
      constexpr angle& operator*=(U rhs) noexcept
      {
          m_Angle *= rhs;
          return *this;
      }

      template<std::floating_point U>
      [[nodiscard]]
      friend constexpr angle<common_fp_type_t<T, U>, Period> operator*(angle<T, Period> lhs, U rhs) noexcept
      {
          using common_t = common_fp_type_t<T, U>;
          return angle<common_t, Period>{lhs.value()* rhs};
      }

      template<std::floating_point U>
      [[nodiscard]]
      friend constexpr angle<common_fp_type_t<T, U>, Period> operator*(U lhs, angle<T, Period> rhs) noexcept
      {
          return rhs * lhs;
      }

      template<class U>
          requires is_compatible_scalar<T, U>
      constexpr angle& operator/=(U rhs)
      {
          m_Angle /= rhs;
          return *this;
      }

      template<std::floating_point U>
      [[nodiscard]]
      friend constexpr angle<common_fp_type_t<T, U>, Period> operator/(angle<T, Period> lhs, U rhs)
      {
          using common_t = common_fp_type_t<T, U>;
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
  using radians = angle<T, T(2)* std::numbers::pi_v<T>>;

  template<std::floating_point T>
  using degrees = angle<T, T(360)>;

  template<std::floating_point T>
  [[nodiscard]]
  constexpr radians<T> to_radians(degrees<T> theta) noexcept
  {
    return radians<T>{theta.value()* radians<T>::period / degrees<T>::period};
  }
}
