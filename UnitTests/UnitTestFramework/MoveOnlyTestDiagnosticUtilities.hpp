////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "UnitTestCore.hpp"

namespace sequoia::unit_testing
{
  template<class T=int, class Allocator=std::allocator<int>>
  struct move_only_broken_equality
  {
    using allocator_type = Allocator;

    move_only_broken_equality(std::initializer_list<T> list) : x{list} {}
      
    move_only_broken_equality(std::initializer_list<T> list, const allocator_type& alloc) : x{list, alloc} {}

    move_only_broken_equality(const move_only_broken_equality&) = delete;

    move_only_broken_equality(move_only_broken_equality&&) noexcept = default;

    move_only_broken_equality(move_only_broken_equality&& other, const allocator_type& alloc) : x(std::move(other.x), alloc) {}

    move_only_broken_equality& operator=(const move_only_broken_equality&) = delete;

    move_only_broken_equality& operator=(move_only_broken_equality&&) = default;

    friend void swap(move_only_broken_equality& lhs, move_only_broken_equality& rhs)
    {
      std::swap(lhs.x, rhs.x);
    }
      
    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const move_only_broken_equality& lhs, const move_only_broken_equality& rhs) noexcept
    {
      return lhs.x != rhs.x;
    }

    [[nodiscard]]
    friend bool operator!=(const move_only_broken_equality& lhs, const move_only_broken_equality& rhs) noexcept
    {
      return lhs.x != rhs.x;
    }

    template<class Stream>
    friend Stream& operator<<(Stream& s, const move_only_broken_equality& b)
    {
      for(auto i : b.x) s << i << ' ';
      return s;
    }
  };

  template<class T=int, class Allocator=std::allocator<int>>
  struct move_only_broken_inequality
  {
    using allocator_type = Allocator;

    move_only_broken_inequality(std::initializer_list<T> list) : x{list} {}

    move_only_broken_inequality(std::initializer_list<T> list, const allocator_type& alloc) : x{list, alloc} {}

    move_only_broken_inequality(const move_only_broken_inequality&) = delete;

    move_only_broken_inequality(move_only_broken_inequality&&) noexcept = default;

    move_only_broken_inequality(move_only_broken_inequality&& other, const allocator_type& alloc) : x(std::move(other.x), alloc) {}

    move_only_broken_inequality& operator=(const move_only_broken_inequality&) = delete;

    move_only_broken_inequality& operator=(move_only_broken_inequality&&) = default;

    friend void swap(move_only_broken_inequality& lhs, move_only_broken_inequality& rhs)
    {
      std::swap(lhs.x, rhs.x);
    }

    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const move_only_broken_inequality& lhs, const move_only_broken_inequality& rhs) noexcept
    {
      return lhs.x != rhs.x;
    }

    [[nodiscard]]
    friend bool operator!=(const move_only_broken_inequality& lhs, const move_only_broken_inequality& rhs) noexcept
    {
      return lhs.x == rhs.x;
    }

    template<class Stream>
    friend Stream& operator<<(Stream& s, const move_only_broken_inequality& b)
    {
      for(auto i : b.x) s << i << ' ';
      return s;
    }
  };

  template<class T=int, class Allocator=std::allocator<int>>
  struct move_only_broken_move
  {
    using allocator_type = Allocator;

    move_only_broken_move(std::initializer_list<T> list) : x{list} {}

    move_only_broken_move(std::initializer_list<T> list, const allocator_type& alloc) : x{list, alloc} {}

    move_only_broken_move(const move_only_broken_move&) = delete;

    move_only_broken_move(move_only_broken_move&&)      
    {
      // Do nothing
    }

    move_only_broken_move(move_only_broken_move&& other, const allocator_type& alloc) : x(std::move(other.x), alloc)
    {}
      
    move_only_broken_move& operator=(const move_only_broken_move&) = delete;

    move_only_broken_move& operator=(move_only_broken_move&&) = default;

    friend void swap(move_only_broken_move& lhs, move_only_broken_move& rhs)
    {
      std::swap(lhs.x, rhs.x);
    }
      
    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const move_only_broken_move& lhs, const move_only_broken_move& rhs) noexcept
    {
      return lhs.x == rhs.x;
    }

    [[nodiscard]]
    friend bool operator!=(const move_only_broken_move& lhs, const move_only_broken_move& rhs) noexcept
    {
      return !(lhs == rhs);
    }

    template<class Stream>
    friend Stream& operator<<(Stream& s, const move_only_broken_move& b)
    {
      for(auto i : b.x) s << i << ' ';
      return s;
    }
  };

  template<class T=int, class Allocator=std::allocator<int>>
  struct move_only_broken_move_assignment
  {
    using allocator_type = Allocator;

    move_only_broken_move_assignment(std::initializer_list<T> list) : x{list} {}

    move_only_broken_move_assignment(std::initializer_list<T> list, const allocator_type& alloc) : x{list, alloc} {}

    move_only_broken_move_assignment(const move_only_broken_move_assignment&) = delete;

    move_only_broken_move_assignment(move_only_broken_move_assignment&&) = default;

    move_only_broken_move_assignment(move_only_broken_move_assignment&& other, const allocator_type& alloc) : x(std::move(other.x), alloc) {}
      
    move_only_broken_move_assignment& operator=(const move_only_broken_move_assignment&) = delete;
      
    move_only_broken_move_assignment& operator=(move_only_broken_move_assignment&&)
    {
      return *this;
    }

    friend void swap(move_only_broken_move_assignment& lhs, move_only_broken_move_assignment& rhs)
    {
      std::swap(lhs.x, rhs.x);
    }
      
    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const move_only_broken_move_assignment& lhs, const move_only_broken_move_assignment& rhs) noexcept
    {
      return lhs.x == rhs.x;
    }

    [[nodiscard]]
    friend bool operator!=(const move_only_broken_move_assignment& lhs, const move_only_broken_move_assignment& rhs) noexcept
    {
      return !(lhs == rhs);
    }

    template<class Stream>
    friend Stream& operator<<(Stream& s, const move_only_broken_move_assignment& b)
    {
      for(auto i : b.x) s << i << ' ';
      return s;
    }
  };

  template<class T=int, class Allocator=std::allocator<int>>
  struct move_only_broken_swap
  {
    using allocator_type = Allocator;

    move_only_broken_swap(std::initializer_list<T> list) : x{list} {}

    move_only_broken_swap(std::initializer_list<T> list, const allocator_type& a) : x{list, a} {}

    move_only_broken_swap(const move_only_broken_swap&) = delete;

    move_only_broken_swap(move_only_broken_swap&&) noexcept = default;

    move_only_broken_swap(move_only_broken_swap&& other, const allocator_type& alloc) : x(std::move(other.x), alloc) {}

    move_only_broken_swap& operator=(const move_only_broken_swap&) = delete;

    move_only_broken_swap& operator=(move_only_broken_swap&&) = default;

    friend void swap(move_only_broken_swap&, move_only_broken_swap&) noexcept
    {
      // do nothing
    }
      
    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const move_only_broken_swap& lhs, const move_only_broken_swap& rhs) noexcept
    {
      return lhs.x == rhs.x;
    }

    [[nodiscard]]
    friend bool operator!=(const move_only_broken_swap& lhs, const move_only_broken_swap& rhs) noexcept
    {
      return !(lhs == rhs);
    }

    template<class Stream>
    friend Stream& operator<<(Stream& s, const move_only_broken_swap& b)
    {
      for(auto i : b.x) s << i << ' ';
      return s;
    }
  };

  template<class T=int, class Allocator=std::allocator<int>>
  struct move_only_beast
  {
    using allocator_type = Allocator;

    move_only_beast(std::initializer_list<T> list) : x{list} {}

    move_only_beast(std::initializer_list<T> list, const allocator_type& a) : x{list, a} {}

    move_only_beast(const allocator_type& a) : x(a) {}

    move_only_beast(const move_only_beast&) = delete;

    move_only_beast(move_only_beast&&) noexcept = default;

    move_only_beast(move_only_beast&& other, const allocator_type& alloc) : x(std::move(other.x), alloc) {}

    move_only_beast& operator=(const move_only_beast&) = delete;

    move_only_beast& operator=(move_only_beast&&) = default;

    void swap(move_only_beast& other) noexcept(noexcept(std::swap(x, other.x)))
    {
      std::swap(x, other.x);
    }

    friend void swap(move_only_beast& lhs, move_only_beast& rhs)
      noexcept(noexcept(lhs.swap(rhs)))
    {
      lhs.swap(rhs);
    }
      
    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const move_only_beast& lhs, const move_only_beast& rhs) noexcept
    {
      return lhs.x == rhs.x;
    }

    [[nodiscard]]
    friend bool operator!=(const move_only_beast& lhs, const move_only_beast& rhs) noexcept
    {
      return !(lhs == rhs);
    }

    template<class Stream>
    friend Stream& operator<<(Stream& s, const move_only_beast& b)
    {
      for(auto i : b.x) s << i << ' ';
      return s;
    }
  };
}
