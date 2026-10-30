////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

export module sequoia.test_framework:CoreInfrastructure;

import std;

export import sequoia.core.meta;

/** \file
    \brief Core declarations / definitions used in the testing framework

 */

export namespace sequoia::testing
{
  /** \brief Specialize this struct template to provide custom serialization of a given class.
      \anchor serializer_primary
   */

  template<class T>
  struct serializer;

  template<std::formattable<char> T>
  struct serializer<T>
  {
    [[nodiscard]]
    static std::string make(const T& val)
    {
      return std::format("{}", val);
    }
  };

  template<serializable_to<std::stringstream> T>
    requires (!std::formattable<T, char>)
  struct serializer<T>
  {
    [[nodiscard]]
    static std::string make(const T& val)
    {
      std::ostringstream os{};
      os << std::boolalpha << val;
      return os.str();
    }
  };

  /** \brief Serializes a `std::chrono::duration` as its count followed by its
             units suffix.

      According to the C++ standard:
      -# Streaming a duration streams its count.
      -# Formatting a duration without a format specification streams the
         duration.

      Therefore, a duration whose count cannot be streamed can be neither
      streamed nor formatted without a format specification, even when the
      `std::formattable` concept is satisfied.

      libc++'s `std::filesystem::file_time_type` counts in an `__int128`,
      which libc++ cannot stream.
   */
  template<class Rep, class Period>
    requires (!serializable_to<Rep, std::stringstream>) && std::formattable<Rep, char>
  struct serializer<std::chrono::duration<Rep, Period>>
  {
    [[nodiscard]]
    static std::string make(const std::chrono::duration<Rep, Period>& d)
    {
      // The units suffix depends on the period alone
      return std::format("{}{:%q}", d.count(), std::chrono::duration<long long, Period>{});
    }
  };

  template<class T>
  concept serializable = requires(serializer<T>& s, T& t) {
    s.make(t);
  };

  template<serializable T>
  [[nodiscard]]
  std::string to_string(const T& value)
  {
    return serializer<T>::make(value);
  }

  /** \brief Serializes a `std::chrono::time_point` which can be neither
             formatted nor streamed, as its time since its clock's epoch.

      The C++ standard library formats and streams the time points of most of
      its clocks, such as those of `std::chrono::system_clock` and
      `std::chrono::file_clock`, but not those of `std::chrono::steady_clock`.
      `std::chrono::high_resolution_clock` may be an alias of either, so
      whether this serializer prints its time points depends on the library.

      A standard time point which `std::formattable` accepts but which its
      formatter cannot print, such as a `std::chrono::sys_time` with a
      floating-point count, is claimed by the serializer for formattable
      types, whose `make` then fails to compile.
   */
  template<class Clock, class Duration>
    requires    (!std::formattable<std::chrono::time_point<Clock, Duration>, char>)
             && (!serializable_to<std::chrono::time_point<Clock, Duration>, std::stringstream>)
             && serializable<Duration>
  struct serializer<std::chrono::time_point<Clock, Duration>>
  {
    [[nodiscard]]
    static std::string make(const std::chrono::time_point<Clock, Duration>& t)
    {
      return to_string(t.time_since_epoch()).append(" since epoch");
    }
  };

  /** \brief Primary class template mapping a type to itself.

      The specializations map an unsigned integer type of 32 or 64 bits to the fixed-width type of its size. A
      `character` type is left as it is.
   */
  template<class T>
  struct type_normalizer
  {
    using type = T;
  };

  template<class T>
    requires (integer<T> && std::is_unsigned_v<T> && (sizeof(T) == sizeof(std::uint64_t)))
  struct type_normalizer<T>
  {
    using type = std::uint64_t;
  };

  template<class T>
    requires (integer<T> && std::is_unsigned_v<T> && (sizeof(T) == sizeof(std::uint32_t)))
  struct type_normalizer<T>
  {
    using type = std::uint32_t;
  };

  template<class T>
  using type_normalizer_t = type_normalizer<T>::type;

  template<std::integral T>
  [[nodiscard]]
  auto fixed_width_unsigned_cast(T x) noexcept
  {
    using U = std::make_unsigned_t<T>;

    return static_cast<type_normalizer_t<U>>(x);
  }

  struct top_level_check_exit_info
  {
    bool via_exception{};
    std::string message{};
  };

  using opt_top_level_check_exit_info = std::optional<top_level_check_exit_info>;
}
