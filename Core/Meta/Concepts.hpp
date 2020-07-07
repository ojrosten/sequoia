////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Concepts mostly, but not exclusively, replicating things which will appear in std at some point.
 */

#include "Algorithms.hpp"

#include <type_traits>
#include <utility>

namespace sequoia
{
  template <class T, class U>
  concept same_as = std::is_same_v<T, U>;
  
  template <class T>
  concept destructible = std::is_nothrow_destructible_v<T>;

  template <class T, class... Args>
  concept constructible_from =
    destructible<T> && std::is_constructible_v<T, Args...>;

  template <class From, class To>
  concept convertible_to =
       std::is_convertible_v<From, To>
    && requires(std::add_rvalue_reference_t<From> (&f)()) { static_cast<To>(f()); };

  /*template < class T, class U >
  concept common_reference_with =
       same_as<std::common_reference_t<T, U>, std::common_reference_t<U, T>>
    && convertible_to<T, std::common_reference_t<T, U>>
    && convertible_to<U, std::common_reference_t<T, U>>;
  */

  template<class LHS, class RHS>
  concept assignable_from =
       std::is_lvalue_reference_v<LHS>
    /*&& common_reference_with<
         const std::remove_reference_t<LHS>&,
         const std::remove_reference_t<RHS>&
         >*/
    && requires(LHS lhs, RHS&& rhs) {
         { lhs = std::forward<RHS>(rhs) } -> same_as<LHS>;
       };
  
  template<class T>
  concept move_constructible =
    constructible_from<T, T> && convertible_to<T, T>;

  template< class T >
  concept swappable =
    requires(T& a, T& b) {
      sequoia::swap(a, b);
    };
  
  template <class T>
  concept movable =
       std::is_object_v<T>
    && move_constructible<T>
    && assignable_from<T&, T>
    && swappable<T>;

  template <class T>
  concept copy_constructible =
       move_constructible<T>
    && constructible_from<T, T&>       && convertible_to<T&, T>
    && constructible_from<T, const T&> && convertible_to<const T&, T>
    && constructible_from<T, const T>  && convertible_to<const T, T>;
  
  template <class T>
  concept copyable =
       copy_constructible<T>
    && movable<T>
    && assignable_from<T&, T&>
    && assignable_from<T&, const T&>
    && assignable_from<T&, const T>;

  template<class T>
  concept default_initializable =
       constructible_from<T>
    && requires { T{}; }
    && requires { ::new (static_cast<void*>(nullptr)) T; };

  template<class B>
  concept boolean =
       movable<std::remove_cvref_t<B>>
    && requires(const std::remove_reference_t<B>& b1,
                const std::remove_reference_t<B>& b2, const bool a) {
      { b1 }       -> convertible_to<bool>;
      { !b1 }      -> convertible_to<bool>;
      { b1 && b2 } -> same_as<bool>;
      { b1 &&  a } -> same_as<bool>;
      {  a && b2 } -> same_as<bool>;
      { b1 || b2 } -> same_as<bool>;
      { b1 ||  a } -> same_as<bool>;
      {  a || b2 } -> same_as<bool>;
      { b1 == b2 } -> convertible_to<bool>;
      { b1 ==  a } -> convertible_to<bool>;
      {  a == b2 } -> convertible_to<bool>;
      { b1 != b2 } -> convertible_to<bool>;
      { b1 !=  a } -> convertible_to<bool>;
      {  a != b2 } -> convertible_to<bool>;
    };

  template<class T, class U>
  concept weak_equality_comparable_with =
    requires(const std::remove_reference_t<T>& t,
             const std::remove_reference_t<U>& u) {
      { t == u } -> boolean;
      { t != u } -> boolean;
      { u == t } -> boolean;
      { u != t } -> boolean;
    };

  template <class T>
  concept equality_comparable = weak_equality_comparable_with<T, T>;
  
  template <class T>
  concept semiregular = copyable<T> && default_initializable<T>;

  template <class T>
  concept regular = semiregular<T> && equality_comparable<T>;

  template <class T>
  concept pseudoregular = copyable<T> && equality_comparable<T>;

  template <class T>
  concept moveonly = movable<T> && !copyable<T> && equality_comparable<T>;

  template <class T>
  concept stronglymovable = movable<T> && equality_comparable<T>;

  template <class F, class... Args>
  concept invocable =
    requires(F&& f, Args&&... args) {
      std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
  };

  template <class A>
  concept allocator_c = is_allocator_v<A>;  
}
