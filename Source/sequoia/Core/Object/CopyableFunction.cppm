////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

module;

// The std module does not export the global-namespace allocation functions, and placement new is
// among them.
#include <new>

export module sequoia.core.object:CopyableFunction;

import std;

export import sequoia.core.meta;

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
    the symbol table. Erasing through a pair of function pointers instead - a caller and a single
    manager taking copy/move/destroy as an argument - contributes **two** symbols per erased type
    rather than eight, with no allocator in any of them.

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

export namespace sequoia::object
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
      : m_Vtable{&vtable_for<Fn>}
    {
      if constexpr(fits<Fn>) ::new (static_cast<void*>(m_Buffer)) Fn{std::move(fn)};
      else                   *reinterpret_cast<Fn**>(m_Buffer) = new Fn{std::move(fn)};
    }

    copyable_function(const copyable_function& other) : m_Vtable{other.m_Vtable}
    {
      if(m_Vtable) m_Vtable->manage(op::copy, m_Buffer, other.m_Buffer);
    }

    copyable_function(copyable_function&& other) noexcept : m_Vtable{other.m_Vtable}
    {
      if(m_Vtable) m_Vtable->manage(op::move, m_Buffer, other.m_Buffer);
      other.m_Vtable = nullptr;
    }

    copyable_function& operator=(copyable_function other) noexcept
    {
      swap(*this, other);
      return *this;
    }

    ~copyable_function() { if(m_Vtable) m_Vtable->manage(op::destroy, m_Buffer, m_Buffer); }

    R operator()(Args... args) const
    {
      return m_Vtable->call(m_Buffer, std::forward<Args>(args)...);
    }

    [[nodiscard]]
    explicit operator bool() const noexcept { return m_Vtable != nullptr; }

    /** `op::move` leaves a small target destroyed but a large one's pointer still in the source
        buffer, since the caller always overwrites or abandons what it moved from. Each of the three
        moves below therefore hands ownership on exactly once, and the vtable pointers - which is
        what decides whether a buffer is ever destroyed - are exchanged last.
     */
    friend void swap(copyable_function& lhs, copyable_function& rhs) noexcept
    {
      // Without this, a self-swap moves the target out of the buffer and then straight back out of
      // the buffer it has just vacated.
      if(&lhs == &rhs) return;

      alignas(std::max_align_t) std::byte tmp[buffer_size];
      const auto lv{lhs.m_Vtable}, rv{rhs.m_Vtable};
      if(lv) lv->manage(op::move, tmp, lhs.m_Buffer);
      if(rv) rv->manage(op::move, lhs.m_Buffer, rhs.m_Buffer);
      if(lv) lv->manage(op::move, rhs.m_Buffer, tmp);
      lhs.m_Vtable = rv;
      rhs.m_Vtable = lv;
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

    enum class op { copy, move, destroy };

    /** One manager per erased type rather than one function per operation.

        Each distinct callable contributes exactly two symbols - this and the caller - where a
        function per operation contributes four, and every one of them carries the closure type in
        its name. Collapsing four thunks into one was worth more than the original swap away from
        `std::function`, which is why the operation is a runtime argument rather than four entries.
     */
    struct vtable
    {
      R    (*call)(const std::byte*, Args&&...);
      void (*manage)(op, std::byte*, const std::byte*);
    };

    /** Where the target lives is decided by `if constexpr` **in place**, and the manager's branches
        repeat their expressions rather than sharing a helper. Every helper here would be another
        entity carrying `Fn` in its name - a member function template obviously so, but at `-O0` a
        lambda too, which nothing inlines away. Measured on `TestAll` under asan: writing the casts
        through lambdas instead cost **61,281 symbols and 20.5 MB**, 613.1 against 592.6. The
        duplication below is much the cheaper of the two; a local variable, which carries no name
        into the symbol table, is cheaper still where one will serve.
     */
    template<class Fn>
    constexpr static vtable vtable_for{
      [](const std::byte* b, Args&&... args) -> R {
        const Fn* target{};
        if constexpr(fits<Fn>) target = reinterpret_cast<const Fn*>(b);
        else                   target = *reinterpret_cast<const Fn* const*>(b);

        // A void signature discards whatever the target returns, exactly as `std::is_invocable_r_v`
        // - and so the constructor's constraint - already promises it may.
        if constexpr(std::is_void_v<R>)        (*target)(std::forward<Args>(args)...);
        else                            return (*target)(std::forward<Args>(args)...);
      },
      [](op o, std::byte* to, const std::byte* from) {
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
      }
    };

    alignas(std::max_align_t) std::byte m_Buffer[buffer_size]{};
    const vtable* m_Vtable{};
  };
}
