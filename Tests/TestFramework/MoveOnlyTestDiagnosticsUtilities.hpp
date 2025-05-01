////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "CommonMoveOnlyTestDiagnosticsUtilities.hpp"

namespace sequoia::testing
{  
  template<enable_serialization EnableSerialization>
  class resource_binder
  {
  public:
    resource_binder() = default;

    explicit resource_binder(int i)
      : m_Index{i}
    {}

    resource_binder(resource_binder&& other) noexcept
      : m_Index{std::exchange(other.m_Index, 0)}
    {}

    resource_binder& operator=(resource_binder&& other) noexcept
    {
      std::ranges::swap(m_Index, other.m_Index);
      return *this;
    }

    [[nodiscard]]
    int index() const noexcept
    {
      return m_Index;
    }

    [[nodiscard]]
    friend bool operator==(const resource_binder&, const resource_binder&) = default;

    template<class Stream>
      requires (EnableSerialization == enable_serialization::yes)
    friend Stream& operator<<(Stream& s, const resource_binder& b)
    {
      s << b.index();
      return s;
    }

    template<class Stream>
      requires (EnableSerialization == enable_serialization::yes)
    friend Stream& operator>>(Stream& s, resource_binder& b)
    {
      s >> b.m_Index;
      return s;
    }
  private:
    int m_Index{-1};
  };

  template<class T=int, class Allocator=std::allocator<int>>
  struct move_only_beast
  {
    using value_type     = T;
    using allocator_type = Allocator;

    move_only_beast(std::initializer_list<T> list) : x{list} {}

    move_only_beast(std::initializer_list<T> list, const allocator_type& a) : x(list, a) {}

    move_only_beast(const allocator_type& a) : x(a) {}

    move_only_beast(const move_only_beast&) = delete;

    move_only_beast(move_only_beast&&) noexcept = default;

    move_only_beast(move_only_beast&& other, const allocator_type& a) : x(std::move(other.x), a) {}

    move_only_beast& operator=(const move_only_beast&) = delete;

    move_only_beast& operator=(move_only_beast&&) = default;

    void swap(move_only_beast& other) noexcept(noexcept(std::ranges::swap(this->x, other.x)))
    {
      std::ranges::swap(x, other.x);
    }

    friend void swap(move_only_beast& lhs, move_only_beast& rhs)
      noexcept(noexcept(lhs.swap(rhs)))
    {
      lhs.swap(rhs);
    }

    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const move_only_beast&, const move_only_beast&) noexcept = default;

    template<class Stream>
    friend Stream& operator<<(Stream& s, const move_only_beast& b)
    {
      for(const auto& i : b.x) s << i << '\n';
      return s;
    }
  };

  template<class T, class Allocator>
  struct value_tester<move_only_beast<T, Allocator>> : move_only_beast_value_tester<move_only_beast<T, Allocator>> {};

  template<class T = int, class Allocator = std::allocator<int>>
  struct specified_moved_from_beast
  {
    using     value_type = T;
    using allocator_type = Allocator;

    specified_moved_from_beast(std::initializer_list<T> list) : x{list} {}

    specified_moved_from_beast(std::initializer_list<T> list, const allocator_type& a) : x(list, a) {}

    specified_moved_from_beast(const allocator_type& a) : x(a) {}

    specified_moved_from_beast(const specified_moved_from_beast&) = delete;

    specified_moved_from_beast(specified_moved_from_beast&& other) noexcept
      : x{std::move(other.x)}
    {
      other.x.clear();
    }

    specified_moved_from_beast(specified_moved_from_beast&& other, const allocator_type& a) : x(std::move(other.x), a) {}

    specified_moved_from_beast& operator=(const specified_moved_from_beast&) = delete;

    specified_moved_from_beast& operator=(specified_moved_from_beast&& other)
    {
      x = std::move(other.x);
      other.x.clear();

      return *this;
    }

    void swap(specified_moved_from_beast& other) noexcept(noexcept(std::ranges::swap(this->x, other.x)))
    {
      std::ranges::swap(x, other.x);
    }

    friend void swap(specified_moved_from_beast& lhs, specified_moved_from_beast& rhs)
      noexcept(noexcept(lhs.swap(rhs)))
    {
      lhs.swap(rhs);
    }

    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const specified_moved_from_beast&, const specified_moved_from_beast&) noexcept = default;

    template<class Stream>
    friend Stream& operator<<(Stream& s, const specified_moved_from_beast& b)
    {
      for(const auto& i : b.x) s << i << '\n';
      return s;
    }
  };

  template<class T, class Allocator>
  struct value_tester<specified_moved_from_beast<T, Allocator>> : move_only_beast_value_tester<specified_moved_from_beast<T, Allocator>> {};

  template<class T=int, class Allocator=std::allocator<int>>
  struct move_only_broken_equality
  {
    using value_type     = T;
    using allocator_type = Allocator;

    move_only_broken_equality(std::initializer_list<T> list) : x{list} {}

    move_only_broken_equality(std::initializer_list<T> list, const allocator_type& a) : x(list, a) {}

    move_only_broken_equality(const move_only_broken_equality&) = delete;

    move_only_broken_equality(move_only_broken_equality&&) noexcept = default;

    move_only_broken_equality(move_only_broken_equality&& other, const allocator_type& a) : x(std::move(other.x), a) {}

    move_only_broken_equality& operator=(const move_only_broken_equality&) = delete;

    move_only_broken_equality& operator=(move_only_broken_equality&&) = default;

    friend void swap(move_only_broken_equality& lhs, move_only_broken_equality& rhs)
    {
      std::ranges::swap(lhs.x, rhs.x);
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

  template<class T, class Allocator>
  struct value_tester<move_only_broken_equality<T, Allocator>> : move_only_beast_value_tester<move_only_broken_equality<T, Allocator>> {};

  template<class T=int, class Allocator=std::allocator<int>>
  struct move_only_broken_inequality
  {
    using     value_type = T;
    using allocator_type = Allocator;

    move_only_broken_inequality(std::initializer_list<T> list) : x{list} {}

    move_only_broken_inequality(std::initializer_list<T> list, const allocator_type& a) : x(list, a) {}

    move_only_broken_inequality(const move_only_broken_inequality&) = delete;

    move_only_broken_inequality(move_only_broken_inequality&&) noexcept = default;

    move_only_broken_inequality(move_only_broken_inequality&& other, const allocator_type& a) : x(std::move(other.x), a) {}

    move_only_broken_inequality& operator=(const move_only_broken_inequality&) = delete;

    move_only_broken_inequality& operator=(move_only_broken_inequality&&) = default;

    friend void swap(move_only_broken_inequality& lhs, move_only_broken_inequality& rhs)
    {
      std::ranges::swap(lhs.x, rhs.x);
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

  template<class T, class Allocator>
  struct value_tester<move_only_broken_inequality<T, Allocator>> : move_only_beast_value_tester<move_only_broken_inequality<T, Allocator>> {};

  template<class T=int, class Allocator=std::allocator<int>>
  struct move_only_broken_move
  {
    using value_type     = T;
    using allocator_type = Allocator;

    move_only_broken_move(std::initializer_list<T> list) : x{list} {}

    move_only_broken_move(std::initializer_list<T> list, const allocator_type& a) : x(list, a) {}

    move_only_broken_move(const move_only_broken_move&) = delete;

    move_only_broken_move(move_only_broken_move&&) noexcept
    {
      // Do nothing
    }

    move_only_broken_move(move_only_broken_move&& other, const allocator_type& a) : x(std::move(other.x), a)
    {}

    move_only_broken_move& operator=(const move_only_broken_move&) = delete;

    move_only_broken_move& operator=(move_only_broken_move&&) = default;

    friend void swap(move_only_broken_move& lhs, move_only_broken_move& rhs)
    {
      std::ranges::swap(lhs.x, rhs.x);
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

  template<class T, class Allocator>
  struct value_tester<move_only_broken_move<T, Allocator>> : move_only_beast_value_tester<move_only_broken_move<T, Allocator>> {};

  template<class T=int, class Allocator=std::allocator<int>>
  struct move_only_broken_move_assignment
  {
    using value_type     = T;
    using allocator_type = Allocator;

    move_only_broken_move_assignment(std::initializer_list<T> list) : x{list} {}

    move_only_broken_move_assignment(std::initializer_list<T> list, const allocator_type& a) : x(list, a) {}

    move_only_broken_move_assignment(const move_only_broken_move_assignment&) = delete;

    move_only_broken_move_assignment(move_only_broken_move_assignment&&) = default;

    move_only_broken_move_assignment(move_only_broken_move_assignment&& other, const allocator_type& a) : x(std::move(other.x), a) {}

    move_only_broken_move_assignment& operator=(const move_only_broken_move_assignment&) = delete;

    move_only_broken_move_assignment& operator=(move_only_broken_move_assignment&&) noexcept
    {
      return *this;
    }

    friend void swap(move_only_broken_move_assignment& lhs, move_only_broken_move_assignment& rhs)
    {
      std::ranges::swap(lhs.x, rhs.x);
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

  template<class T, class Allocator>
  struct value_tester<move_only_broken_move_assignment<T, Allocator>>
    : move_only_beast_value_tester<move_only_broken_move_assignment<T, Allocator>>
  {};

  template<class T=int, class Allocator=std::allocator<int>>
  struct move_only_broken_swap
  {
    using value_type     = T;
    using allocator_type = Allocator;

    move_only_broken_swap(std::initializer_list<T> list) : x{list} {}

    move_only_broken_swap(std::initializer_list<T> list, const allocator_type& a) : x(list, a) {}

    move_only_broken_swap(const move_only_broken_swap&) = delete;

    move_only_broken_swap(move_only_broken_swap&&) noexcept = default;

    move_only_broken_swap(move_only_broken_swap&& other, const allocator_type& a) : x(std::move(other.x), a) {}

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

  template<class T, class Allocator>
  struct value_tester<move_only_broken_swap<T, Allocator>> : move_only_beast_value_tester<move_only_broken_swap<T, Allocator>> {};
  
  template<class T=int, class Allocator=std::allocator<int>>
  struct move_only_inefficient_move
  {
    using value_type     = T;
    using allocator_type = Allocator;

    move_only_inefficient_move(std::initializer_list<T> list) : x{list} {}

    move_only_inefficient_move(std::initializer_list<T> list, const allocator_type& a) : x(list, a) {}

    move_only_inefficient_move(const allocator_type& a) : x(a) {}

    move_only_inefficient_move(const move_only_inefficient_move&) = delete;

    move_only_inefficient_move(move_only_inefficient_move&& other) : x{std::move(other.x)}
    {
      x.reserve(x.capacity() + 10);
    }

    move_only_inefficient_move(move_only_inefficient_move&& other, const allocator_type& a) : x(std::move(other.x), a) {}

    move_only_inefficient_move& operator=(const move_only_inefficient_move&) = delete;

    move_only_inefficient_move& operator=(move_only_inefficient_move&&) = default;

    void swap(move_only_inefficient_move& other) noexcept(noexcept(std::ranges::swap(this->x, other.x)))
    {
      std::ranges::swap(x, other.x);
    }

    friend void swap(move_only_inefficient_move& lhs, move_only_inefficient_move& rhs)
      noexcept(noexcept(lhs.swap(rhs)))
    {
      lhs.swap(rhs);
    }

    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const move_only_inefficient_move& lhs, const move_only_inefficient_move& rhs) noexcept
    {
      return lhs.x == rhs.x;
    }

    [[nodiscard]]
    friend bool operator!=(const move_only_inefficient_move& lhs, const move_only_inefficient_move& rhs) noexcept
    {
      return !(lhs == rhs);
    }

    template<class Stream>
    friend Stream& operator<<(Stream& s, const move_only_inefficient_move& b)
    {
      for(auto i : b.x) s << i << ' ';
      return s;
    }
  };

  template<class T, class Allocator>
  struct value_tester<move_only_inefficient_move<T, Allocator>> : move_only_beast_value_tester<move_only_inefficient_move<T, Allocator>> {};
  
  template<class T=int, class Allocator=std::allocator<int>>
  struct move_only_inefficient_move_assignment
  {
    using value_type     = T;
    using allocator_type = Allocator;

    move_only_inefficient_move_assignment(std::initializer_list<T> list) : x{list} {}

    move_only_inefficient_move_assignment(std::initializer_list<T> list, const allocator_type& a)
      : x(list, a)
    {}

    move_only_inefficient_move_assignment(const allocator_type& a) : x(a) {}

    move_only_inefficient_move_assignment(const move_only_inefficient_move_assignment&) = delete;

    move_only_inefficient_move_assignment(move_only_inefficient_move_assignment&&) noexcept = default;

    move_only_inefficient_move_assignment(move_only_inefficient_move_assignment&& other, const allocator_type& a) : x(std::move(other.x), a) {}

    move_only_inefficient_move_assignment& operator=(const move_only_inefficient_move_assignment&) = delete;

    move_only_inefficient_move_assignment& operator=(move_only_inefficient_move_assignment&& other)
    {
      x = std::move(other.x);
      x.reserve(x.capacity() + 1);

      return *this;
    }

    void swap(move_only_inefficient_move_assignment& other) noexcept(noexcept(std::ranges::swap(this->x, other.x)))
    {
      std::ranges::swap(x, other.x);
    }

    friend void swap(move_only_inefficient_move_assignment& lhs, move_only_inefficient_move_assignment& rhs)
      noexcept(noexcept(lhs.swap(rhs)))
    {
      lhs.swap(rhs);
    }

    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const move_only_inefficient_move_assignment& lhs, const move_only_inefficient_move_assignment& rhs) noexcept
    {
      return lhs.x == rhs.x;
    }

    [[nodiscard]]
    friend bool operator!=(const move_only_inefficient_move_assignment& lhs, const move_only_inefficient_move_assignment& rhs) noexcept
    {
      return !(lhs == rhs);
    }

    template<class Stream>
    friend Stream& operator<<(Stream& s, const move_only_inefficient_move_assignment& b)
    {
      for(auto i : b.x) s << i << ' ';
      return s;
    }
  };

  template<class T, class Allocator>
  struct value_tester<move_only_inefficient_move_assignment<T, Allocator>>
    : move_only_beast_value_tester<move_only_inefficient_move_assignment<T, Allocator>>
  {};
}
