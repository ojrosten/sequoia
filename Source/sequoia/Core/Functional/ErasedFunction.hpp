////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief A copyable, owning, type-erased callable which sits between `std::function` and
    `std::copyable_function`, and which may be used in a constant expression.

    From `std::function` it takes a wide contract: invoking an empty `erased_function` throws
    `std::bad_function_call`, where invoking an empty `std::copyable_function` is undefined.

    From `std::copyable_function` it takes:
    -# **The signature's qualifiers reach the call operator.** `erased_function<R(Args...) cv ref
       noexcept(noex)>` has a call operator qualified `cv ref noexcept(noex)`, and invokes its target
       as `cv T&`, or `cv T&&` for an `&&` signature. A `const` signature therefore accepts only a
       target callable as `const`, where `std::function`'s `const` call operator invokes its target as
       non-`const`.
    -# **In-place construction.**
    -# **No `target()` or `target_type()`**, which `std::function` offers.

    ## Where it also differs from `std::copyable_function`

    -# **No discarded result.** A `void` signature takes only a target returning `void`, where the
       standard type discards whatever a target returns.
    -# **No pointer to member as a target.**
    -# **No target whose destructor may throw**, where the standard makes it a precondition that none
       does.
    -# **The constructors' requirements are constraints**, where the standard mandates some of them:
       `std::is_constructible_v` is false for a target which is not copy constructible, rather than
       true and ill-formed on use.
    -# **No converting assignment.** Assigning a callable converts it and then assigns.
    -# **No `swap` of its own.** `std::ranges::swap` exchanges two functions through the defaulted
       moves; an unqualified `swap(f, g)` finds nothing.
    -# **Invoking an empty function through a `noexcept` signature terminates**, since the throw
       escapes a `noexcept` call operator.

    ## Exception guarantees

    Copy assignment is strong. Move construction and move assignment never throw.

    ## In a constant expression

    -# When compiled as C++26 or later (P2738 permits a cast from `void*` in a constant expression),
       an `erased_function` may be created, copied, moved, invoked and destroyed within a constant
       evaluation, but may not outlive one: a `constexpr` variable holding a target is ill-formed,
       since the target is an allocation.
    -# `operator bool`, comparison with `nullptr`, and construction from a function pointer cannot be
       evaluated in a constant expression by gcc with `-fsanitize=undefined` (GCC bug 71962).
 */

#include "sequoia/Core/Meta/Concepts.hpp"
#include "sequoia/Core/Meta/TypeTraits.hpp"
#include "sequoia/Core/Object/ResetOnMove.hpp"
#include "sequoia/PlatformSpecific/Macros.hpp"

#include <cstddef>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <new>
#include <type_traits>
#include <utility>

namespace sequoia
{
  namespace impl
  {
    /** \brief Owns a type-erased target: the storage it lives in, and the manager which copies,
        moves and destroys it.

        Independent of the signature through which the target is called. Empty when default
        constructed or moved from.
     */
    class erased_target
    {
    public:
      constexpr erased_target() = default;

      template<class Fn, class... FnArgs>
      SEQUOIA_FORCE_INLINE
      constexpr explicit erased_target(std::in_place_type_t<Fn>, FnArgs&&... fnArgs)
        : m_Pointer{place_or_allocate<Fn>(m_Buffer, std::forward<FnArgs>(fnArgs)...)}
        , m_SpecialMemberManager{special_member_manager_for<Fn>()}
      {}

      constexpr erased_target(const erased_target& other)
        : m_SpecialMemberManager{other.m_SpecialMemberManager}
      {
        m_SpecialMemberManager(operation::copy, *this, other);
      }

      constexpr erased_target(erased_target&& other) noexcept
        : m_SpecialMemberManager{std::exchange(other.m_SpecialMemberManager, &manage_empty)}
      {
        m_SpecialMemberManager(operation::move, *this, other);
      }

      /** Gives the strong exception guarantee. */
      constexpr erased_target& operator=(const erased_target& other)
      {
        return *this = erased_target{other};
      }

      constexpr erased_target& operator=(erased_target&& other) noexcept
      {
        if(&other != this)
        {
          m_SpecialMemberManager(operation::destroy, *this, *this);
          m_SpecialMemberManager = std::exchange(other.m_SpecialMemberManager, &manage_empty);
          m_SpecialMemberManager(operation::move, *this, other);
        }

        return *this;
      }

      constexpr ~erased_target() { m_SpecialMemberManager(operation::destroy, *this, *this); }

      /** The caller of a target of type `Fn`, invoked as `Invoked` with `Args`, returning `R`.

          A named function rather than a lambda: gcc cannot call a lambda through a function pointer
          in a constant evaluation when the result is a class type with a non-trivial destructor
          (GCC bug 125000, a regression since 14).
       */
      template<class Fn, class Invoked, class R, class... Args>
      constexpr static R call(const erased_target& erased, Args&&... args)
      {
        // A target is never itself const, so casting away the `const` on the way to it is defined
        auto* const target{const_cast<Fn*>(target_held_by<Fn>(erased))};
        return static_cast<Invoked>(*target)(std::forward<Args>(args)...);
      }

      /** The caller of an empty target, through any signature. */
      template<class R, class... Args>
      constexpr static R call_empty(const erased_target&, Args&&...) { throw std::bad_function_call{}; }
    private:
      constexpr static std::size_t buffer_size{3 * sizeof(void*)};

      template<class Fn>
      constexpr static bool fits_in_buffer_v{(sizeof(Fn) <= buffer_size) && (alignof(Fn) <= alignof(std::max_align_t))};

      template<class Fn>
      constexpr static bool holdable_in_buffer_v{fits_in_buffer_v<Fn> && std::is_nothrow_move_constructible_v<Fn>};

      template<class Fn>
      constexpr static bool trivially_managed_v{holdable_in_buffer_v<Fn> && std::is_trivially_copyable_v<Fn>};

      enum class operation { copy, move, destroy };

      using special_member_manager_type = void (*)(operation, erased_target&, const erased_target&);

      /** Held by an empty `erased_target` in place of a null pointer, so that no manager is tested for
          null before it is called: gcc with `-fsanitize=undefined` cannot evaluate such a test in a
          constant expression (GCC bug 71962).
       */
      constexpr static void manage_empty(operation, erased_target&, const erased_target&) noexcept {}

      static void manage_trivial(operation op, erased_target& to, const erased_target& from)
      {
        // The byte copy creates the target in `to`, a trivially copyable type being implicit-lifetime
        if(op != operation::destroy)
          std::memcpy(to.m_Buffer, from.m_Buffer, buffer_size);
      }

      /** The manager of a target which is not trivially managed, and of every target in a constant
          evaluation.
       */
      template<class Fn>
      constexpr static void manage_general(operation op, erased_target& to, const erased_target& from)
      {
        switch(op)
        {
        case operation::copy:
          to.m_Pointer = place_or_allocate<Fn>(to.m_Buffer, *target_held_by<Fn>(from));
          break;
        case operation::move:
          relocate<Fn>(to, from);
          break;
        case operation::destroy:
          destroy_target<Fn>(from);
          break;
        }
      }

      template<class Fn>
      SEQUOIA_FORCE_INLINE
      constexpr static special_member_manager_type special_member_manager_for()
      {
        if consteval
        {
          return &manage_general<Fn>;
        }
        else
        {
          if constexpr(trivially_managed_v<Fn>)
            return &manage_trivial;
          else
            return &manage_general<Fn>;
        }
      }

      template<class Fn>
      SEQUOIA_FORCE_INLINE
      constexpr static const Fn* target_held_by(const erased_target& erased)
      {
        if consteval
        {
          return static_cast<const Fn*>(erased.m_Pointer);
        }
        else
        {
          if constexpr(holdable_in_buffer_v<Fn>)
            return std::launder(reinterpret_cast<const Fn*>(erased.m_Buffer));
          else
            return static_cast<const Fn*>(erased.m_Pointer);
        }
      }

      /** Constructs a target in the buffer if it is holdable there, and otherwise allocates it,
          initializing it with parentheses, as the standard specifies for the in-place constructors.

          \returns the allocation, or null if the target was placed in the buffer
       */
      template<class Fn, class... Args>
      SEQUOIA_FORCE_INLINE
      constexpr static void* place_or_allocate(std::byte* buffer, Args&&... args)
      {
        if consteval
        {
          return new Fn(std::forward<Args>(args)...);
        }
        else
        {
          if constexpr(holdable_in_buffer_v<Fn>)
          {
            ::new (static_cast<void*>(buffer)) Fn(std::forward<Args>(args)...);
            return nullptr;
          }
          else
            return new Fn(std::forward<Args>(args)...);
        }
      }

      template<class Fn>
      SEQUOIA_FORCE_INLINE
      constexpr static void relocate(erased_target& to, const erased_target& from)
      {
        if consteval
        {
          to.m_Pointer = from.m_Pointer;
        }
        else
        {
          if constexpr(holdable_in_buffer_v<Fn>)
          {
            auto* const target{const_cast<Fn*>(target_held_by<Fn>(from))};
            to.m_Pointer = place_or_allocate<Fn>(to.m_Buffer, std::move(*target));
            target->~Fn();
          }
          else
            to.m_Pointer = from.m_Pointer;
        }
      }

      template<class Fn>
      SEQUOIA_FORCE_INLINE
      constexpr static void destroy_target(const erased_target& erased)
      {
        if consteval
        {
          delete target_held_by<Fn>(erased);
        }
        else
        {
          if constexpr(holdable_in_buffer_v<Fn>)
            target_held_by<Fn>(erased)->~Fn();
          else
            delete target_held_by<Fn>(erased);
        }
      }

      /** A target is held in the buffer if it is holdable there; otherwise, and always in a constant
          evaluation, the target is allocated and held through the pointer.

          Two members, not a union. Only a live `std::byte` array provides storage for an object
          constructed in it; in a union, the buffer is dead whenever the pointer is the active member,
          so a target constructed in that dead buffer would end the lifetime of the object holding it.
       */
      alignas(std::max_align_t) std::byte m_Buffer[buffer_size]{};
      void*                               m_Pointer{};
      special_member_manager_type         m_SpecialMemberManager{&manage_empty};
    };
  }

  namespace impl
  {
    enum class call_constness { non_const, const_qualified };

    enum class call_reference { none, lvalue, rvalue };

    /** \brief The call operator of an `erased_function`, qualified as its signature is. */
    template<call_constness Constness, call_reference Reference, bool Noexcept, class R, class... Args>
    class call_operator
    {
      template<class T>
      using cv_qualified = std::conditional_t<Constness == call_constness::const_qualified, const T, T>;

      /** The object type the signature's qualifiers describe: `cv T ref`. */
      template<class T>
      using qualified_object
        = std::conditional_t<Reference == call_reference::none,
                             cv_qualified<T>,
                             std::conditional_t<Reference == call_reference::lvalue,
                                                cv_qualified<T>&,
                                                cv_qualified<T>&&>>;

      /** The type as which a target is invoked: `cv T&` unless the signature is `&&`-qualified. */
      template<class T>
      using invoked_as = std::conditional_t<Reference == call_reference::rvalue, cv_qualified<T>&&, cv_qualified<T>&>;

      template<class F>
      constexpr static bool invocable_as_signature_v{
        invocable_r<F, R, Args...> && (!Noexcept || std::is_nothrow_invocable_r_v<R, F, Args...>)
      };

      /** Whether a member function with these qualifiers may be called on an object expression of
          type `Self&&`.
       */
      template<class Self>
      constexpr static bool admits_v{
           (!std::is_const_v<std::remove_reference_t<Self>> || (Constness == call_constness::const_qualified))
        && (   (Reference == call_reference::none)
            || ((Reference == call_reference::lvalue)
                  && (std::is_lvalue_reference_v<Self> || (Constness == call_constness::const_qualified)))
            || ((Reference == call_reference::rvalue) && !std::is_lvalue_reference_v<Self>))
      };
    public:
      /** Whether a target of type `T` can be invoked with `Args` for a result convertible to `R`, with
          the signature's `const` and reference qualifiers preserved, and without throwing if the
          signature is `noexcept`.
       */
      template<class T>
      constexpr static bool callable_through_signature_v{
        invocable_as_signature_v<qualified_object<T>> && invocable_as_signature_v<invoked_as<T>>
      };

      template<class Self>
        requires admits_v<Self>
      SEQUOIA_FORCE_INLINE
      constexpr R operator()(this Self&& self, Args... args) noexcept(Noexcept)
      {
        return self.m_Caller.value()(self.m_Target, std::forward<Args>(args)...);
      }
    protected:
      constexpr call_operator() = default;

      constexpr call_operator(const call_operator&) = default;

      constexpr call_operator(call_operator&&) = default;

      constexpr call_operator& operator=(const call_operator&) = default;

      constexpr call_operator& operator=(call_operator&&) = default;

      constexpr ~call_operator() = default;

      using caller_type = R (*)(const erased_target&, Args&&...);

      template<class Fn>
      consteval static caller_type caller_for() { return &erased_target::call<Fn, invoked_as<Fn>, R, Args...>; }

      consteval static caller_type caller_for_empty() { return &erased_target::call_empty<R, Args...>; }
    };

    template<class Signature>
    struct call_operator_for
    {};

    template<class R, class... Args, bool Noexcept>
    struct call_operator_for<R(Args...) noexcept(Noexcept)>
    {
      using type = call_operator<call_constness::non_const, call_reference::none, Noexcept, R, Args...>;
    };

    template<class R, class... Args, bool Noexcept>
    struct call_operator_for<R(Args...) const noexcept(Noexcept)>
    {
      using type = call_operator<call_constness::const_qualified, call_reference::none, Noexcept, R, Args...>;
    };

    template<class R, class... Args, bool Noexcept>
    struct call_operator_for<R(Args...) & noexcept(Noexcept)>
    {
      using type = call_operator<call_constness::non_const, call_reference::lvalue, Noexcept, R, Args...>;
    };

    template<class R, class... Args, bool Noexcept>
    struct call_operator_for<R(Args...) const & noexcept(Noexcept)>
    {
      using type = call_operator<call_constness::const_qualified, call_reference::lvalue, Noexcept, R, Args...>;
    };

    template<class R, class... Args, bool Noexcept>
    struct call_operator_for<R(Args...) && noexcept(Noexcept)>
    {
      using type = call_operator<call_constness::non_const, call_reference::rvalue, Noexcept, R, Args...>;
    };

    template<class R, class... Args, bool Noexcept>
    struct call_operator_for<R(Args...) const && noexcept(Noexcept)>
    {
      using type = call_operator<call_constness::const_qualified, call_reference::rvalue, Noexcept, R, Args...>;
    };

    template<class Signature>
    using call_operator_for_t = call_operator_for<Signature>::type;
  }

  /** \brief A signature `erased_function` supports: `R(Args...) cv ref noexcept(noex)`, where `cv` is
      either empty or `const`, and `ref` is empty, `&` or `&&`.
   */
  template<class Signature>
  concept erasable_signature = requires { typename impl::call_operator_for_t<Signature>; };

  template<class Signature>
    requires erasable_signature<Signature>
  class erased_function;

  namespace impl
  {
    template<class T>
    constexpr bool is_erased_function_v{false};

    template<class Signature>
    constexpr bool is_erased_function_v<erased_function<Signature>>{true};

    /** Whether a target leaves the function empty, as the standard specifies: a null function pointer,
        or an empty `erased_function`.
     */
    template<class T>
    SEQUOIA_FORCE_INLINE
    constexpr bool is_empty_target(const T& target)
    {
      if constexpr(std::is_pointer_v<T> && std::is_function_v<std::remove_pointer_t<T>>)
        return target == nullptr;
      else if constexpr(is_erased_function_v<T>)
        return !target;
      else
        return false;
    }
  }

  /** \brief Owning type erasure for a callable, with the call operator qualified as `Signature` is. */
  template<class Signature>
    requires erasable_signature<Signature>
  class erased_function : public impl::call_operator_for_t<Signature>
  {
    using call_operator_type = impl::call_operator_for_t<Signature>;
    friend call_operator_type;

    using caller_type = call_operator_type::caller_type;

    template<class T>
    consteval static caller_type caller_for() { return call_operator_type::template caller_for<T>(); }

    /** A function rather than a conditional expression. C++17 requires a prvalue conditional expression
        to initialise an object directly, but MSVC (19.40 to 19.44 at least) materialises one whose class
        type has a user-provided destructor and moves from it, which moves the target once more.
     */
    template<class Target, class F>
    SEQUOIA_FORCE_INLINE
    constexpr static impl::erased_target erased_target_for(F&& f)
    {
      if(impl::is_empty_target(f))
        return {};

      return impl::erased_target{std::in_place_type_t<Target>{}, std::forward<F>(f)};
    }

    template<class Target, class F>
    SEQUOIA_FORCE_INLINE
    constexpr erased_function(caller_type caller, std::in_place_type_t<Target>, F&& f)
      : m_Target{erased_target_for<Target>(std::forward<F>(f))}
      , m_Caller{caller}
    {}
  public:
    template<class T, class... TArgs>
    constexpr static bool target_constructible_from_v{
         std::is_same_v<T, std::decay_t<T>>
      && (!std::is_member_pointer_v<T>)
      && std::is_nothrow_destructible_v<T>
      && std::is_copy_constructible_v<T>
      && std::is_constructible_v<T, TArgs...>
      && call_operator_type::template callable_through_signature_v<T>
    };

    constexpr erased_function() = default;

    constexpr erased_function(std::nullptr_t) noexcept
      : erased_function{}
    {}

    template<class F, class Target = std::decay_t<F>>
      requires (!resolve_to_copy_v<erased_function, F>) && target_constructible_from_v<Target, F>
    constexpr erased_function(F&& f)
      : erased_function{
          impl::is_empty_target(f) ? empty_caller : caller_for<Target>(),
          std::in_place_type_t<Target>{},
          std::forward<F>(f)
        }
    {}

    template<class T, class... TArgs>
      requires target_constructible_from_v<T, TArgs...>
    constexpr explicit erased_function(std::in_place_type_t<T>, TArgs&&... args)
      : m_Target{std::in_place_type_t<T>{}, std::forward<TArgs>(args)...}
      , m_Caller{caller_for<T>()}
    {}

    template<class T, class U, class... TArgs>
      requires target_constructible_from_v<T, std::initializer_list<U>&, TArgs...>
    constexpr explicit erased_function(std::in_place_type_t<T>, std::initializer_list<U> list, TArgs&&... args)
      : m_Target{std::in_place_type_t<T>{}, list, std::forward<TArgs>(args)...}
      , m_Caller{caller_for<T>()}
    {}

    constexpr erased_function& operator=(std::nullptr_t) noexcept
    {
      return *this = erased_function{};
    }

    [[nodiscard]]
    constexpr explicit operator bool() const noexcept { return m_Caller.value() != empty_caller; }

    [[nodiscard]]
    friend constexpr bool operator==(const erased_function& f, std::nullptr_t) noexcept { return !f; }
  private:
    constexpr static caller_type empty_caller{call_operator_type::caller_for_empty()};

    // Declared first: the strong guarantee of copy assignment depends on it
    impl::erased_target                              m_Target;
    object::reset_on_move<caller_type, empty_caller> m_Caller;
  };
}
