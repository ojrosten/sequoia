////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Concepts mostly, but not exclusively, replicating things which will appear in std at some point.
 */

#include <type_traits>
#include <utility>
#include <functional>
#include <concepts>

namespace sequoia
{
  template <class F, class R, class... Args>
  concept invocable_r =
    requires(F&& f, Args&&... args) {
      { std::invoke(std::forward<F>(f), std::forward<Args>(args)...) } -> std::same_as<R>;
  };

  template <class T>
  concept pseudoregular = std::copyable<T> && std::equality_comparable<T>;

  template <class T>
  concept moveonly = std::movable<T> && !std::copyable<T> && std::equality_comparable<T>;

  template <class T>
  concept movable_comparable = std::movable<T> && std::equality_comparable<T>;

  template <class A>
  concept alloc = requires(A& a) {
    a.allocate(0);
  };

  template <class A>
  concept scoped_alloc = alloc<A> && requires() {
    typename A::outer_allocator_type;
    typename A::inner_allocator_type;
  };

  template<class T>
  concept has_allocator_type = requires() {
    typename T::allocator_type;
  };

  template<class T, class Stream>
  concept serializable_to = requires(std::remove_reference_t<Stream>& stream, const std::remove_reference_t<T>& t) {
   stream << t;
  };

  template<class T, class Stream>
  concept deserializable_from = requires(std::remove_reference_t<Stream>& stream, std::remove_reference_t<T>& t) {
    stream >> t;
  };

  template<class T>
  concept range = requires(T& t) {
    std::begin(t);
    std::end(t);

    requires (!std::same_as<std::remove_cvref_t<decltype(*std::begin(t))>, std::remove_cvref_t<T>>);
  };

  template<template<class...> class T, class... Args>
  concept class_template_is_default_instantiable
   = requires() { T<Args...>{}; };

}
