////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once


#include "RegularTestCore.hpp"

#include <vector>
#include <string>

namespace sequoia::testing
{
  template<class InnerAllocator>
  struct perfectly_scoped_beast
  {
    using string = std::basic_string<char, std::char_traits<char>, InnerAllocator>;

    using inner_allocator_type = InnerAllocator;
    using outer_allocator_type = typename InnerAllocator::template rebind<string>::other;
    
    using allocator_type
      = std::scoped_allocator_adaptor<outer_allocator_type, InnerAllocator>;

    perfectly_scoped_beast(std::initializer_list<string> list) : x{list} {}

    perfectly_scoped_beast(std::initializer_list<string> list, const allocator_type& a) : x(list, a) {}

    perfectly_scoped_beast(const allocator_type& a) : x(a) {}

    perfectly_scoped_beast(const perfectly_scoped_beast&) = default;

    perfectly_scoped_beast(const perfectly_scoped_beast& other, const allocator_type& a) : x(other.x, a) {}

    perfectly_scoped_beast(perfectly_scoped_beast&&) noexcept = default;

    perfectly_scoped_beast(perfectly_scoped_beast&& other, const allocator_type& a) : x(std::move(other.x), a) {}

    perfectly_scoped_beast& operator=(const perfectly_scoped_beast&) = default;

    perfectly_scoped_beast& operator=(perfectly_scoped_beast&&) = default;

    void swap(perfectly_scoped_beast& other) noexcept(noexcept(std::swap(this->x, other.x)))
    {
      std::swap(x, other.x);
    }

    friend void swap(perfectly_scoped_beast& lhs, perfectly_scoped_beast& rhs)
      noexcept(noexcept(lhs.swap(rhs)))
    {
      lhs.swap(rhs);
    }

    std::vector<string, allocator_type> x;

    [[nodiscard]]
    friend bool operator==(const perfectly_scoped_beast& lhs, const perfectly_scoped_beast& rhs) noexcept
    {
      return lhs.x == rhs.x;
    }

    [[nodiscard]]
    friend bool operator!=(const perfectly_scoped_beast& lhs, const perfectly_scoped_beast& rhs) noexcept
    {
      return !(lhs == rhs);
    }

    template<class Stream>
    friend Stream& operator<<(Stream& s, const perfectly_scoped_beast& b)
    {
      for(auto i : b.x) s << i << '\n';
      return s;
    }
  };

  template<class InnerAllocator>
  struct alloc_equivalence_class<perfectly_scoped_beast<InnerAllocator>>
  {
    using type = std::tuple<std::vector<int>, std::string>;
  };
}
