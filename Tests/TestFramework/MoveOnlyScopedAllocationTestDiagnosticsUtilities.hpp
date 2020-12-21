////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once


#include "MoveOnlyTestCore.hpp"

#include <vector>
#include <string>

namespace sequoia::testing
{
  template<class InnerAllocator>
  struct move_only_scoped_beast
  {
    using string = std::basic_string<char, std::char_traits<char>, InnerAllocator>;

    using inner_allocator_type = InnerAllocator;
    using outer_allocator_type = typename InnerAllocator::template rebind<string>::other;
    
    using allocator_type
      = std::scoped_allocator_adaptor<outer_allocator_type, InnerAllocator>;

    move_only_scoped_beast(std::initializer_list<string> list) : x{list} {}

    move_only_scoped_beast(std::initializer_list<string> list, const allocator_type& a) : x{list, a} {}

    move_only_scoped_beast(const allocator_type& a) : x(a) {}

    move_only_scoped_beast(const move_only_scoped_beast&) = delete;

    move_only_scoped_beast(move_only_scoped_beast&&) noexcept = default;

    move_only_scoped_beast(move_only_scoped_beast&& other, const allocator_type& a) : x(std::move(other.x), a) {}

    move_only_scoped_beast& operator=(const move_only_scoped_beast&) = delete;

    move_only_scoped_beast& operator=(move_only_scoped_beast&&) = default;

    void swap(move_only_scoped_beast& other) noexcept(noexcept(std::swap(this->x, other.x)))
    {
      std::swap(x, other.x);
    }

    friend void swap(move_only_scoped_beast& lhs, move_only_scoped_beast& rhs)
      noexcept(noexcept(lhs.swap(rhs)))
    {
      lhs.swap(rhs);
    }

    std::vector<string, allocator_type> x;

    [[nodiscard]]
    friend bool operator==(const move_only_scoped_beast& lhs, const move_only_scoped_beast& rhs) noexcept
    {
      return lhs.x == rhs.x;
    }

    [[nodiscard]]
    friend bool operator!=(const move_only_scoped_beast& lhs, const move_only_scoped_beast& rhs) noexcept
    {
      return !(lhs == rhs);
    }

    template<class Stream>
    friend Stream& operator<<(Stream& s, const move_only_scoped_beast& b)
    {
      for(auto i : b.x) s << i << '\n';
      return s;
    }
  };
}
