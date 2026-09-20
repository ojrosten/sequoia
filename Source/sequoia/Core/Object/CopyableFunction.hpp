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

    ## In a constant evaluation

    Bytes cannot be reinterpreted there, so every target is held behind a pointer, and its manager
    is a `consteval` function: reachable only at compile time and therefore never emitted, which
    keeps the run-time symbol economy above intact. The cast back from `void*` is C++26's (P2738).
    A `copyable_function` may live within a constant evaluation but not outlive one: a `constexpr`
    variable of this type is ill-formed, since its target is an allocation.
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

    constexpr copyable_function() = default;

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
    constexpr copyable_function(Fn fn)
      : m_Call{caller_for<Fn>}, m_Manage{manager_for<Fn>()}
    {
      // The address of a consteval function may be taken only in an immediate function context,
      // which the block of an `if consteval` is and a mem-initializer is not
      if consteval
      {
        m_Manage = &manage_in_constant_evaluation<Fn>;
        m_Storage.pointer = new Fn{std::move(fn)};
      }
      else
      {
        if constexpr(fits<Fn>)
          ::new (static_cast<void*>(m_Storage.buffer)) Fn{std::move(fn)};
        else
          m_Storage.pointer = new Fn{std::move(fn)};
      }
    }

    constexpr copyable_function(const copyable_function& other) : m_Call{other.m_Call}, m_Manage{other.m_Manage}
    {
      if(m_Manage)
        m_Manage(op::copy, m_Storage, other.m_Storage);
    }

    constexpr copyable_function(copyable_function&& other) noexcept : m_Call{other.m_Call}, m_Manage{other.m_Manage}
    {
      if(m_Manage)
        m_Manage(op::move, m_Storage, other.m_Storage);
      other.m_Call   = nullptr;
      other.m_Manage = nullptr;
    }

    constexpr copyable_function& operator=(copyable_function other) noexcept
    {
      swap(*this, other);
      return *this;
    }

    constexpr ~copyable_function()
    {
      if(m_Manage)
        m_Manage(op::destroy, m_Storage, m_Storage);
    }

    constexpr R operator()(Args... args) const
    {
      return m_Call(m_Storage, std::forward<Args>(args)...);
    }

    [[nodiscard]]
    constexpr explicit operator bool() const noexcept { return m_Call != nullptr; }

    /** `op::move` hands ownership on exactly once, and the thunk pointers - which are what decide
        whether a buffer is ever destroyed - are exchanged last, so each of the three moves below is
        safe.

        What `op::move` leaves behind differs by manager, and deliberately: the general one destroys
        a small source and leaves a large one's pointer in place, the shared one leaves the
        source's bytes alone, and the constant-evaluation one leaves the pointer in place. All are
        correct because the caller always overwrites or abandons what it moved from, and because a
        trivially managed target has nothing to destroy.
     */
    friend constexpr void swap(copyable_function& lhs, copyable_function& rhs) noexcept
    {
      // Without this, a self-swap moves the target out of the buffer and then straight back out of
      // the buffer it has just vacated.
      if(&lhs == &rhs)
        return;

      storage tmp{};
      const auto lm{lhs.m_Manage}, rm{rhs.m_Manage};
      if(lm)
        lm(op::move, tmp, lhs.m_Storage);

      if(rm)
        rm(op::move, lhs.m_Storage, rhs.m_Storage);

      if(lm)
        lm(op::move, rhs.m_Storage, tmp);
      std::swap(lhs.m_Call,   rhs.m_Call);
      std::swap(lhs.m_Manage, rhs.m_Manage);
    }
  private:
    constexpr static std::size_t buffer_size{3 * sizeof(void*)};

    /** A small target lives in the buffer; a large one, and in a constant evaluation every one,
        behind the pointer.
     */
    union storage
    {
      alignas(std::max_align_t) std::byte buffer[buffer_size];
      void* pointer;
    };

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
    using call_thunk    = R    (*)(const storage&, Args&&...);
    using manage_thunk  = void (*)(op, storage&, const storage&);

    /** The manager shared by every trivially-managed target, and so **not** a template: one symbol
        per signature rather than one per erased type.

        Copying the whole buffer rather than `sizeof(Fn)` bytes is what makes the sharing possible,
        and it is reading and writing within a `std::byte` array which is always fully initialized,
        never past the end of an object.
     */
    static void manage_trivially(op o, storage& to, const storage& from)
    {
      if(o != op::destroy)
        std::memcpy(to.buffer, from.buffer, buffer_size);
    }

    /** The manager of every target in a constant evaluation, where the target is behind the
        pointer; `consteval`, so that no run-time symbol is added for it.
     */
    template<class Fn>
    consteval static void manage_in_constant_evaluation(op o, storage& to, const storage& from)
    {
      switch(o)
      {
      case op::copy:
        to.pointer = new Fn{*static_cast<const Fn*>(from.pointer)};
        break;
      case op::move:
        to.pointer = from.pointer;
        break;
      case op::destroy:
        delete static_cast<Fn*>(from.pointer);
        break;
      }
    }

    /** Chosen with `if constexpr` rather than a ternary, so that the general manager is instantiated
        only for the targets which actually need it - a ternary would odr-use both operands and
        defeat the sharing entirely. `consteval` guarantees this function is itself never emitted.
     */
    template<class Fn>
    consteval static manage_thunk manager_for()
    {
      if constexpr(trivially_managed<Fn>)
      {
        return &manage_trivially;
      }
      else
      {
        return [](op o, storage& to, const storage& from) {
          switch(o)
          {
          case op::copy:
            if constexpr(fits<Fn>)
              ::new (static_cast<void*>(to.buffer)) Fn{*reinterpret_cast<const Fn*>(from.buffer)};
            else
              to.pointer = new Fn{*static_cast<const Fn*>(from.pointer)};
            break;
          case op::move:
            if constexpr(fits<Fn>)
            {
              auto* const f{const_cast<Fn*>(reinterpret_cast<const Fn*>(from.buffer))};
              ::new (static_cast<void*>(to.buffer)) Fn{std::move(*f)};
              f->~Fn();
            }
            else
              to.pointer = from.pointer;
            break;
          case op::destroy:
            if constexpr(fits<Fn>)
              reinterpret_cast<const Fn*>(from.buffer)->~Fn();
            else
              delete static_cast<Fn*>(from.pointer);
            break;
          }
        };
      }
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
      [](const storage& s, Args&&... args) -> R {
        const Fn* target{};
        if consteval
        {
          target = static_cast<const Fn*>(s.pointer);
        }
        else
        {
          if constexpr(fits<Fn>)
            target = reinterpret_cast<const Fn*>(s.buffer);
          else
            target = static_cast<const Fn*>(s.pointer);
        }

        // A void signature discards whatever the target returns, exactly as `std::is_invocable_r_v`
        // - and so the constructor's constraint - already promises it may.
        if constexpr(std::is_void_v<R>)
          (*target)(std::forward<Args>(args)...);
        else
          return (*target)(std::forward<Args>(args)...);
      }
    };

    /** Both thunks are set by the converting constructor and cleared by a move, always together, so
        either serves as the test for whether a target is held; each use above reads whichever one it
        is about to need.
     */
    storage      m_Storage{};
    call_thunk   m_Call{};
    manage_thunk m_Manage{};
  };
}
