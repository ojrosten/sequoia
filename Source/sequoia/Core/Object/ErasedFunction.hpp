////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief A deliberately small owning type-erased callable.

    `std::function` is the obvious tool and is expensive in one specific way:
    every distinct callable it erases instantiates `std::__function::__func<Fn,
    std::allocator<Fn>, Signature>`, a class whose several members each carry the
    closure type *and* the allocator in their mangled names. Across a test suite
    which erases a callable per tested type, that dominates the symbol table.

    `erased_function` erases through three function pointers instead, so a given
    callable contributes three small thunks rather than a class. It has no
    allocator, no `target()`, no `target_type()`, and a small buffer sized for
    the case the tests actually present - a closure capturing one or two
    references.
 */

#include "sequoia/Core/Meta/Concepts.hpp"

#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

namespace sequoia::object
{
  template<class> class erased_function;

  /** \brief Owning type erasure for a callable of the given signature. */
  template<class R, class... Args>
  class erased_function<R(Args...)>
  {
  public:
    using result_type = R;

    erased_function() = default;

    template<class Fn>
      requires (!std::same_as<std::remove_cvref_t<Fn>, erased_function>)
                && std::invocable<Fn&, Args...>
    erased_function(Fn fn)
      : m_Vtable{&vtable_for<Fn>}
    {
      if constexpr(fits<Fn>) ::new (static_cast<void*>(m_Buffer)) Fn{std::move(fn)};
      else                   *reinterpret_cast<Fn**>(m_Buffer) = new Fn{std::move(fn)};
    }

    erased_function(const erased_function& other) : m_Vtable{other.m_Vtable}
    {
      if(m_Vtable) m_Vtable->manage(op::copy, m_Buffer, other.m_Buffer);
    }

    erased_function(erased_function&& other) noexcept : m_Vtable{other.m_Vtable}
    {
      if(m_Vtable) m_Vtable->manage(op::move, m_Buffer, other.m_Buffer);
      other.m_Vtable = nullptr;
    }

    erased_function& operator=(erased_function other) noexcept
    {
      swap(*this, other);
      return *this;
    }

    ~erased_function() { if(m_Vtable) m_Vtable->manage(op::destroy, m_Buffer, m_Buffer); }

    R operator()(Args... args) const
    {
      return m_Vtable->call(const_cast<std::byte*>(m_Buffer), std::forward<Args>(args)...);
    }

    [[nodiscard]] explicit operator bool() const noexcept { return m_Vtable != nullptr; }

    friend void swap(erased_function& lhs, erased_function& rhs) noexcept
    {
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

    template<class Fn>
    constexpr static bool fits{(sizeof(Fn) <= buffer_size) && (alignof(Fn) <= alignof(std::max_align_t))
                               && std::is_nothrow_move_constructible_v<Fn>};

    enum class op { copy, move, destroy };

    /** One manager per erased type rather than one function per operation.

        Each distinct callable contributes exactly two symbols - this and the
        caller - where a function per operation contributes four, and each
        symbol carries the closure type in its name. Halving the count halves
        the contribution to the symbol table, which is what this class exists
        to reduce.
     */
    struct vtable
    {
      R    (*call)(std::byte*, Args&&...);
      void (*manage)(op, std::byte*, const std::byte*);
    };

    template<class Fn>
    constexpr static vtable vtable_for{
      [](std::byte* b, Args&&... args) -> R {
        Fn* f{fits<Fn> ? reinterpret_cast<Fn*>(b) : *reinterpret_cast<Fn**>(b)};
        return (*f)(std::forward<Args>(args)...);
      },
      [](op o, std::byte* to, const std::byte* from) {
        const auto src{[from]() -> const Fn* {
          return fits<Fn> ? reinterpret_cast<const Fn*>(from) : *reinterpret_cast<Fn* const*>(from);
        }};
        switch(o)
        {
        case op::copy:
          if constexpr(fits<Fn>) ::new (static_cast<void*>(to)) Fn{*src()};
          else                   *reinterpret_cast<Fn**>(to) = new Fn{*src()};
          break;
        case op::move:
          if constexpr(fits<Fn>)
          {
            auto* f{const_cast<Fn*>(src())};
            ::new (static_cast<void*>(to)) Fn{std::move(*f)};
            f->~Fn();
          }
          else *reinterpret_cast<Fn**>(to) = *reinterpret_cast<Fn* const*>(from);
          break;
        case op::destroy:
          if constexpr(fits<Fn>) const_cast<Fn*>(src())->~Fn();
          else                   delete const_cast<Fn*>(src());
          break;
        }
      }
    };

    alignas(std::max_align_t) std::byte m_Buffer[buffer_size]{};
    const vtable* m_Vtable{};
  };
}
