////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/Core/ContainerUtilities/AssignmentUtilities.hpp"

#include <vector>

namespace sequoia::testing
{
  template<class T=int, class Allocator=std::allocator<int>>
  struct inefficient_equality
  {
    using allocator_type = Allocator;

    inefficient_equality(std::initializer_list<T> list) : x{list} {}

    inefficient_equality(std::initializer_list<T> list, const allocator_type& a) : x(list, a) {}

    inefficient_equality(const inefficient_equality&) = default;

    inefficient_equality(const inefficient_equality& other, const allocator_type& a) : x(other.x, a) {}

    inefficient_equality(inefficient_equality&&) noexcept = default;

    inefficient_equality(inefficient_equality&& other, const allocator_type& a) : x(std::move(other.x), a) {}

    inefficient_equality& operator=(const inefficient_equality&) = default;

    inefficient_equality& operator=(inefficient_equality&&) = default;

    friend void swap(inefficient_equality& lhs, inefficient_equality& rhs)
    {
      std::swap(lhs.x, rhs.x);
    }

    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const inefficient_equality lhs, const inefficient_equality rhs) noexcept
    {
      return lhs.x == rhs.x;
    }

    [[nodiscard]]
    friend bool operator!=(const inefficient_equality& lhs, const inefficient_equality& rhs) noexcept
    {
      return lhs.x != rhs.x;
    }

    template<class Stream>
    friend Stream& operator<<(Stream& s, const inefficient_equality& b)
    {
      for(auto i : b.x) s << i << ' ';
      return s;
    }
  };

  template<class T, class Allocator>
  struct allocation_count_shifter<inefficient_equality<T, Allocator>> : allocation_count_shifter<int>
  {
    using allocation_count_shifter<int>::shift;

    static int shift(int count, const alloc_prediction<comparison_flavour::equality>&) noexcept
    {
      if constexpr(with_msvc_v && (iterator_debug_level() > 0))
      {
        if(count > 1) return count + 2;
      }

      return count;
    }
  };

  template<class T=int, class Allocator=std::allocator<int>>
  struct inefficient_inequality
  {
    using allocator_type = Allocator;

    inefficient_inequality(std::initializer_list<T> list) : x{list} {}

    inefficient_inequality(std::initializer_list<T> list, const allocator_type& a) : x(list, a) {}

    inefficient_inequality(const inefficient_inequality&) = default;

    inefficient_inequality(const inefficient_inequality& other, const allocator_type& a) : x(other.x, a) {}

    inefficient_inequality(inefficient_inequality&&) noexcept = default;

    inefficient_inequality(inefficient_inequality&& other, const allocator_type& a) : x(std::move(other.x), a) {}

    inefficient_inequality& operator=(const inefficient_inequality&) = default;

    inefficient_inequality& operator=(inefficient_inequality&&) = default;

    friend void swap(inefficient_inequality& lhs, inefficient_inequality& rhs)
    {
      std::swap(lhs.x, rhs.x);
    }

    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const inefficient_inequality& lhs, const inefficient_inequality& rhs) noexcept
    {
      return lhs.x == rhs.x;
    }

    [[nodiscard]]
    friend bool operator!=(const inefficient_inequality lhs, const inefficient_inequality rhs) noexcept
    {
      return !(rhs == lhs);
    }

    template<class Stream>
    friend Stream& operator<<(Stream& s, const inefficient_inequality& b)
    {
      for(auto i : b.x) s << i << ' ';
      return s;
    }
  };

  template<class T, class Allocator>
  struct allocation_count_shifter<inefficient_inequality<T, Allocator>> : allocation_count_shifter<int>
  {
    using allocation_count_shifter<int>::shift;

    static int shift(int count, const alloc_prediction<comparison_flavour::inequality>&) noexcept
    {
      if constexpr(with_msvc_v && (iterator_debug_level() > 0))
      {
        if(count > 1) return count + 2;
      }

      return count;
    }
  };

  template<class T=int, class Allocator=std::allocator<int>>
  struct inefficient_copy
  {
    using allocator_type = Allocator;

    inefficient_copy(std::initializer_list<T> list) : x{list} {}

    inefficient_copy(std::initializer_list<T> list, const allocator_type& a) : x(list, a) {}

    inefficient_copy(const inefficient_copy& other) : x(other.x.get_allocator())
    {
      x.reserve(1);
      x.shrink_to_fit();
      x.reserve(other.x.size());
      std::copy(other.x.cbegin(), other.x.cend(), std::back_inserter(x));
    }

    inefficient_copy(const inefficient_copy& other, const allocator_type& a)
      : x(other.x, a)
    {}

    inefficient_copy(inefficient_copy&&) noexcept = default;

    inefficient_copy(inefficient_copy&& other, const allocator_type& a) : x(std::move(other.x), a)
    {}

    inefficient_copy& operator=(const inefficient_copy&) = default;

    inefficient_copy& operator=(inefficient_copy&&) = default;

    friend void swap(inefficient_copy& lhs, inefficient_copy& rhs)
    {
      std::swap(lhs.x, rhs.x);
    }

    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const inefficient_copy& lhs, const inefficient_copy& rhs) noexcept
    {
      return lhs.x == rhs.x;
    }

    [[nodiscard]]
    friend bool operator!=(const inefficient_copy& lhs, const inefficient_copy& rhs) noexcept
    {
      return !(lhs == rhs);
    }

    template<class Stream>
    friend Stream& operator<<(Stream& s, const inefficient_copy& b)
    {
      for(auto i : b.x) s << i << ' ';
      return s;
    }
  };

  template<class T=int, class Allocator=std::allocator<int>>
  struct inefficient_para_copy
  {
    using allocator_type = Allocator;

    inefficient_para_copy(std::initializer_list<T> list) : x{list} {}

    inefficient_para_copy(std::initializer_list<T> list, const allocator_type& a) : x(list, a) {}

    inefficient_para_copy(const inefficient_para_copy&) = default;

    inefficient_para_copy(const inefficient_para_copy& other, const allocator_type& a)
      : x(a)
    {
      x.reserve(1);
      x.shrink_to_fit();
      x.reserve(other.x.size());
      std::copy(other.x.cbegin(), other.x.cend(), std::back_inserter(x));
    }

    inefficient_para_copy(inefficient_para_copy&&) noexcept = default;

    inefficient_para_copy(inefficient_para_copy&& other, const allocator_type& a) : x(std::move(other.x), a) {}

    inefficient_para_copy& operator=(const inefficient_para_copy&) = default;

    inefficient_para_copy& operator=(inefficient_para_copy&&) = default;

    friend void swap(inefficient_para_copy& lhs, inefficient_para_copy& rhs)
    {
      std::swap(lhs.x, rhs.x);
    }

    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const inefficient_para_copy& lhs, const inefficient_para_copy& rhs) noexcept
    {
      return lhs.x == rhs.x;
    }

    [[nodiscard]]
    friend bool operator!=(const inefficient_para_copy& lhs, const inefficient_para_copy& rhs) noexcept
    {
      return !(lhs == rhs);
    }

    template<class Stream>
    friend Stream& operator<<(Stream& s, const inefficient_para_copy& b)
    {
      for(auto i : b.x) s << i << ' ';
      return s;
    }
  };

  template<class T=int, class Allocator=std::allocator<int>>
  struct inefficient_move
  {
    using allocator_type = Allocator;

    inefficient_move(std::initializer_list<T> list) : x{list} {}

    inefficient_move(std::initializer_list<T> list, const allocator_type& a) : x(list, a) {}

    inefficient_move(const inefficient_move& other) = default;

    inefficient_move(const inefficient_move& other, const allocator_type& a)
      : x(other.x, a)
    {}

    inefficient_move(inefficient_move&& other) : x{std::move(other.x)}
    {
      x.reserve(x.capacity() + 1);
    }

    inefficient_move(inefficient_move&& other, const allocator_type& a) : x(std::move(other.x), a) {}

    inefficient_move& operator=(const inefficient_move&) = default;

    inefficient_move& operator=(inefficient_move&&) = default;

    friend void swap(inefficient_move& lhs, inefficient_move& rhs)
    {
      std::swap(lhs.x, rhs.x);
    }

    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const inefficient_move& lhs, const inefficient_move& rhs) noexcept
    {
      return lhs.x == rhs.x;
    }

    [[nodiscard]]
    friend bool operator!=(const inefficient_move& lhs, const inefficient_move& rhs) noexcept
    {
      return !(lhs == rhs);
    }

    template<class Stream>
    friend Stream& operator<<(Stream& s, const inefficient_move& b)
    {
      for(auto i : b.x) s << i << ' ';
      return s;
    }
  };

  template<class T=int, class Allocator=std::allocator<int>>
  struct inefficient_para_move
  {
    using allocator_type = Allocator;

    inefficient_para_move(std::initializer_list<T> list) : x{list} {}

    inefficient_para_move(std::initializer_list<T> list, const allocator_type& a) : x(list, a) {}

    inefficient_para_move(const inefficient_para_move& other) = default;

    inefficient_para_move(const inefficient_para_move& other, const allocator_type& a)
      : x(other.x, a)
    {}

    inefficient_para_move(inefficient_para_move&& other) = default;

    inefficient_para_move(inefficient_para_move&& other, const allocator_type& a) : x(std::move(other.x), a)
    {
       x.reserve(x.capacity() + 1);
    }

    inefficient_para_move& operator=(const inefficient_para_move&) = default;

    inefficient_para_move& operator=(inefficient_para_move&&) = default;

    friend void swap(inefficient_para_move& lhs, inefficient_para_move& rhs)
    {
      std::swap(lhs.x, rhs.x);
    }

    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const inefficient_para_move& lhs, const inefficient_para_move& rhs) noexcept
    {
      return lhs.x == rhs.x;
    }

    [[nodiscard]]
    friend bool operator!=(const inefficient_para_move& lhs, const inefficient_para_move& rhs) noexcept
    {
      return !(lhs == rhs);
    }

    template<class Stream>
    friend Stream& operator<<(Stream& s, const inefficient_para_move& b)
    {
      for(auto i : b.x) s << i << ' ';
      return s;
    }
  };

  template<class T=int, class Allocator=std::allocator<int>>
  struct inefficient_serialization
  {
    using allocator_type = Allocator;

    inefficient_serialization(std::initializer_list<T> list) : x{list} {}

    inefficient_serialization(std::initializer_list<T> list, const allocator_type& a) : x{list, a} {}

    inefficient_serialization(const allocator_type& a) : x(a) {}

    inefficient_serialization(const inefficient_serialization&) = default;

    inefficient_serialization(const inefficient_serialization& other, const allocator_type& a) : x(other.x, a) {}

    inefficient_serialization(inefficient_serialization&&) noexcept = default;

    inefficient_serialization(inefficient_serialization&& other, const allocator_type& a) : x(std::move(other.x), a) {}

    inefficient_serialization& operator=(const inefficient_serialization&) = default;

    inefficient_serialization& operator=(inefficient_serialization&&) = default;

    void swap(inefficient_serialization& other) noexcept(noexcept(std::swap(this->x, other.x)))
    {
      std::swap(x, other.x);
    }

    friend void swap(inefficient_serialization& lhs, inefficient_serialization& rhs)
      noexcept(noexcept(lhs.swap(rhs)))
    {
      lhs.swap(rhs);
    }

    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const inefficient_serialization&, const inefficient_serialization&) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const inefficient_serialization&, const inefficient_serialization&) noexcept = default;

    template<class Stream>
    friend Stream& operator<<(Stream& s, const inefficient_serialization b)
    {
      for(auto i : b.x) s << i << ' ';
      return s;
    }

    template<class Stream>
    friend Stream& operator>>(Stream& s, inefficient_serialization& b)
    {
      b.x.clear();

      int i{};
      while(s >> i)
      {
        b.x.push_back(i);
      }

      return s;
    }
  };

  template<class T, class Allocator>
  struct allocation_count_shifter<inefficient_serialization<T, Allocator>> : allocation_count_shifter<int>
  {
    using allocation_count_shifter<int>::shift;

    static int shift(int count, const alloc_prediction<null_allocation_event::serialization>&)
    {
      if constexpr(with_msvc_v && (iterator_debug_level() > 0))
      {
        if(count) return count + 1;
      }

      return count;
    }
  };

  template<class T = int, class Allocator = std::allocator<int>>
  struct broken_copy_assignment_propagation
  {
    using allocator_type = Allocator;

    broken_copy_assignment_propagation(std::initializer_list<T> list) : x{list} {}

    broken_copy_assignment_propagation(std::initializer_list<T> list, const allocator_type& a) : x{list, a} {}

    broken_copy_assignment_propagation(const allocator_type& a) : x(a) {}

    broken_copy_assignment_propagation(const broken_copy_assignment_propagation& other) = default;

    broken_copy_assignment_propagation(const broken_copy_assignment_propagation& other, const allocator_type& a) : x(other.x, a) {}

    broken_copy_assignment_propagation(broken_copy_assignment_propagation&&) noexcept = default;

    broken_copy_assignment_propagation(broken_copy_assignment_propagation&& other, const allocator_type& a) : x(std::move(other.x), a) {}

    // Broken if propagation on copy assignment is not the same as for move
    broken_copy_assignment_propagation& operator=(const broken_copy_assignment_propagation& other)
    {
      auto tmp{other};
      *this = std::move(tmp);

      return *this;
    }

    broken_copy_assignment_propagation& operator=(broken_copy_assignment_propagation&&) = default;

    void swap(broken_copy_assignment_propagation& other) noexcept(noexcept(std::swap(this->x, other.x)))
    {
      std::swap(x, other.x);
    }

    friend void swap(broken_copy_assignment_propagation& lhs, broken_copy_assignment_propagation& rhs)
      noexcept(noexcept(lhs.swap(rhs)))
    {
      lhs.swap(rhs);
    }

    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const broken_copy_assignment_propagation&, const broken_copy_assignment_propagation&) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const broken_copy_assignment_propagation&, const broken_copy_assignment_propagation&) noexcept = default;

    template<class Stream>
    friend Stream& operator<<(Stream& s, const broken_copy_assignment_propagation& b)
    {
      for(auto i : b.x) s << i << ' ';
      return s;
    }

    template<class Stream>
    friend Stream& operator>>(Stream& s, broken_copy_assignment_propagation& b)
    {
      b.x.clear();

      int i{};
      while(s >> i)
      {
        b.x.push_back(i);
      }

      return s;
    }
  };

  template<class T, bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  struct allocation_count_shifter<broken_copy_assignment_propagation<T, shared_counting_allocator<T, PropagateCopy, PropagateMove, PropagateSwap>>>
    : allocation_count_shifter<int>
  {
    using allocation_count_shifter<int>::shift;

    static int shift(int count, const alloc_prediction<null_allocation_event::spectator>&)
    {
      if constexpr(with_msvc_v && (iterator_debug_level() > 0))
      {
        if constexpr(!PropagateCopy)
        {
          if(count) return count + 2;
        }
      }

      return count;
    }

    static int shift(int count, const alloc_prediction<assignment_allocation_event::assign_no_prop>&)
    {
      if constexpr(with_msvc_v && (iterator_debug_level() > 0))
      {
        if constexpr(!PropagateCopy)
        {
          if(count) return count + 2;
        }
      }

      return count;
    }

    static int shift(int count, const alloc_prediction<assignment_allocation_event::assign>&)
    {
      if constexpr(with_msvc_v && (iterator_debug_level() > 0))
      {
        if(count) return count - 1;
      }

      return count;
    }
  };
}
