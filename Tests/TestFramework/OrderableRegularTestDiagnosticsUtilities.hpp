////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/TestFramework/RegularTestCore.hpp"

#include <vector>

namespace sequoia::testing
{
  template<class T=int, class Allocator=std::allocator<int>>
  struct orderable_regular_beast
  {
    using allocator_type = Allocator;

    orderable_regular_beast(std::initializer_list<T> list) : x{list} {}

    orderable_regular_beast(std::initializer_list<T> list, const allocator_type& a) : x{list, a} {}

    orderable_regular_beast(const allocator_type& a) : x(a) {}

    orderable_regular_beast(const orderable_regular_beast&) = default;

    orderable_regular_beast(orderable_regular_beast&&) noexcept = default;

    orderable_regular_beast(orderable_regular_beast&& other, const allocator_type& a) : x(std::move(other.x), a) {}

    orderable_regular_beast& operator=(const orderable_regular_beast&) = default;

    orderable_regular_beast& operator=(orderable_regular_beast&&) = default;

    void swap(orderable_regular_beast& other) noexcept(noexcept(std::swap(this->x, other.x)))
    {
      std::swap(x, other.x);
    }

    friend void swap(orderable_regular_beast& lhs, orderable_regular_beast& rhs)
      noexcept(noexcept(lhs.swap(rhs)))
    {
      lhs.swap(rhs);
    }
      
    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const orderable_regular_beast&, const orderable_regular_beast&) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const orderable_regular_beast&, const orderable_regular_beast&) noexcept = default;

    [[nodiscard]]
    friend bool operator<(const orderable_regular_beast& lhs, const orderable_regular_beast& rhs) noexcept
    {
      return lhs.x < rhs.x;
    }

    [[nodiscard]]
    friend bool operator<=(const orderable_regular_beast& lhs, const orderable_regular_beast& rhs) noexcept
    {
      return lhs.x <= rhs.x;
    }

    [[nodiscard]]
    friend bool operator>(const orderable_regular_beast& lhs, const orderable_regular_beast& rhs) noexcept
    {
      return lhs.x > rhs.x;
    }

    [[nodiscard]]
    friend bool operator>=(const orderable_regular_beast& lhs, const orderable_regular_beast& rhs) noexcept
    {
      return lhs.x >= rhs.x;
    }

    // TO DO: default this and remove most of the above when libc++ rolls out <=> to std
    friend std::weak_ordering operator<=>(const orderable_regular_beast& lhs, const orderable_regular_beast& rhs) noexcept
    {
      if(lhs < rhs) return std::weak_ordering::less;

      if(rhs > lhs) return std::weak_ordering::greater;

      return std::weak_ordering::equivalent;
    }

    template<class Stream>
    friend Stream& operator<<(Stream& s, const orderable_regular_beast& b)
    {
      for(auto i : b.x) s << i << ' ';
      return s;
    }
  };

  template<class T=int, class Allocator=std::allocator<int>>
  struct regular_broken_less
  {
    using allocator_type = Allocator;

    regular_broken_less(std::initializer_list<T> list) : x{list} {}

    regular_broken_less(std::initializer_list<T> list, const allocator_type& a) : x{list, a} {}

    regular_broken_less(const allocator_type& a) : x(a) {}

    regular_broken_less(const regular_broken_less&) = default;

    regular_broken_less(regular_broken_less&&) noexcept = default;

    regular_broken_less(regular_broken_less&& other, const allocator_type& a) : x(std::move(other.x), a) {}

    regular_broken_less& operator=(const regular_broken_less&) = default;

    regular_broken_less& operator=(regular_broken_less&&) = default;

    void swap(regular_broken_less& other) noexcept(noexcept(std::swap(this->x, other.x)))
    {
      std::swap(x, other.x);
    }

    friend void swap(regular_broken_less& lhs, regular_broken_less& rhs)
      noexcept(noexcept(lhs.swap(rhs)))
    {
      lhs.swap(rhs);
    }
      
    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const regular_broken_less&, const regular_broken_less&) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const regular_broken_less&, const regular_broken_less&) noexcept = default;

    [[nodiscard]]
    friend bool operator<(const regular_broken_less& lhs, const regular_broken_less& rhs) noexcept
    {
      return lhs.x > rhs.x;
    }

    [[nodiscard]]
    friend bool operator<=(const regular_broken_less& lhs, const regular_broken_less& rhs) noexcept
    {
      return lhs.x <= rhs.x;
    }

    [[nodiscard]]
    friend bool operator>(const regular_broken_less& lhs, const regular_broken_less& rhs) noexcept
    {
      return lhs.x > rhs.x;
    }

    [[nodiscard]]
    friend bool operator>=(const regular_broken_less& lhs, const regular_broken_less& rhs) noexcept
    {
      return lhs.x >= rhs.x;
    }

    // TO DO: default this and remove most of the above when libc++ rolls out <=> to std
    friend std::weak_ordering operator<=>(const regular_broken_less& lhs, const regular_broken_less& rhs) noexcept
    {
      if(lhs < rhs) return std::weak_ordering::less;

      if(rhs > lhs) return std::weak_ordering::greater;

      return std::weak_ordering::equivalent;
    }

    template<class Stream>
    friend Stream& operator<<(Stream& s, const regular_broken_less& b)
    {
      for(auto i : b.x) s << i << ' ';
      return s;
    }
  };

  template<class T=int, class Allocator=std::allocator<int>>
  struct regular_broken_lesseq
  {
    using allocator_type = Allocator;

    regular_broken_lesseq(std::initializer_list<T> list) : x{list} {}

    regular_broken_lesseq(std::initializer_list<T> list, const allocator_type& a) : x{list, a} {}

    regular_broken_lesseq(const allocator_type& a) : x(a) {}

    regular_broken_lesseq(const regular_broken_lesseq&) = default;

    regular_broken_lesseq(regular_broken_lesseq&&) noexcept = default;

    regular_broken_lesseq(regular_broken_lesseq&& other, const allocator_type& a) : x(std::move(other.x), a) {}

    regular_broken_lesseq& operator=(const regular_broken_lesseq&) = default;

    regular_broken_lesseq& operator=(regular_broken_lesseq&&) = default;

    void swap(regular_broken_lesseq& other) noexcept(noexcept(std::swap(this->x, other.x)))
    {
      std::swap(x, other.x);
    }

    friend void swap(regular_broken_lesseq& lhs, regular_broken_lesseq& rhs)
      noexcept(noexcept(lhs.swap(rhs)))
    {
      lhs.swap(rhs);
    }
      
    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const regular_broken_lesseq&, const regular_broken_lesseq&) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const regular_broken_lesseq&, const regular_broken_lesseq&) noexcept = default;

    [[nodiscard]]
    friend bool operator<(const regular_broken_lesseq& lhs, const regular_broken_lesseq& rhs) noexcept
    {
      return lhs.x < rhs.x;
    }

    [[nodiscard]]
    friend bool operator<=(const regular_broken_lesseq& lhs, const regular_broken_lesseq& rhs) noexcept
    {
      return lhs.x >= rhs.x;
    }

    [[nodiscard]]
    friend bool operator>(const regular_broken_lesseq& lhs, const regular_broken_lesseq& rhs) noexcept
    {
      return lhs.x > rhs.x;
    }

    [[nodiscard]]
    friend bool operator>=(const regular_broken_lesseq& lhs, const regular_broken_lesseq& rhs) noexcept
    {
      return lhs.x >= rhs.x;
    }

    // TO DO: default this and remove most of the above when libc++ rolls out <=> to std
    friend std::weak_ordering operator<=>(const regular_broken_lesseq& lhs, const regular_broken_lesseq& rhs) noexcept
    {
      if(lhs < rhs) return std::weak_ordering::less;

      if(rhs > lhs) return std::weak_ordering::greater;

      return std::weak_ordering::equivalent;
    }

    template<class Stream>
    friend Stream& operator<<(Stream& s, const regular_broken_lesseq& b)
    {
      for(auto i : b.x) s << i << ' ';
      return s;
    }
  };

  template<class T=int, class Allocator=std::allocator<int>>
  struct regular_broken_greater
  {
    using allocator_type = Allocator;

    regular_broken_greater(std::initializer_list<T> list) : x{list} {}

    regular_broken_greater(std::initializer_list<T> list, const allocator_type& a) : x{list, a} {}

    regular_broken_greater(const allocator_type& a) : x(a) {}

    regular_broken_greater(const regular_broken_greater&) = default;

    regular_broken_greater(regular_broken_greater&&) noexcept = default;

    regular_broken_greater(regular_broken_greater&& other, const allocator_type& a) : x(std::move(other.x), a) {}

    regular_broken_greater& operator=(const regular_broken_greater&) = default;

    regular_broken_greater& operator=(regular_broken_greater&&) = default;

    void swap(regular_broken_greater& other) noexcept(noexcept(std::swap(this->x, other.x)))
    {
      std::swap(x, other.x);
    }

    friend void swap(regular_broken_greater& lhs, regular_broken_greater& rhs)
      noexcept(noexcept(lhs.swap(rhs)))
    {
      lhs.swap(rhs);
    }
      
    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const regular_broken_greater&, const regular_broken_greater&) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const regular_broken_greater&, const regular_broken_greater&) noexcept = default;

    [[nodiscard]]
    friend bool operator<(const regular_broken_greater& lhs, const regular_broken_greater& rhs) noexcept
    {
      return lhs.x < rhs.x;
    }

    [[nodiscard]]
    friend bool operator<=(const regular_broken_greater& lhs, const regular_broken_greater& rhs) noexcept
    {
      return lhs.x <= rhs.x;
    }

    [[nodiscard]]
    friend bool operator>(const regular_broken_greater& lhs, const regular_broken_greater& rhs) noexcept
    {
      return lhs.x < rhs.x;
    }

    [[nodiscard]]
    friend bool operator>=(const regular_broken_greater& lhs, const regular_broken_greater& rhs) noexcept
    {
      return lhs.x >= rhs.x;
    }

    // TO DO: default this and remove most of the above when libc++ rolls out <=> to std
    friend std::weak_ordering operator<=>(const regular_broken_greater& lhs, const regular_broken_greater& rhs) noexcept
    {
      if(lhs < rhs) return std::weak_ordering::less;

      if(rhs > lhs) return std::weak_ordering::greater;

      return std::weak_ordering::equivalent;
    }

    template<class Stream>
    friend Stream& operator<<(Stream& s, const regular_broken_greater& b)
    {
      for(auto i : b.x) s << i << ' ';
      return s;
    }
  };

  template<class T=int, class Allocator=std::allocator<int>>
  struct regular_broken_greatereq
  {
    using allocator_type = Allocator;

    regular_broken_greatereq(std::initializer_list<T> list) : x{list} {}

    regular_broken_greatereq(std::initializer_list<T> list, const allocator_type& a) : x{list, a} {}

    regular_broken_greatereq(const allocator_type& a) : x(a) {}

    regular_broken_greatereq(const regular_broken_greatereq&) = default;

    regular_broken_greatereq(regular_broken_greatereq&&) noexcept = default;

    regular_broken_greatereq(regular_broken_greatereq&& other, const allocator_type& a) : x(std::move(other.x), a) {}

    regular_broken_greatereq& operator=(const regular_broken_greatereq&) = default;

    regular_broken_greatereq& operator=(regular_broken_greatereq&&) = default;

    void swap(regular_broken_greatereq& other) noexcept(noexcept(std::swap(this->x, other.x)))
    {
      std::swap(x, other.x);
    }

    friend void swap(regular_broken_greatereq& lhs, regular_broken_greatereq& rhs)
      noexcept(noexcept(lhs.swap(rhs)))
    {
      lhs.swap(rhs);
    }
      
    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const regular_broken_greatereq&, const regular_broken_greatereq&) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const regular_broken_greatereq&, const regular_broken_greatereq&) noexcept = default;

    [[nodiscard]]
    friend bool operator<(const regular_broken_greatereq& lhs, const regular_broken_greatereq& rhs) noexcept
    {
      return lhs.x < rhs.x;
    }

    [[nodiscard]]
    friend bool operator<=(const regular_broken_greatereq& lhs, const regular_broken_greatereq& rhs) noexcept
    {
      return lhs.x >= rhs.x;
    }

    [[nodiscard]]
    friend bool operator>(const regular_broken_greatereq& lhs, const regular_broken_greatereq& rhs) noexcept
    {
      return lhs.x > rhs.x;
    }

    [[nodiscard]]
    friend bool operator>=(const regular_broken_greatereq& lhs, const regular_broken_greatereq& rhs) noexcept
    {
      return lhs.x >= rhs.x;
    }

    // TO DO: default this and remove most of the above when libc++ rolls out <=> to std
    friend std::weak_ordering operator<=>(const regular_broken_greatereq& lhs, const regular_broken_greatereq& rhs) noexcept
    {
      if(lhs < rhs) return std::weak_ordering::less;

      if(rhs > lhs) return std::weak_ordering::greater;

      return std::weak_ordering::equivalent;
    }

    template<class Stream>
    friend Stream& operator<<(Stream& s, const regular_broken_greatereq& b)
    {
      for(auto i : b.x) s << i << ' ';
      return s;
    }
  };

  template<class T=int, class Allocator=std::allocator<int>>
  struct regular_inverted_comparisons
  {
    using allocator_type = Allocator;

    regular_inverted_comparisons(std::initializer_list<T> list) : x{list} {}

    regular_inverted_comparisons(std::initializer_list<T> list, const allocator_type& a) : x{list, a} {}

    regular_inverted_comparisons(const allocator_type& a) : x(a) {}

    regular_inverted_comparisons(const regular_inverted_comparisons&) = default;

    regular_inverted_comparisons(regular_inverted_comparisons&&) noexcept = default;

    regular_inverted_comparisons(regular_inverted_comparisons&& other, const allocator_type& a) : x(std::move(other.x), a) {}

    regular_inverted_comparisons& operator=(const regular_inverted_comparisons&) = default;

    regular_inverted_comparisons& operator=(regular_inverted_comparisons&&) = default;

    void swap(regular_inverted_comparisons& other) noexcept(noexcept(std::swap(this->x, other.x)))
    {
      std::swap(x, other.x);
    }

    friend void swap(regular_inverted_comparisons& lhs, regular_inverted_comparisons& rhs)
      noexcept(noexcept(lhs.swap(rhs)))
    {
      lhs.swap(rhs);
    }
      
    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const regular_inverted_comparisons&, const regular_inverted_comparisons&) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const regular_inverted_comparisons&, const regular_inverted_comparisons&) noexcept = default;

    [[nodiscard]]
    friend bool operator<(const regular_inverted_comparisons& lhs, const regular_inverted_comparisons& rhs) noexcept
    {
      return lhs.x > rhs.x;
    }

    [[nodiscard]]
    friend bool operator<=(const regular_inverted_comparisons& lhs, const regular_inverted_comparisons& rhs) noexcept
    {
      return lhs.x >= rhs.x;
    }

    [[nodiscard]]
    friend bool operator>(const regular_inverted_comparisons& lhs, const regular_inverted_comparisons& rhs) noexcept
    {
      return lhs.x < rhs.x;
    }

    [[nodiscard]]
    friend bool operator>=(const regular_inverted_comparisons& lhs, const regular_inverted_comparisons& rhs) noexcept
    {
      return lhs.x <= rhs.x;
    }

    // TO DO: default this and remove most of the above when libc++ rolls out <=> to std
    friend std::weak_ordering operator<=>(const regular_inverted_comparisons& lhs, const regular_inverted_comparisons& rhs) noexcept
    {
      if(lhs < rhs) return std::weak_ordering::less;

      if(rhs > lhs) return std::weak_ordering::greater;

      return std::weak_ordering::equivalent;
    }

    template<class Stream>
    friend Stream& operator<<(Stream& s, const regular_inverted_comparisons& b)
    {
      for(auto i : b.x) s << i << ' ';
      return s;
    }
  };

  template<class T=int, class Allocator=std::allocator<int>>
  struct regular_broken_spaceship
  {
    using allocator_type = Allocator;

    regular_broken_spaceship(std::initializer_list<T> list) : x{list} {}

    regular_broken_spaceship(std::initializer_list<T> list, const allocator_type& a) : x{list, a} {}

    regular_broken_spaceship(const allocator_type& a) : x(a) {}

    regular_broken_spaceship(const regular_broken_spaceship&) = default;

    regular_broken_spaceship(regular_broken_spaceship&&) noexcept = default;

    regular_broken_spaceship(regular_broken_spaceship&& other, const allocator_type& a) : x(std::move(other.x), a) {}

    regular_broken_spaceship& operator=(const regular_broken_spaceship&) = default;

    regular_broken_spaceship& operator=(regular_broken_spaceship&&) = default;

    void swap(regular_broken_spaceship& other) noexcept(noexcept(std::swap(this->x, other.x)))
    {
      std::swap(x, other.x);
    }

    friend void swap(regular_broken_spaceship& lhs, regular_broken_spaceship& rhs)
      noexcept(noexcept(lhs.swap(rhs)))
    {
      lhs.swap(rhs);
    }
      
    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const regular_broken_spaceship&, const regular_broken_spaceship&) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const regular_broken_spaceship&, const regular_broken_spaceship&) noexcept = default;

    [[nodiscard]]
    friend bool operator<(const regular_broken_spaceship& lhs, const regular_broken_spaceship& rhs) noexcept
    {
      return lhs.x < rhs.x;
    }

    [[nodiscard]]
    friend bool operator<=(const regular_broken_spaceship& lhs, const regular_broken_spaceship& rhs) noexcept
    {
      return lhs.x <= rhs.x;
    }

    [[nodiscard]]
    friend bool operator>(const regular_broken_spaceship& lhs, const regular_broken_spaceship& rhs) noexcept
    {
      return lhs.x > rhs.x;
    }

    [[nodiscard]]
    friend bool operator>=(const regular_broken_spaceship& lhs, const regular_broken_spaceship& rhs) noexcept
    {
      return lhs.x >= rhs.x;
    }

    // TO DO: default this and remove most of the above when libc++ rolls out <=> to std
    friend std::weak_ordering operator<=>(const regular_broken_spaceship& lhs, const regular_broken_spaceship& rhs) noexcept
    {
      if(lhs < rhs) return std::weak_ordering::greater;

      if(rhs > lhs) return std::weak_ordering::less;

      return std::weak_ordering::equivalent;
    }

    template<class Stream>
    friend Stream& operator<<(Stream& s, const regular_broken_spaceship& b)
    {
      for(auto i : b.x) s << i << ' ';
      return s;
    }
  };
}
