////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once


#include "RegularTestCore.hpp"
#include "CoreDiagnosticsUtilities.hpp"

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
    friend bool operator==(const perfectly_scoped_beast& lhs, const perfectly_scoped_beast& rhs) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const perfectly_scoped_beast& lhs, const perfectly_scoped_beast& rhs) noexcept = default;

    template<class Stream>
    friend Stream& operator<<(Stream& s, const perfectly_scoped_beast& b)
    {
      for(auto i : b.x) s << i << '\n';
      return s;
    }
  };

  template<class InnerAllocator>
  struct perfectly_mixed_beast
  {
    using sharing_beast = perfectly_sharing_beast<int, std::shared_ptr<int>, InnerAllocator>;

    using inner_allocator_type = InnerAllocator;
    using outer_allocator_type = typename InnerAllocator::template rebind<sharing_beast>::other;

    using allocator_type
      = std::scoped_allocator_adaptor<outer_allocator_type, InnerAllocator>;

    struct alloc_acquirer
    {
      using outer_equiv_class = allocation_equivalence_classes::container_of_values<outer_allocator_type>;
      using inner_equiv_class = allocation_equivalence_classes::container_of_pointers<inner_allocator_type>;

      using alloc_equivalence_class = std::tuple<outer_equiv_class, inner_equiv_class>;

      [[nodiscard]]
      allocator_type operator()(const perfectly_mixed_beast& beast) const
      {
        return beast.x.get_allocator();
      }
    };

    perfectly_mixed_beast(std::initializer_list<sharing_beast> list) : x{list} {}

    perfectly_mixed_beast(std::initializer_list<sharing_beast> list, const allocator_type& a) : x(list, a) {}

    perfectly_mixed_beast(const allocator_type& a) : x(a) {}

    perfectly_mixed_beast(const perfectly_mixed_beast&) = default;

    perfectly_mixed_beast(const perfectly_mixed_beast& other, const allocator_type& a) : x(other.x, a) {}

    perfectly_mixed_beast(perfectly_mixed_beast&&) noexcept = default;

    perfectly_mixed_beast(perfectly_mixed_beast&& other, const allocator_type& a) : x(std::move(other.x), a) {}

    perfectly_mixed_beast& operator=(const perfectly_mixed_beast&) = default;

    perfectly_mixed_beast& operator=(perfectly_mixed_beast&&) = default;

    void swap(perfectly_mixed_beast& other) noexcept(noexcept(std::swap(this->x, other.x)))
    {
      std::swap(x, other.x);
    }

    friend void swap(perfectly_mixed_beast& lhs, perfectly_mixed_beast& rhs)
      noexcept(noexcept(lhs.swap(rhs)))
    {
      lhs.swap(rhs);
    }

    std::vector<sharing_beast, allocator_type> x;

    [[nodiscard]]
    friend bool operator==(const perfectly_mixed_beast& lhs, const perfectly_mixed_beast& rhs) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const perfectly_mixed_beast& lhs, const perfectly_mixed_beast& rhs) noexcept = default;

    template<class Stream>
    friend Stream& operator<<(Stream& s, const perfectly_mixed_beast& b)
    {
      for(auto i : b.x) s << i << '\n';
      return s;
    }
  };
}
