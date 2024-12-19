////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Concepts which are sufficiently general to appear in the `sequoia` namespace.
 */

#include "sequoia/Core/Meta/TypeTraits.hpp"

#include <utility>
#include <functional>
#include <concepts>

namespace sequoia
{
  /// TO DO: Temporary, crude implementation while waiting for clang.
  template<class T>
  concept three_way_comparable = requires(const std::remove_reference_t<T>&t,
                                          const std::remove_reference_t<T>&u)
  {
    t <=> u;
  };

  /// \brief Supplements `std::invocable`.
  template <class F, class R, class... Args>
  concept invocable_r =
    requires(F&& f, Args&&... args) {
      { std::invoke(std::forward<F>(f), std::forward<Args>(args)...) } -> std::same_as<R>;
  };

  /// \brief Supplements `std::regular_invocable`.
  template <class F, class R, class... Args>
  concept regular_invocable_r = invocable_r<F, R, Args...>;

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

  /// \brief A concept which is realized by a `T const&` which be serialized to a `Stream&`.
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

  /*! \brief Similar to std::range but excludes the case where dereferencing yields the same type as the range.
  
      This avoids treating std::filesystem::path as a range in circumstances where, to do so, would be inappropriate.
      The implementation of `faithful_range` is not complete; it deals with the simplest circular case but
      doesn't take into account more complicated possibilites. A full treatment could almost certainly be
      readily implemented if/when reflection is properly supported in C++.
   */
  template<class T>
  concept faithful_range = requires(T& t) {
    std::ranges::begin(t);
    std::ranges::end(t);

    requires (!std::same_as<std::remove_cvref_t<decltype(*std::begin(t))>, std::remove_cvref_t<T>>);
  };

  /*! \addtogroup deep_equality
  
      @{
   */

  /*! \brief Concept to work around the fact that currently the stl typically underconstrains `operator== `. */
  template<class T>
  concept deep_equality_comparable = is_deep_equality_comparable_v<T>;

  /*! @} */
}
