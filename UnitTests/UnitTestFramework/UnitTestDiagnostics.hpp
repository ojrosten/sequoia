////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "UnitTestCore.hpp"

namespace sequoia
{
  namespace unit_testing
  {
    class false_positive_diagnostics : public false_positive_test
    {
    public:
      using false_positive_test::false_positive_test;
    private:
      void run_tests() override;

      void basic_tests();
      void test_relative_performance();
      void test_container_checks();
      void test_mixed();
      void test_regular_semantics();
    };

    class false_negative_diagnostics : public false_negative_test
    {
    public:
      using false_negative_test::false_negative_test;
    private:
      void run_tests() override;

      void basic_tests();
      void test_relative_performance();
      void test_container_checks();
      void test_mixed();
      void test_regular_semantics();   
    };

    template<class T=int, class Allocator=std::allocator<int>>
    struct broken_equality
    {
      using allocator_type = Allocator;

      broken_equality(std::initializer_list<T> list) : x{list} {};
      
      broken_equality(std::initializer_list<T> list, const allocator_type& alloc) : x{list, alloc} {};

      broken_equality(const broken_equality&) = default;

      broken_equality(const broken_equality& other, const allocator_type& alloc) : x(other.x, alloc) {}

      broken_equality(broken_equality&&) noexcept = default;

      broken_equality(broken_equality&& other, const allocator_type& alloc) : x(std::move(other.x), alloc) {}

      broken_equality& operator=(const broken_equality&) = default;

      broken_equality& operator=(broken_equality&&) noexcept = default;
      
      std::vector<T, Allocator> x{};

      friend bool operator==(const broken_equality& lhs, const broken_equality& rhs) noexcept
      {
        return lhs.x != rhs.x;
      }

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

      broken_inequality(std::initializer_list<T> list) : x{list} {};

      broken_inequality(std::initializer_list<T> list, const allocator_type& alloc) : x{list, alloc} {};

      broken_inequality(const broken_inequality&) = default;

      broken_inequality(const broken_inequality& other, const allocator_type& alloc) : x(other.x, alloc) {}

      broken_inequality(broken_inequality&&) noexcept = default;

      broken_inequality(broken_inequality&& other, const allocator_type& alloc) : x(std::move(other.x), alloc) {}

      broken_inequality& operator=(const broken_inequality&) = default;

      broken_inequality& operator=(broken_inequality&&) noexcept = default;

      std::vector<T, Allocator> x{};

      friend bool operator==(const broken_inequality& lhs, const broken_inequality& rhs) noexcept
      {
        return lhs.x != rhs.x;
      }

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

      broken_copy(std::initializer_list<T> list) : x{list} {};

      broken_copy(std::initializer_list<T> list, const allocator_type& a) : x{list, a} {};

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
      
      std::vector<T, Allocator> x{};

      friend bool operator==(const broken_copy& lhs, const broken_copy& rhs) noexcept
      {
        return lhs.x == rhs.x;
      }

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
    struct broken_copy_alloc
    {
      using allocator_type = Allocator;

      broken_copy_alloc(std::initializer_list<T> list) : x{list} {};

      broken_copy_alloc(std::initializer_list<T> list, const allocator_type& alloc) : x{list, alloc} {};

      broken_copy_alloc(const broken_copy_alloc&) = default;

      broken_copy_alloc(const broken_copy_alloc&, const allocator_type&)
      {
        // do nothing
      }

      broken_copy_alloc(broken_copy_alloc&&) = default;

      broken_copy_alloc(broken_copy_alloc&& other, const allocator_type& alloc) : x(std::move(other.x), alloc) {}
      
      broken_copy_alloc& operator=(const broken_copy_alloc&) = default;

      broken_copy_alloc& operator=(broken_copy_alloc&&) = default;
      
      std::vector<T, Allocator> x{};

      friend bool operator==(const broken_copy_alloc& lhs, const broken_copy_alloc& rhs) noexcept
      {
        return lhs.x == rhs.x;
      }

      friend bool operator!=(const broken_copy_alloc& lhs, const broken_copy_alloc& rhs) noexcept
      {
        return !(lhs == rhs);
      }

      template<class Stream>
      friend Stream& operator<<(Stream& s, const broken_copy_alloc& b)
      {
        for(auto i : b.x) s << i << ' ';
        return s;
      }
    };

    template<class T=int, class Allocator=std::allocator<int>>
    struct broken_move
    {
      using allocator_type = Allocator;

      broken_move(std::initializer_list<T> list) : x{list} {};

      broken_move(std::initializer_list<T> list, const allocator_type& alloc) : x{list, alloc} {};

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
      
      std::vector<T, Allocator> x{};

      friend bool operator==(const broken_move& lhs, const broken_move& rhs) noexcept
      {
        return lhs.x == rhs.x;
      }

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
    struct broken_move_alloc
    {
      using allocator_type = Allocator;

      broken_move_alloc(std::initializer_list<T> list) : x{list} {};

      broken_move_alloc(std::initializer_list<T> list, const allocator_type& alloc) : x{list, alloc} {};

      broken_move_alloc(const broken_move_alloc&) = default;

      broken_move_alloc(const broken_move_alloc& other, const allocator_type& alloc) : x(other.x, alloc) {}

      broken_move_alloc(broken_move_alloc&&) = default;

      broken_move_alloc(broken_move_alloc&&, const allocator_type&)
      {
        // do nothing
      }
      
      broken_move_alloc& operator=(const broken_move_alloc&) = default;

      broken_move_alloc& operator=(broken_move_alloc&&) = default;
      
      std::vector<T, Allocator> x{};

      friend bool operator==(const broken_move_alloc& lhs, const broken_move_alloc& rhs) noexcept
      {
        return lhs.x == rhs.x;
      }

      friend bool operator!=(const broken_move_alloc& lhs, const broken_move_alloc& rhs) noexcept
      {
        return !(lhs == rhs);
      }

      template<class Stream>
      friend Stream& operator<<(Stream& s, const broken_move_alloc& b)
      {
        for(auto i : b.x) s << i << ' ';
        return s;
      }
    };

    template<class T=int, class Allocator=std::allocator<int>>
    struct broken_copy_assignment
    {
      using allocator_type = Allocator;

      broken_copy_assignment(std::initializer_list<int> list) : x{list} {};

      broken_copy_assignment(std::initializer_list<int> list, const allocator_type& alloc) : x{list, alloc} {};

      broken_copy_assignment(const broken_copy_assignment&) = default;

      broken_copy_assignment(const broken_copy_assignment& other, const allocator_type& alloc) : x(other.x, alloc) {}

      broken_copy_assignment(broken_copy_assignment&&) = default;

      broken_copy_assignment(broken_copy_assignment&& other, const allocator_type& alloc) : x(std::move(other.x), alloc) {}
      
      broken_copy_assignment& operator=(const broken_copy_assignment&)
      {
        return *this;
      }

      broken_copy_assignment& operator=(broken_copy_assignment&&) = default;
      
      std::vector<T, Allocator> x{};

      friend bool operator==(const broken_copy_assignment& lhs, const broken_copy_assignment& rhs) noexcept
      {
        return lhs.x == rhs.x;
      }

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

      broken_move_assignment(std::initializer_list<T> list) : x{list} {};

      broken_move_assignment(std::initializer_list<T> list, const allocator_type& alloc) : x{list, alloc} {};

      broken_move_assignment(const broken_move_assignment&) = default;

      broken_move_assignment(const broken_move_assignment& other, const allocator_type& alloc) : x(other.x, alloc) {}

      broken_move_assignment(broken_move_assignment&&) = default;

      broken_move_assignment(broken_move_assignment&& other, const allocator_type& alloc) : x(std::move(other.x), alloc) {}
      
      broken_move_assignment& operator=(const broken_move_assignment&) = default;
      
      broken_move_assignment& operator=(broken_move_assignment&&)
      {
        return *this;
      }
      
      std::vector<T, Allocator> x{};

      friend bool operator==(const broken_move_assignment& lhs, const broken_move_assignment& rhs) noexcept
      {
        return lhs.x == rhs.x;
      }

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
    struct perfectly_normal_beast
    {
      using allocator_type = Allocator;

      perfectly_normal_beast(std::initializer_list<T> list) : x{list} {};

      perfectly_normal_beast(std::initializer_list<T> list, const allocator_type& a) : x{list, a} {};

      perfectly_normal_beast(const perfectly_normal_beast&) = default;

      perfectly_normal_beast(const perfectly_normal_beast& other, const allocator_type& alloc) : x(other.x, alloc) {}

      perfectly_normal_beast(perfectly_normal_beast&&) noexcept = default;

      perfectly_normal_beast(perfectly_normal_beast&& other, const allocator_type& alloc) : x(std::move(other.x), alloc) {}

      perfectly_normal_beast& operator=(const perfectly_normal_beast&) = default;

      perfectly_normal_beast& operator=(perfectly_normal_beast&&) noexcept = default;
      
      std::vector<T, Allocator> x{};

      friend bool operator==(const perfectly_normal_beast& lhs, const perfectly_normal_beast& rhs) noexcept
      {
        return lhs.x == rhs.x;
      }

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

    template<class T=int, class Allocator=std::allocator<int>>
    struct broken_swap
    {
      using allocator_type = Allocator;

      broken_swap(std::initializer_list<T> list) : x{list} {};

      broken_swap(std::initializer_list<T> list, const allocator_type& a) : x{list, a} {};

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

      friend bool operator==(const broken_swap& lhs, const broken_swap& rhs) noexcept
      {
        return lhs.x == rhs.x;
      }

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

      broken_copy_value_semantics& operator=(const broken_copy_value_semantics&) = default;

      broken_copy_value_semantics& operator=(broken_copy_value_semantics&&) noexcept = default;
      
      std::vector<handle_type, allocator_type> x{};

      friend bool operator==(const broken_copy_value_semantics& lhs, const broken_copy_value_semantics& rhs) noexcept
      {
        return std::equal(lhs.x.cbegin(), lhs.x.cend(), rhs.x.cbegin(), rhs.x.cend(), [](auto& l, auto& r){
            return *l == *r;
          });
      }

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
        : broken_copy_assignment_value_semantics(other, allocator_type{})
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

      broken_copy_assignment_value_semantics& operator=(broken_copy_assignment_value_semantics&&) noexcept = default;
      
      std::vector<handle_type, allocator_type> x{};

      friend bool operator==(const broken_copy_assignment_value_semantics& lhs, const broken_copy_assignment_value_semantics& rhs) noexcept
      {
        return std::equal(lhs.x.cbegin(), lhs.x.cend(), rhs.x.cbegin(), rhs.x.cend(), [](auto& l, auto& r){
            return *l == *r;
          });
      }

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
  }
}
