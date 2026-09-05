////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief A small owning type-erased callable, standing in for `std::copyable_function`.

    `std::copyable_function` is the right tool and is not yet available: libstdc++ has it, libc++
    23.1.0 has neither it nor `std::move_only_function`, and MSVC's C++26 support is further behind
    still. This is a deliberately narrow stand-in, to be **retired** in favour of the standard type
    once that is available on all three, at which point every use site should continue to compile
    unchanged.

    The reason not to reach for `std::function` instead is measured rather than assumed. Erasing a
    callable with `std::function` instantiates `std::__function::__func<Fn, std::allocator<Fn>,
    Signature>`, a class whose several members each carry the closure type *and* the allocator in
    their mangled names; across a test suite which erases a callable per tested type that dominated
    the symbol table. Erasing through a pair of function pointers instead - a caller and a manager
    taking copy/move/destroy as an argument - contributes **one** symbol per erased type rather than
    eight, with no allocator in any of them: the caller is all that must know the target's type,
    because the manager is shared by every target which can be managed without knowing it, and the
    two pointers are held in the object rather than in a per-target table.

    On `TestAll` under asan, against the version which gave every target its own manager and reached
    them through a table: **593.5 MB -> 569.4 MB and 69,900 fewer symbols**, of which the shared
    manager is 17.7 MB and dropping the table 6.4 MB. Of 12,844 erased types in that suite only 287
    need a manager of their own.

    The design of the standard facility, and in particular the decision to hold the manager as one
    function taking an operation rather than one function per operation, follows libstdc++'s
    implementation of `std::move_only_function` and `std::copyable_function`
    (`bits/funcwrap.h`, GNU ISO C++ Library, GPL-3 with the GCC Runtime Library Exception). No code
    is copied from it; sequoia's version is much smaller because it needs much less.

    ## What is deliberately absent

    - **Only const-qualified signatures.** `copyable_function<R(Args...) const>` is the whole
      interface; the unqualified, `&`, `&&` and `noexcept` forms are not supported, because nothing
      here needs them. `std::copyable_function` accepts all of them, so this restriction is in the
      direction that keeps retirement a one-line alias.
    - **No allocator, no `target()`, no `target_type()`.** These are the parts of `std::function`
      whose cost this class exists to avoid.
 */

#include "sequoia/Core/Meta/TypeTraits.hpp"

#include <cstddef>
#include <cstring>
#include <new>
#include <type_traits>
#include <utility>

namespace sequoia::object
{
  template<class Signature>
  class copyable_function;

  template<class R, class... Args>
  class copyable_function<R(Args...)>
  {
    static_assert(dependent_false<R>::value,
                  "copyable_function supports only const-qualified signatures: write R(Args...) const");
  };

  /** \brief Owning type erasure for a callable invocable on a const object.

      \tparam R the result type
      \tparam Args the argument types
   */
  template<class R, class... Args>
  class copyable_function<R(Args...) const>
  {
  public:
    using result_type = R;

    copyable_function() = default;

    /** The constraint is `std::is_invocable_r_v` rather than sequoia's `invocable_r`, and the
        difference is load-bearing: `invocable_r` demands `std::same_as<..., R>`, whereas a wrapper
        must accept a target whose result is merely *convertible* to `R`. A generator returning
        `const T&` for a signature of `T()` is exactly that case, and `invocable_r` would reject it.
        A `void` signature is the same rule at its limit, admitting any result and discarding it.

        `resolve_to_copy_v` excludes the wrapper's own type, as the standard's specification of this
        constructor does. A `copyable_function` is itself a valid target, so without it this
        template would be a candidate for copying and moving one; the non-template constructors win
        that tiebreaker regardless, which is why removing the clause breaks no test - it states the
        intent rather than repairing an overload.
     */
    template<class Fn>
      requires (!resolve_to_copy_v<copyable_function, Fn>) && std::is_invocable_r_v<R, const Fn&, Args...>
    copyable_function(Fn fn)
      : m_Call{caller_for<Fn>}, m_Manage{manager_for<Fn>()}
    {
      if constexpr(fits<Fn>) ::new (static_cast<void*>(m_Buffer)) Fn{std::move(fn)};
      else                   *reinterpret_cast<Fn**>(m_Buffer) = new Fn{std::move(fn)};
    }

    copyable_function(const copyable_function& other) : m_Call{other.m_Call}, m_Manage{other.m_Manage}
    {
      if(m_Manage) m_Manage(op::copy, m_Buffer, other.m_Buffer);
    }

    copyable_function(copyable_function&& other) noexcept : m_Call{other.m_Call}, m_Manage{other.m_Manage}
    {
      if(m_Manage) m_Manage(op::move, m_Buffer, other.m_Buffer);
      other.m_Call   = nullptr;
      other.m_Manage = nullptr;
    }

    copyable_function& operator=(copyable_function other) noexcept
    {
      swap(*this, other);
      return *this;
    }

    ~copyable_function() { if(m_Manage) m_Manage(op::destroy, m_Buffer, m_Buffer); }

    R operator()(Args... args) const
    {
      return m_Call(m_Buffer, std::forward<Args>(args)...);
    }

    [[nodiscard]]
    explicit operator bool() const noexcept { return m_Call != nullptr; }

    /** `op::move` hands ownership on exactly once, and the thunk pointers - which are what decide
        whether a buffer is ever destroyed - are exchanged last, so each of the three moves below is
        safe.

        What `op::move` leaves behind differs by manager, and deliberately: the general one destroys
        a small source and leaves a large one's pointer in place, while the shared one leaves the
        source's bytes alone. Both are correct because the caller always overwrites or abandons what
        it moved from, and because a trivially managed target has nothing to destroy.
     */
    friend void swap(copyable_function& lhs, copyable_function& rhs) noexcept
    {
      // Without this, a self-swap moves the target out of the buffer and then straight back out of
      // the buffer it has just vacated.
      if(&lhs == &rhs) return;

      alignas(std::max_align_t) std::byte tmp[buffer_size];
      const auto lm{lhs.m_Manage}, rm{rhs.m_Manage};
      if(lm) lm(op::move, tmp, lhs.m_Buffer);
      if(rm) rm(op::move, lhs.m_Buffer, rhs.m_Buffer);
      if(lm) lm(op::move, rhs.m_Buffer, tmp);
      std::swap(lhs.m_Call,   rhs.m_Call);
      std::swap(lhs.m_Manage, rhs.m_Manage);
    }
  private:
    constexpr static std::size_t buffer_size{3 * sizeof(void*)};

    /** Three pointers is ample for what the tests present - a closure capturing one or two
        references - and anything larger goes on the heap.
     */
    template<class Fn>
    constexpr static bool fits{   (sizeof(Fn) <= buffer_size)
                               && (alignof(Fn) <= alignof(std::max_align_t))
                               && std::is_nothrow_move_constructible_v<Fn>};

    /** A target which is trivially copyable and lives in the buffer can be managed **without
        knowing its type**: copy and move are a byte copy, and destruction is nothing. Such targets
        can therefore share a single manager rather than instantiating one apiece, which matters
        because the closures erased here are overwhelmingly of this kind - a lambda capturing one
        or two references is trivially copyable.

        Trivial copyability subsumes trivial destructibility, so it is the only condition needed
        beyond fitting.
     */
    template<class Fn>
    constexpr static bool trivially_managed{fits<Fn> && std::is_trivially_copyable_v<Fn>};

    enum class op { copy, move, destroy };

    /** One manager taking the operation as an argument, rather than one function per operation.

        A function per operation contributes four entities per erased type, each carrying the
        closure type in its name; collapsing them into one was worth more than the original swap
        away from `std::function`, which is why the operation is a runtime argument.

        The two thunks are held **directly**, rather than behind a pointer to a per-target table.
        Such a table is itself an entity carrying `Fn` in its name, so it costs a long symbol per
        erased type; the price of removing it is one extra pointer per `copyable_function`, which
        for a suite erasing a callable per tested type is the better side of the trade.
     */
    using call_thunk    = R    (*)(const std::byte*, Args&&...);
    using manage_thunk  = void (*)(op, std::byte*, const std::byte*);

    /** The manager shared by every trivially-managed target, and so **not** a template: one symbol
        per signature rather than one per erased type.

        Copying the whole buffer rather than `sizeof(Fn)` bytes is what makes the sharing possible,
        and it is reading and writing within a `std::byte` array which is always fully initialized,
        never past the end of an object.
     */
    static void manage_trivially(op o, std::byte* to, const std::byte* from)
    {
      if(o != op::destroy) std::memcpy(to, from, buffer_size);
    }

    /** Chosen with `if constexpr` rather than a ternary, so that the general manager is instantiated
        only for the targets which actually need it - a ternary would odr-use both operands and
        defeat the sharing entirely. `consteval` guarantees this function is itself never emitted.
     */
    template<class Fn>
    consteval static manage_thunk manager_for()
    {
      if constexpr(trivially_managed<Fn>) return &manage_trivially;
      else return +[](op o, std::byte* to, const std::byte* from) {
        switch(o)
        {
        case op::copy:
          if constexpr(fits<Fn>) ::new (static_cast<void*>(to)) Fn{*reinterpret_cast<const Fn*>(from)};
          else                   *reinterpret_cast<Fn**>(to) = new Fn{**reinterpret_cast<const Fn* const*>(from)};
          break;
        case op::move:
          if constexpr(fits<Fn>)
          {
            auto* f{const_cast<Fn*>(reinterpret_cast<const Fn*>(from))};
            ::new (static_cast<void*>(to)) Fn{std::move(*f)};
            f->~Fn();
          }
          else *reinterpret_cast<Fn**>(to) = *reinterpret_cast<Fn* const*>(from);
          break;
        case op::destroy:
          if constexpr(fits<Fn>) reinterpret_cast<const Fn*>(from)->~Fn();
          else                   delete *reinterpret_cast<Fn* const*>(from);
          break;
        }
      };
    }

    /** Where the target lives is decided by `if constexpr` **in place**, and the manager's branches
        repeat their expressions rather than sharing a helper. Every helper here would be another
        entity carrying `Fn` in its name - a member function template obviously so, but at `-O0` a
        lambda too, which nothing inlines away. Measured on `TestAll` under asan: writing the casts
        through lambdas instead cost **61,281 symbols and 20.5 MB**, 613.1 against 592.6. The
        duplication below is much the cheaper of the two; a local variable, which carries no name
        into the symbol table, is cheaper still where one will serve.
     */
    template<class Fn>
    constexpr static call_thunk caller_for{
      [](const std::byte* b, Args&&... args) -> R {
        const Fn* target{};
        if constexpr(fits<Fn>) target = reinterpret_cast<const Fn*>(b);
        else                   target = *reinterpret_cast<const Fn* const*>(b);

        // A void signature discards whatever the target returns, exactly as `std::is_invocable_r_v`
        // - and so the constructor's constraint - already promises it may.
        if constexpr(std::is_void_v<R>)        (*target)(std::forward<Args>(args)...);
        else                            return (*target)(std::forward<Args>(args)...);
      }
    };

    // Both thunks are set by the converting constructor and cleared by a move, always together, so
    // either serves as the test for whether a target is held; each use below reads whichever one it
    // is about to need.
    alignas(std::max_align_t) std::byte m_Buffer[buffer_size]{};
    call_thunk   m_Call{};
    manage_thunk m_Manage{};
  };
}
