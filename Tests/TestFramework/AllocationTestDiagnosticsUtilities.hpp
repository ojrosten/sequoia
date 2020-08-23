////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "AssignmentUtilities.hpp"

#include <vector>

namespace sequoia::testing
{
  template<class T=int, class Allocator=std::allocator<int>>
  struct inefficient_equality
  {
    using allocator_type = Allocator;

    inefficient_equality(std::initializer_list<T> list) : x{list} {}
      
    inefficient_equality(std::initializer_list<T> list, const allocator_type& alloc) : x{list, alloc} {}

    inefficient_equality(const inefficient_equality&) = default;

    inefficient_equality(const inefficient_equality& other, const allocator_type& alloc) : x(other.x, alloc) {}

    inefficient_equality(inefficient_equality&&) noexcept = default;

    inefficient_equality(inefficient_equality&& other, const allocator_type& alloc) : x(std::move(other.x), alloc) {}

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

  template<class T=int, class Allocator=std::allocator<int>>
  struct inefficient_inequality
  {
    using allocator_type = Allocator;

    inefficient_inequality(std::initializer_list<T> list) : x{list} {}
      
    inefficient_inequality(std::initializer_list<T> list, const allocator_type& alloc) : x{list, alloc} {}

    inefficient_inequality(const inefficient_inequality&) = default;

    inefficient_inequality(const inefficient_inequality& other, const allocator_type& alloc) : x(other.x, alloc) {}

    inefficient_inequality(inefficient_inequality&&) noexcept = default;

    inefficient_inequality(inefficient_inequality&& other, const allocator_type& alloc) : x(std::move(other.x), alloc) {}

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

  template<class T=int, class Allocator=std::allocator<int>>
  struct inefficient_copy
  {
    using allocator_type = Allocator;

    inefficient_copy(std::initializer_list<T> list) : x{list} {}
      
    inefficient_copy(std::initializer_list<T> list, const allocator_type& alloc) : x{list, alloc} {}

    inefficient_copy(const inefficient_copy& other) : x(other.x.get_allocator())
    {
      x.reserve(1);
      x.shrink_to_fit();
      x.reserve(other.x.size());
      std::copy(other.x.cbegin(), other.x.cend(), std::back_inserter(x));
    }

    inefficient_copy(const inefficient_copy& other, const allocator_type& alloc)
      : x(other.x, alloc)
    {}

    inefficient_copy(inefficient_copy&&) noexcept = default;

    inefficient_copy(inefficient_copy&& other, const allocator_type& alloc) : x(std::move(other.x), alloc)
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
      
    inefficient_para_copy(std::initializer_list<T> list, const allocator_type& alloc) : x{list, alloc} {}

    inefficient_para_copy(const inefficient_para_copy&) = default;

    inefficient_para_copy(const inefficient_para_copy& other, const allocator_type& alloc)
      : x(alloc)
    {
      x.reserve(1);
      x.shrink_to_fit();
      x.reserve(other.x.size());
      std::copy(other.x.cbegin(), other.x.cend(), std::back_inserter(x)); 
    }

    inefficient_para_copy(inefficient_para_copy&&) noexcept = default;

    inefficient_para_copy(inefficient_para_copy&& other, const allocator_type& alloc) : x(std::move(other.x), alloc) {}

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
      
    inefficient_move(std::initializer_list<T> list, const allocator_type& alloc) : x{list, alloc} {}

    inefficient_move(const inefficient_move& other) = default;

    inefficient_move(const inefficient_move& other, const allocator_type& alloc)
      : x(other.x, alloc)
    {}

    inefficient_move(inefficient_move&& other) : x{std::move(other.x)}
    {
      x.reserve(x.capacity() + 1);
    }

    inefficient_move(inefficient_move&& other, const allocator_type& alloc) : x(std::move(other.x), alloc) {}

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
      
    inefficient_para_move(std::initializer_list<T> list, const allocator_type& alloc) : x{list, alloc} {}

    inefficient_para_move(const inefficient_para_move& other) = default;

    inefficient_para_move(const inefficient_para_move& other, const allocator_type& alloc)
      : x(other.x, alloc)
    {}

    inefficient_para_move(inefficient_para_move&& other) = default;

    inefficient_para_move(inefficient_para_move&& other, const allocator_type& alloc) : x(std::move(other.x), alloc)
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
}
