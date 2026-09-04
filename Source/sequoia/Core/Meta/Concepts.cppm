////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

export module sequoia.core.meta:Concepts;

import std;

import :TypeTraits;

/** \file
    \brief Concepts which are sufficiently general to appear in the `sequoia` namespace.
 */

export namespace sequoia
{
  /** \brief Supplements `std::invocable`, such that the return type is specified.

      As with `std::is_invocable_r`, the return type must be convertible to
      `R`, not necessarily identical to it.
   */
  template <class F, class R, class... Args>
  concept invocable_r =
    requires(F&& f, Args&&... args) {
      { std::invoke(std::forward<F>(f), std::forward<Args>(args)...) } -> std::convertible_to<R>;
  };

  /// \brief Supplements `std::regular_invocable`.
  template <class F, class R, class... Args>
  concept regular_invocable_r = invocable_r<F, R, Args...>;

  /** \brief As `invocable_r`, but where the return type is precisely `R`.

      Deliberately not defined as a conjunction with `invocable_r`: doing so
      inherits a demand for convertibility to `R`, and that fails for an `R`
      having neither a copy nor a move constructor - even though such an `R`
      may be returned by value.
   */
  template <class F, class R, class... Args>
  concept invocable_exact_r =
    requires(F&& f, Args&&... args) {
      { std::invoke(std::forward<F>(f), std::forward<Args>(args)...) } -> std::same_as<R>;
  };

  /// \brief As `regular_invocable_r`, but where the return type is precisely `R`.
  template <class F, class R, class... Args>
  concept regular_invocable_exact_r = invocable_exact_r<F, R, Args...>;

  /// \brief Building block for concepts related to `std::regular` but without the requirement of default constructibility.
  template <class T>
  concept movable_comparable = std::movable<T> && std::equality_comparable<T>;

  /// \brief Similar to std::regular but relaxes the requirement of default initializability.
  template <class T>
  concept pseudoregular = movable_comparable<T> && std::copyable<T>;

  /// \brief The move-only version of sequoia::pseudoregular.
  template <class T>
  concept moveonly = movable_comparable<T> && !std::copyable<T>;

  /// \brief A concept for allocators
  template <class A>
  concept alloc = requires(A& a) {
    a.allocate(0);
  };

  /// \brief A concept for scoped allocators
  template <class A>
  concept scoped_alloc = alloc<A> && requires() {
    typename A::outer_allocator_type;
    typename A::inner_allocator_type;
  };

  /// \brief A concept which is realized by a `T const&` which may be serialized to a `Stream&`.
  template<class T, class Stream>
  concept serializable_to = requires(std::remove_reference_t<Stream>& stream, const std::remove_reference_t<T>& t) {
    typename Stream::char_type;
    stream << t;
  };

  /// \brief A concept which is realized by a `Stream&` which may be deserialized to a `T&`.
  template<class T, class Stream>
  concept deserializable_from = requires(std::remove_reference_t<Stream>& stream, std::remove_reference_t<T>& t) {
    typename Stream::char_type;
    stream >> t;
  };

  /// \brief A concept similar to std::constructible_from, but which considers braced-init
  template<class T, class... Args>
  concept initializable_from = requires{
    T{std::declval<Args>()...};
  };

  /// \brief A concept for arithmetic types
  template<class T>
  concept arithmetic = std::is_arithmetic_v<T>;

  /** \brief A concept for the integer types, as distinct from the integral
             types.

      The standard separates the two: bool, char, wchar_t, char8_t, char16_t
      and char32_t are integral but not integer types, the integer types being
      the signed and unsigned ones, standard and extended. `std::in_range` and
      the `std::cmp_*` family mandate integer types on both sides
      ([utility.intcmp]), and this draws the same line. Note which side the
      narrow types fall: `signed char` and `unsigned char` are integer types,
      so `std::int8_t`, which is `signed char`, satisfies the concept - the
      standard gives 8-bit arithmetic no other spelling. Cv-qualified forms
      follow their unqualified type, as they do for `std::integral` - which
      sees through cv where `std::same_as` does not.
   */
  namespace impl
  {
    template<class T>
    inline constexpr bool bool_or_character_type_v{
         std::same_as<T, bool>     || std::same_as<T, char>
      || std::same_as<T, wchar_t>  || std::same_as<T, char8_t>
      || std::same_as<T, char16_t> || std::same_as<T, char32_t>
    };
  }

  template<class T>
  concept integer = std::integral<T> && (!impl::bool_or_character_type_v<std::remove_cv_t<T>>);

  /** \brief Similar to std::range but excludes the case where dereferencing yields the same type as the range.
  
      This avoids treating std::filesystem::path as a range in circumstances where, to do so, would be inappropriate.
      The implementation of `faithful_range` is not complete; it deals with the simplest circular case but
      doesn't take into account more complicated possibilites. A full treatment could almost certainly be
      readily implemented if/when reflection is properly supported in C++.
   */
  template<class T>
  concept faithful_range = requires(T& t) {
    std::ranges::begin(t);
    std::ranges::end(t);

    requires (!std::same_as<std::remove_cvref_t<decltype(*std::ranges::begin(t))>, std::remove_cvref_t<T>>);
  };

  /** \addtogroup deep_equality
  
      @{
   */

  /** \brief Concept to work around the fact that currently the stl typically underconstrains `operator== `. */
  template<class T>
  concept deep_equality_comparable = is_deep_equality_comparable_v<T>;

  /** @} */

  template<class T>
  concept deep_totally_ordered = deep_equality_comparable<T> && is_deep_totally_ordered_v<T>;

}
