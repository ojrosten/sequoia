////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/Core/Utilities/AssignmentUtilities.hpp"
#include "sequoia/TestFramework/AllocationCheckers.hpp"

#include <vector>

namespace sequoia::testing
{
  template<class T=int, class Allocator=std::allocator<int>>
  struct perfectly_normal_beast
  {
    using value_type = T;
    using allocator_type = Allocator;

    perfectly_normal_beast(std::initializer_list<T> list) : x{list} {}

    perfectly_normal_beast(std::initializer_list<T> list, const allocator_type& a) : x{list, a} {}

    perfectly_normal_beast(const allocator_type& a) : x(a) {}

    perfectly_normal_beast(const perfectly_normal_beast&) = default;

    perfectly_normal_beast(const perfectly_normal_beast& other, const allocator_type& a) : x(other.x, a) {}

    perfectly_normal_beast(perfectly_normal_beast&&) noexcept = default;

    perfectly_normal_beast(perfectly_normal_beast&& other, const allocator_type& a) : x(std::move(other.x), a) {}

    perfectly_normal_beast& operator=(const perfectly_normal_beast&) = default;

    perfectly_normal_beast& operator=(perfectly_normal_beast&&) = default;

    void swap(perfectly_normal_beast& other) noexcept(noexcept(std::swap(this->x, other.x)))
    {
      std::swap(x, other.x);
    }

    friend void swap(perfectly_normal_beast& lhs, perfectly_normal_beast& rhs)
      noexcept(noexcept(lhs.swap(rhs)))
    {
      lhs.swap(rhs);
    }

    using container_type = std::vector<T, Allocator>;

    container_type x{};

    [[nodiscard]]
    friend bool operator==(const perfectly_normal_beast&, const perfectly_normal_beast&) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const perfectly_normal_beast&, const perfectly_normal_beast&) noexcept = default;

    template<class Stream>
      requires serializable<T>
    friend Stream& operator<<(Stream& s, const perfectly_normal_beast& b)
    {
      for(const auto& i : b.x) s << i << '\n';
      return s;
    }

    // Only provide de-serialization in simple cases to keep life easy!
    template<class Stream>
      requires std::is_arithmetic_v<T>
    friend Stream& operator>>(Stream& s, perfectly_normal_beast& b)
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
  struct value_tester<perfectly_normal_beast<T, Allocator>>
  {
    template<test_mode Mode>
    static void test(equality_check_t, test_logger<Mode>& logger, const perfectly_normal_beast<T, Allocator>& obtained, const perfectly_normal_beast<T, Allocator>& prediction)
    {
      check(equality, "", logger, obtained.x, prediction.x);
    }

    template<class Logger>
    static void test(weak_equivalence_check_t, Logger& logger, const perfectly_normal_beast<T, Allocator>& beast, std::initializer_list<T> prediction)
    {
      check(weak_equivalence, "", logger, std::begin(beast.x), std::end(beast.x), std::begin(prediction), std::end(prediction));
    }

    template<class Logger, class Advisor>
    static void test(weak_equivalence_check_t, Logger& logger, const perfectly_normal_beast<T, Allocator>& beast, std::initializer_list<T> prediction, tutor<Advisor> advisor)
    {
      check(weak_equivalence, "", logger, std::begin(beast.x), std::end(beast.x), std::begin(prediction), std::end(prediction), std::move(advisor));
    }
  };

  template<class CharT = char, class Allocator=std::allocator<CharT>>
  struct perfectly_stringy_beast
  {
    using allocator_type = Allocator;
    using string_type    = std::basic_string<CharT, std::char_traits<CharT>, Allocator>;

    perfectly_stringy_beast() = default;

    perfectly_stringy_beast(const CharT* s) : x{s} {}

    perfectly_stringy_beast(const CharT* s, const allocator_type& a) : x{s, a} {}

    perfectly_stringy_beast(const allocator_type& a) : x(a) {}

    perfectly_stringy_beast(const perfectly_stringy_beast&) = default;

    perfectly_stringy_beast(const perfectly_stringy_beast& other, const allocator_type& a) : x(other.x, a) {}

    perfectly_stringy_beast(perfectly_stringy_beast&&) noexcept = default;

    perfectly_stringy_beast(perfectly_stringy_beast&& other, const allocator_type& a) : x(std::move(other.x), a) {}

    perfectly_stringy_beast& operator=(const perfectly_stringy_beast&) = default;

    perfectly_stringy_beast& operator=(perfectly_stringy_beast&&) noexcept = default;

    void swap(perfectly_stringy_beast& other) noexcept(noexcept(std::swap(this->x, other.x)))
    {
      std::swap(x, other.x);
    }

    friend void swap(perfectly_stringy_beast& lhs, perfectly_stringy_beast& rhs)
      noexcept(noexcept(lhs.swap(rhs)))
    {
      lhs.swap(rhs);
    }

    string_type x{};

    [[nodiscard]]
    friend bool operator==(const perfectly_stringy_beast&, const perfectly_stringy_beast&) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const perfectly_stringy_beast&, const perfectly_stringy_beast&) noexcept = default;

    template<class Stream>
    friend Stream& operator<<(Stream& s, const perfectly_stringy_beast& b)
    {
      s << b.x;
      return s;
    }

    template<class Stream>
    friend Stream& operator>>(Stream& s, perfectly_stringy_beast& b)
    {
      b.x.clear();

      string_type word{};
      while(s >> word)
      {
        b.x.append(word).append(" ");
      }

      if(!b.x.empty()) b.x.pop_back();

      return s;
    }
  };

  template<class T=int, class Handle=std::shared_ptr<T>, class Allocator=std::allocator<Handle>>
  struct perfectly_sharing_beast
  {
    using handle_type    = Handle;
    using allocator_type = Allocator;
    using container_type = std::vector<handle_type, allocator_type>;

    struct alloc_acquirer
    {
      using alloc_equivalence_class = allocation_equivalence_classes::container_of_pointers<allocator_type>;

      [[nodiscard]]
      allocator_type operator()(const perfectly_sharing_beast& beast) const
      {
        return beast.x.get_allocator();
      }
    };

    explicit perfectly_sharing_beast(const allocator_type& a) : x(a)
    {}

    perfectly_sharing_beast(std::initializer_list<T> list, const allocator_type& a = allocator_type{})
      : x(a)
    {
      x.reserve(list.size());
      for(auto e : list)
        x.emplace_back(std::make_shared<T>(e));
    };

    perfectly_sharing_beast(const perfectly_sharing_beast& other)
      : perfectly_sharing_beast(other, other.x.get_allocator())
    {}

    perfectly_sharing_beast(const perfectly_sharing_beast& other, const allocator_type& a)
      : x(a)
    {
      x.reserve(other.x.size());
      for(auto e : other.x)
      {
        x.emplace_back(std::make_shared<T>(*e));
      }
    }

    perfectly_sharing_beast(perfectly_sharing_beast&&) noexcept = default;

    perfectly_sharing_beast(perfectly_sharing_beast&& other, const allocator_type& a)
      : x(std::move(other.x), a) {}

    perfectly_sharing_beast& operator=(const perfectly_sharing_beast& other)
    {
      assignment_helper::assign(*this, other, alloc_acquirer{});

      return *this;
    }

    perfectly_sharing_beast& operator=(perfectly_sharing_beast&&) = default;

    void swap(perfectly_sharing_beast& other) noexcept(noexcept(std::swap(this->x, other.x)))
    {
      std::swap(x, other.x);
    }

    friend void swap(perfectly_sharing_beast& lhs, perfectly_sharing_beast& rhs)
      noexcept(noexcept(lhs.swap(rhs)))
    {
      lhs.swap(rhs);
    }

    container_type x{};

    [[nodiscard]]
    friend bool operator==(const perfectly_sharing_beast& lhs, const perfectly_sharing_beast& rhs) noexcept
    {
      return std::equal(lhs.x.cbegin(), lhs.x.cend(), rhs.x.cbegin(), rhs.x.cend(), [](auto& l, auto& r){
          return *l == *r;
        });
    }

    [[nodiscard]]
    friend bool operator!=(const perfectly_sharing_beast& lhs, const perfectly_sharing_beast& rhs) noexcept
    {
      return !(lhs == rhs);
    }

    template<class Stream>
    friend Stream& operator<<(Stream& s, const perfectly_sharing_beast& b)
    {
      for(auto i : b.x) s << *i << ' ';
      return s;
    }

    void reset(const allocator_type& a)
    {
      const std::vector<handle_type, allocator_type> v(a);
      x = v;
    }
  };

  template<class T=int, class U=double, class xAllocator=std::allocator<T>, class yAllocator=std::allocator<U>>
  struct doubly_normal_beast
  {
    using x_allocator_type = xAllocator;
    using y_allocator_type = yAllocator;

    doubly_normal_beast(std::initializer_list<T> xlist, std::initializer_list<U> ylist)
      : x{xlist} , y{ylist}
    {}

    doubly_normal_beast(std::initializer_list<T> xlist, std::initializer_list<U> ylist, const x_allocator_type& a, const y_allocator_type& b)
      : x(xlist, a), y(ylist,b)
    {}

    doubly_normal_beast(const doubly_normal_beast&) = default;

    doubly_normal_beast(const doubly_normal_beast& other, const x_allocator_type& a, const y_allocator_type& b)
      : x(other.x, a), y(other.y, b)
    {}

    doubly_normal_beast(doubly_normal_beast&&) noexcept = default;

    doubly_normal_beast(doubly_normal_beast&& other, const x_allocator_type& a, const y_allocator_type& b)
      : x(std::move(other.x), a), y(std::move(other.y), b)
    {}

    doubly_normal_beast& operator=(const doubly_normal_beast&) = default;

    doubly_normal_beast& operator=(doubly_normal_beast&&) = default;

    void swap(doubly_normal_beast& other) noexcept(noexcept(std::swap(this->x, other.x)) && noexcept(std::swap(this->y, other.y)))
    {
      std::swap(x, other.x);
      std::swap(y, other.y);
    }

    friend void swap(doubly_normal_beast& lhs, doubly_normal_beast& rhs)
      noexcept(noexcept(lhs.swap(rhs)))
    {
      lhs.swap(rhs);
    }

    std::vector<T, xAllocator> x{};
    std::vector<U, yAllocator> y{};

    [[nodiscard]]
    friend bool operator==(const doubly_normal_beast& lhs, const doubly_normal_beast& rhs) noexcept
    {
      return (lhs.x == rhs.x) && (rhs.x == lhs.x);
    }

    [[nodiscard]]
    friend bool operator!=(const doubly_normal_beast& lhs, const doubly_normal_beast& rhs) noexcept
    {
      return !(lhs == rhs);
    }

    template<class Stream>
    friend Stream& operator<<(Stream& s, const doubly_normal_beast& b)
    {
      for(auto i : b.x) s << i << ' ';
      for(auto i : b.y) s << i << ' ';
      return s;
    }
  };

  template<class T=int, class Allocator=std::allocator<int>>
  struct broken_equality
  {
    using allocator_type = Allocator;

    broken_equality(std::initializer_list<T> list) : x{list} {}

    broken_equality(std::initializer_list<T> list, const allocator_type& a) : x(list, a) {}

    broken_equality(const broken_equality&) = default;

    broken_equality(const broken_equality& other, const allocator_type& a) : x(other.x, a) {}

    broken_equality(broken_equality&&) noexcept = default;

    broken_equality(broken_equality&& other, const allocator_type& a) : x(std::move(other.x), a) {}

    broken_equality& operator=(const broken_equality&) = default;

    broken_equality& operator=(broken_equality&&) = default;

    friend void swap(broken_equality& lhs, broken_equality& rhs)
    {
      std::swap(lhs.x, rhs.x);
    }

    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const broken_equality& lhs, const broken_equality& rhs) noexcept
    {
      return lhs.x != rhs.x;
    }

    [[nodiscard]]
    friend bool operator!=(const broken_equality& lhs, const broken_equality& rhs) noexcept
    {
      return lhs.x != rhs.x;
    }

    template<class Stream>
    friend Stream& operator<<(Stream& s, const broken_equality& b)
    {
      for(auto i : b.x) s << i << ' ';
      return s;
    }
  };

  template<class T=int, class Allocator=std::allocator<int>>
  struct broken_inequality
  {
    using allocator_type = Allocator;

    broken_inequality(std::initializer_list<T> list) : x{list} {}

    broken_inequality(std::initializer_list<T> list, const allocator_type& a) : x(list, a) {}

    broken_inequality(const broken_inequality&) = default;

    broken_inequality(const broken_inequality& other, const allocator_type& a) : x(other.x, a) {}

    broken_inequality(broken_inequality&&) noexcept = default;

    broken_inequality(broken_inequality&& other, const allocator_type& a) : x(std::move(other.x), a) {}

    broken_inequality& operator=(const broken_inequality&) = default;

    broken_inequality& operator=(broken_inequality&&) = default;

    friend void swap(broken_inequality& lhs, broken_inequality& rhs)
    {
      std::swap(lhs.x, rhs.x);
    }

    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const broken_inequality& lhs, const broken_inequality& rhs) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const broken_inequality& lhs, const broken_inequality& rhs) noexcept
    {
      return lhs.x == rhs.x;
    }

    template<class Stream>
    friend Stream& operator<<(Stream& s, const broken_inequality& b)
    {
      for(auto i : b.x) s << i << ' ';
      return s;
    }
  };

  template<class T=int, class Allocator=std::allocator<int>>
  struct broken_copy
  {
    using allocator_type = Allocator;

    broken_copy(std::initializer_list<T> list) : x{list} {}

    broken_copy(std::initializer_list<T> list, const allocator_type& a) : x{list, a} {}

    broken_copy(const broken_copy&)
    {
      // Do nothing
    }

    broken_copy(const broken_copy& other, const allocator_type& a) : x(other.x, a)
    {}

    broken_copy(broken_copy&&) = default;

    broken_copy(broken_copy&& other, const allocator_type& a) : x(std::move(other.x), a) {}

    broken_copy& operator=(const broken_copy&) = default;

    broken_copy& operator=(broken_copy&&) = default;

    friend void swap(broken_copy& lhs, broken_copy& rhs)
    {
      std::swap(lhs.x, rhs.x);
    }

    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const broken_copy& lhs, const broken_copy& rhs) noexcept
    {
      return lhs.x == rhs.x;
    }

    [[nodiscard]]
    friend bool operator!=(const broken_copy& lhs, const broken_copy& rhs) noexcept
    {
      return !(lhs == rhs);
    }

    template<class Stream>
    friend Stream& operator<<(Stream& s, const broken_copy& b)
    {
      for(auto i : b.x) s << i << ' ';
      return s;
    }
  };

  template<class T=int, class Allocator=std::allocator<int>>
  struct broken_para_copy
  {
    using allocator_type = Allocator;

    broken_para_copy(std::initializer_list<T> list) : x{list} {}

    broken_para_copy(std::initializer_list<T> list, const allocator_type& a) : x(list, a) {}

    broken_para_copy(const broken_para_copy&) = default;

    broken_para_copy(const broken_para_copy&, const allocator_type&)
    {
      // do nothing
    }

    broken_para_copy(broken_para_copy&&) = default;

    broken_para_copy(broken_para_copy&& other, const allocator_type& a) : x(std::move(other.x), a) {}

    broken_para_copy& operator=(const broken_para_copy&) = default;

    broken_para_copy& operator=(broken_para_copy&&) = default;

    friend void swap(broken_para_copy& lhs, broken_para_copy& rhs)
    {
      std::swap(lhs.x, rhs.x);
    }

    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const broken_para_copy& lhs, const broken_para_copy& rhs) noexcept
    {
      return lhs.x == rhs.x;
    }

    [[nodiscard]]
    friend bool operator!=(const broken_para_copy& lhs, const broken_para_copy& rhs) noexcept
    {
      return !(lhs == rhs);
    }

    template<class Stream>
    friend Stream& operator<<(Stream& s, const broken_para_copy& b)
    {
      for(auto i : b.x) s << i << ' ';
      return s;
    }
  };

  template<class T=int, class Allocator=std::allocator<int>>
  struct broken_move
  {
    using allocator_type = Allocator;

    broken_move(std::initializer_list<T> list) : x{list} {}

    broken_move(std::initializer_list<T> list, const allocator_type& a) : x(list, a) {}

    broken_move(const broken_move&) = default;

    broken_move(const broken_move& other, const allocator_type& a) : x(other.x, a) {}

    broken_move(broken_move&&) noexcept
    {
      // Do nothing
    }

    broken_move(broken_move&& other, const allocator_type& a) : x(std::move(other.x), a)
    {}

    broken_move& operator=(const broken_move&) = default;

    broken_move& operator=(broken_move&&) = default;

    friend void swap(broken_move& lhs, broken_move& rhs)
    {
      std::swap(lhs.x, rhs.x);
    }

    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const broken_move& lhs, const broken_move& rhs) noexcept
    {
      return lhs.x == rhs.x;
    }

    [[nodiscard]]
    friend bool operator!=(const broken_move& lhs, const broken_move& rhs) noexcept
    {
      return !(lhs == rhs);
    }

    template<class Stream>
    friend Stream& operator<<(Stream& s, const broken_move& b)
    {
      for(auto i : b.x) s << i << ' ';
      return s;
    }
  };

  template<class T=int, class Allocator=std::allocator<int>>
  struct broken_para_move
  {
    using allocator_type = Allocator;

    broken_para_move(std::initializer_list<T> list) : x{list} {}

    broken_para_move(std::initializer_list<T> list, const allocator_type& a) : x(list, a) {}

    broken_para_move(const broken_para_move&) = default;

    broken_para_move(const broken_para_move& other, const allocator_type& a) : x(other.x, a) {}

    broken_para_move(broken_para_move&&) = default;

    broken_para_move(broken_para_move&&, const allocator_type&)
    {
      // do nothing
    }

    broken_para_move& operator=(const broken_para_move&) = default;

    broken_para_move& operator=(broken_para_move&&) = default;

    friend void swap(broken_para_move& lhs, broken_para_move& rhs)
    {
      std::swap(lhs.x, rhs.x);
    }

    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const broken_para_move& lhs, const broken_para_move& rhs) noexcept
    {
      return lhs.x == rhs.x;
    }

    [[nodiscard]]
    friend bool operator!=(const broken_para_move& lhs, const broken_para_move& rhs) noexcept
    {
      return !(lhs == rhs);
    }

    template<class Stream>
    friend Stream& operator<<(Stream& s, const broken_para_move& b)
    {
      for(auto i : b.x) s << i << ' ';
      return s;
    }
  };

  template<class T=int, class Allocator=std::allocator<int>>
  struct broken_copy_assignment
  {
    using allocator_type = Allocator;

    broken_copy_assignment(std::initializer_list<int> list) : x{list} {}

    broken_copy_assignment(std::initializer_list<int> list, const allocator_type& a) : x(list, a) {}

    broken_copy_assignment(const broken_copy_assignment&) = default;

    broken_copy_assignment(const broken_copy_assignment& other, const allocator_type& a) : x(other.x, a) {}

    broken_copy_assignment(broken_copy_assignment&&) = default;

    broken_copy_assignment(broken_copy_assignment&& other, const allocator_type& a) : x(std::move(other.x), a) {}

    broken_copy_assignment& operator=(const broken_copy_assignment&)
    {
      return *this;
    }

    broken_copy_assignment& operator=(broken_copy_assignment&&) = default;

    friend void swap(broken_copy_assignment& lhs, broken_copy_assignment& rhs)
    {
      std::swap(lhs.x, rhs.x);
    }

    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const broken_copy_assignment& lhs, const broken_copy_assignment& rhs) noexcept
    {
      return lhs.x == rhs.x;
    }

    [[nodiscard]]
    friend bool operator!=(const broken_copy_assignment& lhs, const broken_copy_assignment& rhs) noexcept
    {
      return !(lhs == rhs);
    }

    template<class Stream>
    friend Stream& operator<<(Stream& s, const broken_copy_assignment& b)
    {
      for(auto i : b.x) s << i << ' ';
      return s;
    }
  };

  template<class T=int, class Allocator=std::allocator<int>>
  struct broken_self_copy_assignment
  {
    using allocator_type = Allocator;

    broken_self_copy_assignment(std::initializer_list<int> list) : x{list} {}

    broken_self_copy_assignment(std::initializer_list<int> list, const allocator_type& a) : x(list, a) {}

    broken_self_copy_assignment(const broken_self_copy_assignment&) = default;

    broken_self_copy_assignment(const broken_self_copy_assignment& other, const allocator_type& a) : x(other.x, a) {}

    broken_self_copy_assignment(broken_self_copy_assignment&&) = default;

    broken_self_copy_assignment(broken_self_copy_assignment&& other, const allocator_type& a) : x(std::move(other.x), a) {}

    broken_self_copy_assignment& operator=(const broken_self_copy_assignment& other)
    {
      if(&other == this)
      {
        x.push_back(T{});
      }
      else
      {
        x = other.x;
      }

      return *this;
    }

    broken_self_copy_assignment& operator=(broken_self_copy_assignment&&) = default;

    friend void swap(broken_self_copy_assignment& lhs, broken_self_copy_assignment& rhs)
    {
      std::swap(lhs.x, rhs.x);
    }

    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const broken_self_copy_assignment& lhs, const broken_self_copy_assignment& rhs) = default;

    [[nodiscard]]
    friend bool operator!=(const broken_self_copy_assignment& lhs, const broken_self_copy_assignment& rhs) = default;

    template<class Stream>
    friend Stream& operator<<(Stream& s, const broken_self_copy_assignment& b)
    {
      for(auto i : b.x) s << i << ' ';
      return s;
    }
  };

  template<class T=int, class Allocator=std::allocator<int>>
  struct broken_move_assignment
  {
    using allocator_type = Allocator;

    broken_move_assignment(std::initializer_list<T> list) : x{list} {}

    broken_move_assignment(std::initializer_list<T> list, const allocator_type& a) : x(list, a) {}

    broken_move_assignment(const broken_move_assignment&) = default;

    broken_move_assignment(const broken_move_assignment& other, const allocator_type& a) : x(other.x, a) {}

    broken_move_assignment(broken_move_assignment&&) = default;

    broken_move_assignment(broken_move_assignment&& other, const allocator_type& a) : x(std::move(other.x), a) {}

    broken_move_assignment& operator=(const broken_move_assignment&) = default;

    broken_move_assignment& operator=(broken_move_assignment&&) noexcept
    {
      return *this;
    }

    friend void swap(broken_move_assignment& lhs, broken_move_assignment& rhs)
    {
      std::swap(lhs.x, rhs.x);
    }

    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const broken_move_assignment& lhs, const broken_move_assignment& rhs) noexcept
    {
      return lhs.x == rhs.x;
    }

    [[nodiscard]]
    friend bool operator!=(const broken_move_assignment& lhs, const broken_move_assignment& rhs) noexcept
    {
      return !(lhs == rhs);
    }

    template<class Stream>
    friend Stream& operator<<(Stream& s, const broken_move_assignment& b)
    {
      for(auto i : b.x) s << i << ' ';
      return s;
    }
  };

  template<class T=int, class Allocator=std::allocator<int>>
  struct broken_swap
  {
    using allocator_type = Allocator;

    broken_swap(std::initializer_list<T> list) : x{list} {}

    broken_swap(std::initializer_list<T> list, const allocator_type& a) : x{list, a} {}

    broken_swap(const broken_swap&) = default;

    broken_swap(const broken_swap& other, const allocator_type& a) : x(other.x, a) {}

    broken_swap(broken_swap&&) noexcept = default;

    broken_swap(broken_swap&& other, const allocator_type& a) : x(std::move(other.x), a) {}

    broken_swap& operator=(const broken_swap&) = default;

    broken_swap& operator=(broken_swap&&) noexcept = default;

    friend void swap(broken_swap&, broken_swap&) noexcept
    {
      // do nothing
    }

    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const broken_swap& lhs, const broken_swap& rhs) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const broken_swap& lhs, const broken_swap& rhs) noexcept = default;

    template<class Stream>
    friend Stream& operator<<(Stream& s, const broken_swap& b)
    {
      for(auto i : b.x) s << i << ' ';
      return s;
    }
  };

  template<class T=int, class Allocator=std::allocator<int>>
  struct broken_self_swap
  {
    using allocator_type = Allocator;

    broken_self_swap(std::initializer_list<T> list) : x{list} {}

    broken_self_swap(std::initializer_list<T> list, const allocator_type& a) : x{list, a} {}

    broken_self_swap(const broken_self_swap&) = default;

    broken_self_swap(const broken_self_swap& other, const allocator_type& a) : x(other.x, a) {}

    broken_self_swap(broken_self_swap&&) noexcept = default;

    broken_self_swap(broken_self_swap&& other, const allocator_type& a) : x(std::move(other.x), a) {}

    broken_self_swap& operator=(const broken_self_swap&) = default;

    broken_self_swap& operator=(broken_self_swap&&) noexcept = default;

    void swap(broken_self_swap& other) noexcept(noexcept(std::swap(this->x, other.x)))
    {
      if(&other == this)
      {
        x.push_back(T{});
      }
      else
      {
        std::swap(this->x, other.x);
      }
    }

    friend void swap(broken_self_swap& lhs, broken_self_swap& rhs)
      noexcept(noexcept(lhs.swap(rhs)))
    {
      lhs.swap(rhs);
    }

    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const broken_self_swap& lhs, const broken_self_swap& rhs) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const broken_self_swap& lhs, const broken_self_swap& rhs) noexcept = default;

    template<class Stream>
    friend Stream& operator<<(Stream& s, const broken_self_swap& b)
    {
      for(auto i : b.x) s << i << ' ';
      return s;
    }
  };

  template<class T=int, class Handle=std::shared_ptr<T>, class Allocator=std::allocator<Handle>>
  struct broken_copy_value_semantics
  {
    using handle_type = Handle;
    using allocator_type = Allocator;

    struct alloc_acquirer
    {
      using alloc_equivalence_class = allocation_equivalence_classes::container_of_pointers<allocator_type>;

      [[nodiscard]]
      allocator_type operator()(const broken_copy_value_semantics& beast) const
      {
        return beast.x.get_allocator();
      }
    };

    broken_copy_value_semantics(std::initializer_list<T> list, const allocator_type& a = allocator_type{})
      : x(a)
    {
      x.reserve(list.size());
      for(auto e : list)
        x.emplace_back(std::make_shared<T>(e));
    };

    broken_copy_value_semantics(const broken_copy_value_semantics&) = default; // Broken!

    broken_copy_value_semantics(const broken_copy_value_semantics& other, const allocator_type& a)
      : x(a)
    {
      x.reserve(other.x.size());
      for(auto e : other.x)
      {
        x.emplace_back(std::make_shared<T>(*e));
      }
    }

    broken_copy_value_semantics(broken_copy_value_semantics&&) noexcept = default;

    broken_copy_value_semantics(broken_copy_value_semantics&& other, const allocator_type& a)
      : x(std::move(other.x), a) {}

    broken_copy_value_semantics& operator=(const broken_copy_value_semantics& other)
    {
      auto allocGetter{
        [](const broken_copy_value_semantics& psb) {
          return psb.x.get_allocator();
        }
      };

      assignment_helper::assign(*this, other, allocGetter);

      return *this;
    }

    broken_copy_value_semantics& operator=(broken_copy_value_semantics&&) = default;

    void swap(broken_copy_value_semantics& other) noexcept(noexcept(std::swap(this->x, other.x)))
    {
      std::swap(x, other.x);
    }

    friend void swap(broken_copy_value_semantics& lhs, broken_copy_value_semantics& rhs)
      noexcept(noexcept(lhs.swap(rhs)))
    {
      lhs.swap(rhs);
    }

    void reset(const allocator_type& a)
    {
      const std::vector<handle_type, allocator_type> v(a);
      x = v;
    }

    std::vector<handle_type, allocator_type> x{};

    [[nodiscard]]
    friend bool operator==(const broken_copy_value_semantics& lhs, const broken_copy_value_semantics& rhs) noexcept
    {
      return std::equal(lhs.x.cbegin(), lhs.x.cend(), rhs.x.cbegin(), rhs.x.cend(), [](auto& l, auto& r) {
        return *l == *r;
        });
    }

    [[nodiscard]]
    friend bool operator!=(const broken_copy_value_semantics& lhs, const broken_copy_value_semantics& rhs) noexcept
    {
      return !(lhs == rhs);
    }

    template<class Stream>
    friend Stream& operator<<(Stream& s, const broken_copy_value_semantics& b)
    {
      for(auto i : b.x) s << *i << ' ';
      return s;
    }
  };

  template<class T=int, class Handle=std::shared_ptr<T>, class Allocator=std::allocator<Handle>>
  struct broken_copy_assignment_value_semantics
  {
    using handle_type = Handle;
    using allocator_type = Allocator;

    broken_copy_assignment_value_semantics(std::initializer_list<T> list, const allocator_type& a = allocator_type{})
      : x(a)
    {
      x.reserve(list.size());
      for(auto e : list)
        x.emplace_back(std::make_shared<T>(e));
    };

    broken_copy_assignment_value_semantics(const broken_copy_assignment_value_semantics& other)
      : broken_copy_assignment_value_semantics(other, other.x.get_allocator())
    {}

    broken_copy_assignment_value_semantics(const broken_copy_assignment_value_semantics& other, const allocator_type& a)
      : x(a)
    {
      x.reserve(other.x.size());
      for(auto e : other.x)
      {
        x.emplace_back(std::make_shared<T>(*e));
      }
    }

    broken_copy_assignment_value_semantics(broken_copy_assignment_value_semantics&&) noexcept = default;

    broken_copy_assignment_value_semantics(broken_copy_assignment_value_semantics&& other, const allocator_type& a)
      : x(std::move(other.x), a) {}

    broken_copy_assignment_value_semantics& operator=(const broken_copy_assignment_value_semantics&) = default;

    broken_copy_assignment_value_semantics& operator=(broken_copy_assignment_value_semantics&&) = default;

    friend void swap(broken_copy_assignment_value_semantics& lhs, broken_copy_assignment_value_semantics& rhs)
    {
      std::swap(lhs.x, rhs.x);
    }

    std::vector<handle_type, allocator_type> x{};

    [[nodiscard]]
    friend bool operator==(const broken_copy_assignment_value_semantics& lhs, const broken_copy_assignment_value_semantics& rhs) noexcept
    {
      return std::equal(lhs.x.cbegin(), lhs.x.cend(), rhs.x.cbegin(), rhs.x.cend(), [](auto& l, auto& r){
          return *l == *r;
        });
    }

    [[nodiscard]]
    friend bool operator!=(const broken_copy_assignment_value_semantics& lhs, const broken_copy_assignment_value_semantics& rhs) noexcept
    {
      return !(lhs == rhs);
    }

    template<class Stream>
    friend Stream& operator<<(Stream& s, const broken_copy_assignment_value_semantics& b)
    {
      for(auto i : b.x) s << *i << ' ';
      return s;
    }
  };

  template<class T=int, class Allocator=std::allocator<int>>
  struct broken_serialization
  {
    using allocator_type = Allocator;

    broken_serialization(std::initializer_list<T> list) : x{list} {}

    broken_serialization(std::initializer_list<T> list, const allocator_type& a) : x{list, a} {}

    broken_serialization(const allocator_type& a) : x(a) {}

    broken_serialization(const broken_serialization&) = default;

    broken_serialization(const broken_serialization& other, const allocator_type& a) : x(other.x, a) {}

    broken_serialization(broken_serialization&&) noexcept = default;

    broken_serialization(broken_serialization&& other, const allocator_type& a) : x(std::move(other.x), a) {}

    broken_serialization& operator=(const broken_serialization&) = default;

    broken_serialization& operator=(broken_serialization&&) = default;

    void swap(broken_serialization& other) noexcept(noexcept(std::swap(this->x, other.x)))
    {
      std::swap(x, other.x);
    }

    friend void swap(broken_serialization& lhs, broken_serialization& rhs)
      noexcept(noexcept(lhs.swap(rhs)))
    {
      lhs.swap(rhs);
    }

    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const broken_serialization&, const broken_serialization&) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const broken_serialization&, const broken_serialization&) noexcept = default;

    template<class Stream>
    friend Stream& operator<<(Stream& s, const broken_serialization&)
    {
      return s;
    }

    template<class Stream>
    friend Stream& operator>>(Stream& s, broken_serialization& b)
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

  template<class T=int, class Allocator=std::allocator<int>>
  struct broken_deserialization
  {
    using allocator_type = Allocator;

    broken_deserialization(std::initializer_list<T> list) : x{list} {}

    broken_deserialization(std::initializer_list<T> list, const allocator_type& a) : x{list, a} {}

    broken_deserialization(const allocator_type& a) : x(a) {}

    broken_deserialization(const broken_deserialization&) = default;

    broken_deserialization(const broken_deserialization& other, const allocator_type& a) : x(other.x, a) {}

    broken_deserialization(broken_deserialization&&) noexcept = default;

    broken_deserialization(broken_deserialization&& other, const allocator_type& a) : x(std::move(other.x), a) {}

    broken_deserialization& operator=(const broken_deserialization&) = default;

    broken_deserialization& operator=(broken_deserialization&&) = default;

    void swap(broken_deserialization& other) noexcept(noexcept(std::swap(this->x, other.x)))
    {
      std::swap(x, other.x);
    }

    friend void swap(broken_deserialization& lhs, broken_deserialization& rhs)
      noexcept(noexcept(lhs.swap(rhs)))
    {
      lhs.swap(rhs);
    }

    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const broken_deserialization&, const broken_deserialization&) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const broken_deserialization&, const broken_deserialization&) noexcept = default;

    template<class Stream>
    friend Stream& operator<<(Stream& s, const broken_deserialization& b)
    {
      for(auto i : b.x) s << i << ' ';
      return s;
    }

    template<class Stream>
    friend Stream& operator>>(Stream& s, broken_deserialization&)
    {
      return s;
    }
  };

}
