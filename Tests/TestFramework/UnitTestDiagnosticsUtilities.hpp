////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "RegularTestCore.hpp"

#include "AssignmentUtilities.hpp"

#include <vector>

namespace sequoia::testing
{
  template<class T=int, class Allocator=std::allocator<int>>
  struct broken_equality
  {
    using allocator_type = Allocator;

    broken_equality(std::initializer_list<T> list) : x{list} {}
      
    broken_equality(std::initializer_list<T> list, const allocator_type& alloc) : x{list, alloc} {}

    broken_equality(const broken_equality&) = default;

    broken_equality(const broken_equality& other, const allocator_type& alloc) : x(other.x, alloc) {}

    broken_equality(broken_equality&&) noexcept = default;

    broken_equality(broken_equality&& other, const allocator_type& alloc) : x(std::move(other.x), alloc) {}

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

    broken_inequality(std::initializer_list<T> list, const allocator_type& alloc) : x{list, alloc} {}

    broken_inequality(const broken_inequality&) = default;

    broken_inequality(const broken_inequality& other, const allocator_type& alloc) : x(other.x, alloc) {}

    broken_inequality(broken_inequality&&) noexcept = default;

    broken_inequality(broken_inequality&& other, const allocator_type& alloc) : x(std::move(other.x), alloc) {}

    broken_inequality& operator=(const broken_inequality&) = default;

    broken_inequality& operator=(broken_inequality&&) = default;

    friend void swap(broken_inequality& lhs, broken_inequality& rhs)
    {
      std::swap(lhs.x, rhs.x);
    }

    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const broken_inequality& lhs, const broken_inequality& rhs) noexcept
    {
      return lhs.x != rhs.x;
    }

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
    friend bool operator==(const inefficient_equality lhs, const inefficient_equality& rhs) noexcept
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
  struct broken_copy
  {
    using allocator_type = Allocator;

    broken_copy(std::initializer_list<T> list) : x{list} {}

    broken_copy(std::initializer_list<T> list, const allocator_type& a) : x{list, a} {}

    broken_copy(const broken_copy&)
    {
      // Do nothing
    }

    broken_copy(const broken_copy& other, const allocator_type& alloc) : x(other.x, alloc)
    {}

    broken_copy(broken_copy&&) = default;

    broken_copy(broken_copy&& other, const allocator_type& alloc) : x(std::move(other.x), alloc) {}
      
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

    broken_para_copy(std::initializer_list<T> list, const allocator_type& alloc) : x{list, alloc} {}

    broken_para_copy(const broken_para_copy&) = default;

    broken_para_copy(const broken_para_copy&, const allocator_type&)
    {
      // do nothing
    }

    broken_para_copy(broken_para_copy&&) = default;

    broken_para_copy(broken_para_copy&& other, const allocator_type& alloc) : x(std::move(other.x), alloc) {}
      
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

    broken_move(std::initializer_list<T> list, const allocator_type& alloc) : x{list, alloc} {}

    broken_move(const broken_move&) = default;

    broken_move(const broken_move& other, const allocator_type& alloc) : x(other.x, alloc) {}

    broken_move(broken_move&&)      
    {
      // Do nothing
    }

    broken_move(broken_move&& other, const allocator_type& alloc) : x(std::move(other.x), alloc)
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

    broken_para_move(std::initializer_list<T> list, const allocator_type& alloc) : x{list, alloc} {}

    broken_para_move(const broken_para_move&) = default;

    broken_para_move(const broken_para_move& other, const allocator_type& alloc) : x(other.x, alloc) {}

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

    broken_copy_assignment(std::initializer_list<int> list, const allocator_type& alloc) : x{list, alloc} {}

    broken_copy_assignment(const broken_copy_assignment&) = default;

    broken_copy_assignment(const broken_copy_assignment& other, const allocator_type& alloc) : x(other.x, alloc) {}

    broken_copy_assignment(broken_copy_assignment&&) = default;

    broken_copy_assignment(broken_copy_assignment&& other, const allocator_type& alloc) : x(std::move(other.x), alloc) {}
      
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
  struct broken_move_assignment
  {
    using allocator_type = Allocator;

    broken_move_assignment(std::initializer_list<T> list) : x{list} {}

    broken_move_assignment(std::initializer_list<T> list, const allocator_type& alloc) : x{list, alloc} {}

    broken_move_assignment(const broken_move_assignment&) = default;

    broken_move_assignment(const broken_move_assignment& other, const allocator_type& alloc) : x(other.x, alloc) {}

    broken_move_assignment(broken_move_assignment&&) = default;

    broken_move_assignment(broken_move_assignment&& other, const allocator_type& alloc) : x(std::move(other.x), alloc) {}
      
    broken_move_assignment& operator=(const broken_move_assignment&) = default;
      
    broken_move_assignment& operator=(broken_move_assignment&&)
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

    broken_swap(const broken_swap& other, const allocator_type& alloc) : x(other.x, alloc) {}

    broken_swap(broken_swap&&) noexcept = default;

    broken_swap(broken_swap&& other, const allocator_type& alloc) : x(std::move(other.x), alloc) {}

    broken_swap& operator=(const broken_swap&) = default;

    broken_swap& operator=(broken_swap&&) noexcept = default;

    friend void swap(broken_swap&, broken_swap&) noexcept
    {
      // do nothing
    }
      
    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const broken_swap& lhs, const broken_swap& rhs) noexcept
    {
      return lhs.x == rhs.x;
    }

    [[nodiscard]]
    friend bool operator!=(const broken_swap& lhs, const broken_swap& rhs) noexcept
    {
      return !(lhs == rhs);
    }

    template<class Stream>
    friend Stream& operator<<(Stream& s, const broken_swap& b)
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

    broken_copy_value_semantics(std::initializer_list<T> list, const allocator_type& alloc = allocator_type{})
      : x(alloc)
    {
      x.reserve(list.size());
      for(auto e : list)
        x.emplace_back(std::make_shared<T>(e));
    };

    broken_copy_value_semantics(const broken_copy_value_semantics&) = default; // Broken!

    broken_copy_value_semantics(const broken_copy_value_semantics& other, const allocator_type& alloc)
      : x(alloc)
    {
      x.reserve(other.x.size());
      for(auto e : other.x)
      {
        x.emplace_back(std::make_shared<T>(*e));
      }
    }

    broken_copy_value_semantics(broken_copy_value_semantics&&) noexcept = default;

    broken_copy_value_semantics(broken_copy_value_semantics&& other, const allocator_type& alloc)
      : x(std::move(other.x), alloc) {}

    broken_copy_value_semantics& operator=(const broken_copy_value_semantics& other)
    {
      auto tmp{other};
      *this = std::move(tmp);

      return *this;
    }

    broken_copy_value_semantics& operator=(broken_copy_value_semantics&&) = default;

    friend void swap(broken_copy_value_semantics& lhs, broken_copy_value_semantics& rhs)
    {
      std::swap(lhs.x, rhs.x);
    }
      
    std::vector<handle_type, allocator_type> x{};

    [[nodiscard]]
    friend bool operator==(const broken_copy_value_semantics& lhs, const broken_copy_value_semantics& rhs) noexcept
    {
      return std::equal(lhs.x.cbegin(), lhs.x.cend(), rhs.x.cbegin(), rhs.x.cend(), [](auto& l, auto& r){
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

    broken_copy_assignment_value_semantics(std::initializer_list<T> list, const allocator_type& alloc = allocator_type{})
      : x(alloc)
    {
      x.reserve(list.size());
      for(auto e : list)
        x.emplace_back(std::make_shared<T>(e));
    };

    broken_copy_assignment_value_semantics(const broken_copy_assignment_value_semantics& other)
      : broken_copy_assignment_value_semantics(other, other.x.get_allocator())
    {}

    broken_copy_assignment_value_semantics(const broken_copy_assignment_value_semantics& other, const allocator_type& alloc)
      : x(alloc)
    {
      x.reserve(other.x.size());
      for(auto e : other.x)
      {
        x.emplace_back(std::make_shared<T>(*e));
      }
    }

    broken_copy_assignment_value_semantics(broken_copy_assignment_value_semantics&&) noexcept = default;

    broken_copy_assignment_value_semantics(broken_copy_assignment_value_semantics&& other, const allocator_type& alloc)
      : x(std::move(other.x), alloc) {}

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
  struct perfectly_normal_beast
  {
    using allocator_type = Allocator;

    perfectly_normal_beast(std::initializer_list<T> list) : x{list} {}

    perfectly_normal_beast(std::initializer_list<T> list, const allocator_type& a) : x{list, a} {}

    perfectly_normal_beast(const allocator_type& a) : x(a) {}

    perfectly_normal_beast(const perfectly_normal_beast&) = default;

    perfectly_normal_beast(const perfectly_normal_beast& other, const allocator_type& alloc) : x(other.x, alloc) {}

    perfectly_normal_beast(perfectly_normal_beast&&) noexcept = default;

    perfectly_normal_beast(perfectly_normal_beast&& other, const allocator_type& alloc) : x(std::move(other.x), alloc) {}

    perfectly_normal_beast& operator=(const perfectly_normal_beast&) = default;

    perfectly_normal_beast& operator=(perfectly_normal_beast&&) = default;

    void swap(perfectly_normal_beast& other) noexcept(noexcept(std::swap(x, other.x)))
    {
      std::swap(x, other.x);
    }

    friend void swap(perfectly_normal_beast& lhs, perfectly_normal_beast& rhs)
      noexcept(noexcept(lhs.swap(rhs)))
    {
      lhs.swap(rhs);
    }
      
    std::vector<T, Allocator> x{};

    [[nodiscard]]
    friend bool operator==(const perfectly_normal_beast& lhs, const perfectly_normal_beast& rhs) noexcept
    {
      return lhs.x == rhs.x;
    }

    [[nodiscard]]
    friend bool operator!=(const perfectly_normal_beast& lhs, const perfectly_normal_beast& rhs) noexcept
    {
      return !(lhs == rhs);
    }

    template<class Stream>
    friend Stream& operator<<(Stream& s, const perfectly_normal_beast& b)
    {
      for(auto i : b.x) s << i << ' ';
      return s;
    }
  };

  template<class T=int, class Handle=std::shared_ptr<T>, class Allocator=std::allocator<Handle>>
  struct perfectly_sharing_beast
  {
    using handle_type = Handle;      
    using allocator_type = Allocator;

    explicit perfectly_sharing_beast(const allocator_type& alloc) : x(alloc)
    {}

    perfectly_sharing_beast(std::initializer_list<T> list, const allocator_type& alloc = allocator_type{})
      : x(alloc)
    {
      x.reserve(list.size());
      for(auto e : list)
        x.emplace_back(std::make_shared<T>(e));
    };

    perfectly_sharing_beast(const perfectly_sharing_beast& other)
      : perfectly_sharing_beast(other, other.x.get_allocator())
    {}

    perfectly_sharing_beast(const perfectly_sharing_beast& other, const allocator_type& alloc)
      : x(alloc)
    {
      x.reserve(other.x.size());
      for(auto e : other.x)
      {
        x.emplace_back(std::make_shared<T>(*e));
      }
    } 

    perfectly_sharing_beast(perfectly_sharing_beast&&) noexcept = default;

    perfectly_sharing_beast(perfectly_sharing_beast&& other, const allocator_type& alloc)
      : x(std::move(other.x), alloc) {}

    perfectly_sharing_beast& operator=(const perfectly_sharing_beast& other)
    {
      if(&other != this)
      {
        auto allocGetter{
          [](const  perfectly_sharing_beast& psb){
            return psb.x.get_allocator();
          }
        };

        sequoia::impl::assignment_helper::assign(*this, other, allocGetter);
      }

      return *this;
    }

    perfectly_sharing_beast& operator=(perfectly_sharing_beast&&) = default;

    void swap(perfectly_sharing_beast& other) noexcept(noexcept(std::swap(x, other.x)))
    {
      std::swap(x, other.x);
    }

    friend void swap(perfectly_sharing_beast& lhs, perfectly_sharing_beast& rhs)
      noexcept(noexcept(lhs.swap(rhs)))
    {
      lhs.swap(rhs);
    }
      
    std::vector<handle_type, allocator_type> x{};

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

    void reset(const allocator_type& alloc)
    {
      const std::vector<handle_type, allocator_type> v(alloc);
      x = v;
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
      x.reserve(x.capacity() + 10);
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

    void swap(doubly_normal_beast& other) noexcept(noexcept(std::swap(x, other.x)) && noexcept(std::swap(y, other.y)))
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
}
