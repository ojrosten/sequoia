////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/MoveOnlyTestCore.hpp"

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

    orderable_move_only_beast(orderable_move_only_beast&& other, const allocator_type& a) : x(std::move(other.x), a) {}

    orderable_move_only_beast& operator=(const orderable_move_only_beast&) = delete;

    orderable_move_only_beast& operator=(orderable_move_only_beast&&) = default;

    void swap(orderable_move_only_beast& other) noexcept(noexcept(std::ranges::swap(this->x, other.x)))
    {
      std::ranges::swap(x, other.x);
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



  class orderable_resource_binder
  {
  public:
    orderable_resource_binder() = default;

    explicit orderable_resource_binder(int i)
      : m_Index{i}
    {}

    orderable_resource_binder(orderable_resource_binder&& other) noexcept
      : m_Index{std::exchange(other.m_Index, 0)}
    {}

    orderable_resource_binder& operator=(orderable_resource_binder&& other) noexcept
    {
      m_Index = std::exchange(other.m_Index, 0);
      return *this;
    }

    [[nodiscard]]
    int index() const noexcept
    {
      return m_Index;
    }

    [[nodiscard]]
    friend auto operator<=>(const orderable_resource_binder&, const orderable_resource_binder&) = default;

    template<class Stream>
    friend Stream& operator<<(Stream& s, const orderable_resource_binder& b)
    {
      s << b.index();
      return s;
    }

    template<class Stream>
    friend Stream& operator>>(Stream& s, orderable_resource_binder& b)
    {
      s >> b.m_Index;
      return s;
    }
  private:
    int m_Index{-1};
  };

  template<class T=int, class Allocator=std::allocator<int>>
  struct move_only_broken_less
  {
    using allocator_type = Allocator;

    move_only_broken_less(std::initializer_list<T> list) : x{list} {}

    move_only_broken_less(std::initializer_list<T> list, const allocator_type& a) : x{list, a} {}

    move_only_broken_less(const allocator_type& a) : x(a) {}

    move_only_broken_less(const move_only_broken_less&) = delete;

    move_only_broken_less(move_only_broken_less&&) noexcept = default;

    move_only_broken_less(move_only_broken_less&& other, const allocator_type& a) : x(std::move(other.x), a) {}

    move_only_broken_less& operator=(const move_only_broken_less&) = delete;

    move_only_broken_less& operator=(move_only_broken_less&&) = default;

    void swap(move_only_broken_less& other) noexcept(noexcept(std::ranges::swap(this->x, other.x)))
    {
      std::ranges::swap(x, other.x);
    }

    friend void swap(move_only_broken_less& lhs, move_only_broken_less& rhs)
      noexcept(noexcept(lhs.swap(rhs)))
    {
      lhs.swap(rhs);
    }

    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const move_only_broken_less&, const move_only_broken_less&) noexcept = default;

    [[nodiscard]]
    friend bool operator<(const move_only_broken_less& lhs, const move_only_broken_less& rhs) noexcept
    {
      return lhs.x > rhs.x;
    }

    [[nodiscard]]
    friend bool operator<=(const move_only_broken_less& lhs, const move_only_broken_less& rhs) noexcept
    {
      return lhs.x <= rhs.x;
    }

    [[nodiscard]]
    friend bool operator>(const move_only_broken_less& lhs, const move_only_broken_less& rhs) noexcept
    {
      return lhs.x > rhs.x;
    }

    [[nodiscard]]
    friend bool operator>=(const move_only_broken_less& lhs, const move_only_broken_less& rhs) noexcept
    {
      return lhs.x >= rhs.x;
    }

    // TO DO: default this and remove most of the above when libc++ rolls out <=> to std
    friend std::weak_ordering operator<=>(const move_only_broken_less& lhs, const move_only_broken_less& rhs) noexcept
    {
      if(lhs < rhs) return std::weak_ordering::less;

      if(rhs > lhs) return std::weak_ordering::greater;

      return std::weak_ordering::equivalent;
    }

    template<class Stream>
    friend Stream& operator<<(Stream& s, const move_only_broken_less& b)
    {
      for(auto i : b.x) s << i << ' ';
      return s;
    }
  };

  template<class T=int, class Allocator=std::allocator<int>>
  struct move_only_broken_lesseq
  {
    using allocator_type = Allocator;

    move_only_broken_lesseq(std::initializer_list<T> list) : x{list} {}

    move_only_broken_lesseq(std::initializer_list<T> list, const allocator_type& a) : x{list, a} {}

    move_only_broken_lesseq(const allocator_type& a) : x(a) {}

    move_only_broken_lesseq(const move_only_broken_lesseq&) = delete;

    move_only_broken_lesseq(move_only_broken_lesseq&&) noexcept = default;

    move_only_broken_lesseq(move_only_broken_lesseq&& other, const allocator_type& a) : x(std::move(other.x), a) {}

    move_only_broken_lesseq& operator=(const move_only_broken_lesseq&) = delete;

    move_only_broken_lesseq& operator=(move_only_broken_lesseq&&) = default;

    void swap(move_only_broken_lesseq& other) noexcept(noexcept(std::ranges::swap(this->x, other.x)))
    {
      std::ranges::swap(x, other.x);
    }

    friend void swap(move_only_broken_lesseq& lhs, move_only_broken_lesseq& rhs)
      noexcept(noexcept(lhs.swap(rhs)))
    {
      lhs.swap(rhs);
    }

    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const move_only_broken_lesseq&, const move_only_broken_lesseq&) noexcept = default;

    [[nodiscard]]
    friend bool operator<(const move_only_broken_lesseq& lhs, const move_only_broken_lesseq& rhs) noexcept
    {
      return lhs.x < rhs.x;
    }

    [[nodiscard]]
    friend bool operator<=(const move_only_broken_lesseq& lhs, const move_only_broken_lesseq& rhs) noexcept
    {
      return lhs.x >= rhs.x;
    }

    [[nodiscard]]
    friend bool operator>(const move_only_broken_lesseq& lhs, const move_only_broken_lesseq& rhs) noexcept
    {
      return lhs.x > rhs.x;
    }

    [[nodiscard]]
    friend bool operator>=(const move_only_broken_lesseq& lhs, const move_only_broken_lesseq& rhs) noexcept
    {
      return lhs.x >= rhs.x;
    }

    // TO DO: default this and remove most of the above when libc++ rolls out <=> to std
    friend std::weak_ordering operator<=>(const move_only_broken_lesseq& lhs, const move_only_broken_lesseq& rhs) noexcept
    {
      if(lhs < rhs) return std::weak_ordering::less;

      if(rhs > lhs) return std::weak_ordering::greater;

      return std::weak_ordering::equivalent;
    }

    template<class Stream>
    friend Stream& operator<<(Stream& s, const move_only_broken_lesseq& b)
    {
      for(auto i : b.x) s << i << ' ';
      return s;
    }
  };

  template<class T=int, class Allocator=std::allocator<int>>
  struct move_only_broken_greater
  {
    using allocator_type = Allocator;

    move_only_broken_greater(std::initializer_list<T> list) : x{list} {}

    move_only_broken_greater(std::initializer_list<T> list, const allocator_type& a) : x{list, a} {}

    move_only_broken_greater(const allocator_type& a) : x(a) {}

    move_only_broken_greater(const move_only_broken_greater&) = delete;

    move_only_broken_greater(move_only_broken_greater&&) noexcept = default;

    move_only_broken_greater(move_only_broken_greater&& other, const allocator_type& a) : x(std::move(other.x), a) {}

    move_only_broken_greater& operator=(const move_only_broken_greater&) = delete;

    move_only_broken_greater& operator=(move_only_broken_greater&&) = default;

    void swap(move_only_broken_greater& other) noexcept(noexcept(std::ranges::swap(this->x, other.x)))
    {
      std::ranges::swap(x, other.x);
    }

    friend void swap(move_only_broken_greater& lhs, move_only_broken_greater& rhs)
      noexcept(noexcept(lhs.swap(rhs)))
    {
      lhs.swap(rhs);
    }

    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const move_only_broken_greater&, const move_only_broken_greater&) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const move_only_broken_greater&, const move_only_broken_greater&) noexcept = default;

    [[nodiscard]]
    friend bool operator<(const move_only_broken_greater& lhs, const move_only_broken_greater& rhs) noexcept
    {
      return lhs.x < rhs.x;
    }

    [[nodiscard]]
    friend bool operator<=(const move_only_broken_greater& lhs, const move_only_broken_greater& rhs) noexcept
    {
      return lhs.x <= rhs.x;
    }

    [[nodiscard]]
    friend bool operator>(const move_only_broken_greater& lhs, const move_only_broken_greater& rhs) noexcept
    {
      return lhs.x < rhs.x;
    }

    [[nodiscard]]
    friend bool operator>=(const move_only_broken_greater& lhs, const move_only_broken_greater& rhs) noexcept
    {
      return lhs.x >= rhs.x;
    }

    // TO DO: default this and remove most of the above when libc++ rolls out <=> to std
    friend std::weak_ordering operator<=>(const move_only_broken_greater& lhs, const move_only_broken_greater& rhs) noexcept
    {
      if(lhs < rhs) return std::weak_ordering::less;

      if(rhs > lhs) return std::weak_ordering::greater;

      return std::weak_ordering::equivalent;
    }

    template<class Stream>
    friend Stream& operator<<(Stream& s, const move_only_broken_greater& b)
    {
      for(auto i : b.x) s << i << ' ';
      return s;
    }
  };

  template<class T=int, class Allocator=std::allocator<int>>
  struct move_only_broken_greatereq
  {
    using allocator_type = Allocator;

    move_only_broken_greatereq(std::initializer_list<T> list) : x{list} {}

    move_only_broken_greatereq(std::initializer_list<T> list, const allocator_type& a) : x{list, a} {}

    move_only_broken_greatereq(const allocator_type& a) : x(a) {}

    move_only_broken_greatereq(const move_only_broken_greatereq&) = delete;

    move_only_broken_greatereq(move_only_broken_greatereq&&) noexcept = default;

    move_only_broken_greatereq(move_only_broken_greatereq&& other, const allocator_type& a) : x(std::move(other.x), a) {}

    move_only_broken_greatereq& operator=(const move_only_broken_greatereq&) = delete;

    move_only_broken_greatereq& operator=(move_only_broken_greatereq&&) = default;

    void swap(move_only_broken_greatereq& other) noexcept(noexcept(std::ranges::swap(this->x, other.x)))
    {
      std::ranges::swap(x, other.x);
    }

    friend void swap(move_only_broken_greatereq& lhs, move_only_broken_greatereq& rhs)
      noexcept(noexcept(lhs.swap(rhs)))
    {
      lhs.swap(rhs);
    }

    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const move_only_broken_greatereq&, const move_only_broken_greatereq&) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const move_only_broken_greatereq&, const move_only_broken_greatereq&) noexcept = default;

    [[nodiscard]]
    friend bool operator<(const move_only_broken_greatereq& lhs, const move_only_broken_greatereq& rhs) noexcept
    {
      return lhs.x < rhs.x;
    }

    [[nodiscard]]
    friend bool operator<=(const move_only_broken_greatereq& lhs, const move_only_broken_greatereq& rhs) noexcept
    {
      return lhs.x >= rhs.x;
    }

    [[nodiscard]]
    friend bool operator>(const move_only_broken_greatereq& lhs, const move_only_broken_greatereq& rhs) noexcept
    {
      return lhs.x > rhs.x;
    }

    [[nodiscard]]
    friend bool operator>=(const move_only_broken_greatereq& lhs, const move_only_broken_greatereq& rhs) noexcept
    {
      return lhs.x >= rhs.x;
    }

    // TO DO: default this and remove most of the above when libc++ rolls out <=> to std
    friend std::weak_ordering operator<=>(const move_only_broken_greatereq& lhs, const move_only_broken_greatereq& rhs) noexcept
    {
      if(lhs < rhs) return std::weak_ordering::less;

      if(rhs > lhs) return std::weak_ordering::greater;

      return std::weak_ordering::equivalent;
    }

    template<class Stream>
    friend Stream& operator<<(Stream& s, const move_only_broken_greatereq& b)
    {
      for(auto i : b.x) s << i << ' ';
      return s;
    }
  };

  template<class T=int, class Allocator=std::allocator<int>>
  struct move_only_inverted_comparisons
  {
    using allocator_type = Allocator;

    move_only_inverted_comparisons(std::initializer_list<T> list) : x{list} {}

    move_only_inverted_comparisons(std::initializer_list<T> list, const allocator_type& a) : x{list, a} {}

    move_only_inverted_comparisons(const allocator_type& a) : x(a) {}

    move_only_inverted_comparisons(const move_only_inverted_comparisons&) = delete;

    move_only_inverted_comparisons(move_only_inverted_comparisons&&) noexcept = default;

    move_only_inverted_comparisons(move_only_inverted_comparisons&& other, const allocator_type& a) : x(std::move(other.x), a) {}

    move_only_inverted_comparisons& operator=(const move_only_inverted_comparisons&) = delete;

    move_only_inverted_comparisons& operator=(move_only_inverted_comparisons&&) = default;

    void swap(move_only_inverted_comparisons& other) noexcept(noexcept(std::ranges::swap(this->x, other.x)))
    {
      std::ranges::swap(x, other.x);
    }

    friend void swap(move_only_inverted_comparisons& lhs, move_only_inverted_comparisons& rhs)
      noexcept(noexcept(lhs.swap(rhs)))
    {
      lhs.swap(rhs);
    }

    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const move_only_inverted_comparisons&, const move_only_inverted_comparisons&) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const move_only_inverted_comparisons&, const move_only_inverted_comparisons&) noexcept = default;

    [[nodiscard]]
    friend bool operator<(const move_only_inverted_comparisons& lhs, const move_only_inverted_comparisons& rhs) noexcept
    {
      return lhs.x > rhs.x;
    }

    [[nodiscard]]
    friend bool operator<=(const move_only_inverted_comparisons& lhs, const move_only_inverted_comparisons& rhs) noexcept
    {
      return lhs.x >= rhs.x;
    }

    [[nodiscard]]
    friend bool operator>(const move_only_inverted_comparisons& lhs, const move_only_inverted_comparisons& rhs) noexcept
    {
      return lhs.x < rhs.x;
    }

    [[nodiscard]]
    friend bool operator>=(const move_only_inverted_comparisons& lhs, const move_only_inverted_comparisons& rhs) noexcept
    {
      return lhs.x <= rhs.x;
    }

    // TO DO: default this and remove most of the above when libc++ rolls out <=> to std
    friend std::weak_ordering operator<=>(const move_only_inverted_comparisons& lhs, const move_only_inverted_comparisons& rhs) noexcept
    {
      if(lhs < rhs) return std::weak_ordering::less;

      if(rhs > lhs) return std::weak_ordering::greater;

      return std::weak_ordering::equivalent;
    }

    template<class Stream>
    friend Stream& operator<<(Stream& s, const move_only_inverted_comparisons& b)
    {
      for(auto i : b.x) s << i << ' ';
      return s;
    }
  };

  template<class T=int, class Allocator=std::allocator<int>>
  struct move_only_broken_spaceship
  {
    using allocator_type = Allocator;

    move_only_broken_spaceship(std::initializer_list<T> list) : x{list} {}

    move_only_broken_spaceship(std::initializer_list<T> list, const allocator_type& a) : x{list, a} {}

    move_only_broken_spaceship(const allocator_type& a) : x(a) {}

    move_only_broken_spaceship(const move_only_broken_spaceship&) = delete;

    move_only_broken_spaceship(move_only_broken_spaceship&&) noexcept = default;

    move_only_broken_spaceship(move_only_broken_spaceship&& other, const allocator_type& a) : x(std::move(other.x), a) {}

    move_only_broken_spaceship& operator=(const move_only_broken_spaceship&) = delete;

    move_only_broken_spaceship& operator=(move_only_broken_spaceship&&) = default;

    void swap(move_only_broken_spaceship& other) noexcept(noexcept(std::ranges::swap(this->x, other.x)))
    {
      std::ranges::swap(x, other.x);
    }

    friend void swap(move_only_broken_spaceship& lhs, move_only_broken_spaceship& rhs)
      noexcept(noexcept(lhs.swap(rhs)))
    {
      lhs.swap(rhs);
    }

    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const move_only_broken_spaceship&, const move_only_broken_spaceship&) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const move_only_broken_spaceship&, const move_only_broken_spaceship&) noexcept = default;

    [[nodiscard]]
    friend bool operator<(const move_only_broken_spaceship& lhs, const move_only_broken_spaceship& rhs) noexcept
    {
      return lhs.x < rhs.x;
    }

    [[nodiscard]]
    friend bool operator<=(const move_only_broken_spaceship& lhs, const move_only_broken_spaceship& rhs) noexcept
    {
      return lhs.x <= rhs.x;
    }

    [[nodiscard]]
    friend bool operator>(const move_only_broken_spaceship& lhs, const move_only_broken_spaceship& rhs) noexcept
    {
      return lhs.x > rhs.x;
    }

    [[nodiscard]]
    friend bool operator>=(const move_only_broken_spaceship& lhs, const move_only_broken_spaceship& rhs) noexcept
    {
      return lhs.x >= rhs.x;
    }

    // TO DO: default this and remove most of the above when libc++ rolls out <=> to std
    friend std::weak_ordering operator<=>(const move_only_broken_spaceship& lhs, const move_only_broken_spaceship& rhs) noexcept
    {
      if(lhs < rhs) return std::weak_ordering::greater;

      if(rhs > lhs) return std::weak_ordering::less;

      return std::weak_ordering::equivalent;
    }

    template<class Stream>
    friend Stream& operator<<(Stream& s, const move_only_broken_spaceship& b)
    {
      for(auto i : b.x) s << i << ' ';
      return s;
    }
  };
}
