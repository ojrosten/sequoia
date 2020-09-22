////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "MoveOnlyTestCore.hpp"

#include <vector>

namespace sequoia::testing
{
  template<class T=int, class Allocator=std::allocator<int>>
  struct orderable_move_only_beast
  {
    using allocator_type = Allocator;

    orderable_move_only_beast(std::initializer_list<T> list) : x{list} {}

    orderable_move_only_beast(std::initializer_list<T> list, const allocator_type& a) : x{list, a} {}

    orderable_move_only_beast(const allocator_type& a) : x(a) {}

    orderable_move_only_beast(const orderable_move_only_beast&) = delete;

    orderable_move_only_beast(orderable_move_only_beast&&) noexcept = default;

    orderable_move_only_beast(orderable_move_only_beast&& other, const allocator_type& alloc) : x(std::move(other.x), alloc) {}

    orderable_move_only_beast& operator=(const orderable_move_only_beast&) = delete;

    orderable_move_only_beast& operator=(orderable_move_only_beast&&) = default;

    void swap(orderable_move_only_beast& other) noexcept(noexcept(std::swap(this->x, other.x)))
    {
      std::swap(x, other.x);
    }

    friend void swap(orderable_move_only_beast& lhs, orderable_move_only_beast& rhs)
      noexcept(noexcept(lhs.swap(rhs)))
    {
      lhs.swap(rhs);
    }
      
    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const orderable_move_only_beast&, const orderable_move_only_beast&) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const orderable_move_only_beast&, const orderable_move_only_beast&) noexcept = default;

    [[nodiscard]]
    friend bool operator<(const orderable_move_only_beast& lhs, const orderable_move_only_beast& rhs) noexcept
    {
      return lhs.x < rhs.x;
    }

    [[nodiscard]]
    friend bool operator<=(const orderable_move_only_beast& lhs, const orderable_move_only_beast& rhs) noexcept
    {
      return lhs.x <= rhs.x;
    }

    [[nodiscard]]
    friend bool operator>(const orderable_move_only_beast& lhs, const orderable_move_only_beast& rhs) noexcept
    {
      return lhs.x > rhs.x;
    }

    [[nodiscard]]
    friend bool operator>=(const orderable_move_only_beast& lhs, const orderable_move_only_beast& rhs) noexcept
    {
      return lhs.x >= rhs.x;
    }

    // TO DO: default this and remove most of the above when libc++ rolls out <=> to std
    friend std::weak_ordering operator<=>(const orderable_move_only_beast& lhs, const orderable_move_only_beast& rhs) noexcept
    {
      if(lhs < rhs) return std::weak_ordering::less;

      if(rhs > lhs) return std::weak_ordering::greater;

      return std::weak_ordering::equivalent;
    }

    template<class Stream>
    friend Stream& operator<<(Stream& s, const orderable_move_only_beast& b)
    {
      for(auto i : b.x) s << i << ' ';
      return s;
    }
  };
}
