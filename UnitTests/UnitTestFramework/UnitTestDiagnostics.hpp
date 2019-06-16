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
      void test_standard_semantics();
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
      void test_standard_semantics();   
    };

    struct broken_equality { int x{}; };

    constexpr bool operator==(const broken_equality& lhs, const broken_equality& rhs) noexcept
    {
      return lhs.x != rhs.x;
    }

    constexpr bool operator!=(const broken_equality& lhs, const broken_equality& rhs) noexcept
    {
      return lhs.x != rhs.x;
    }

    template<class Stream> Stream& operator<<(Stream& s, const broken_equality& b)
    {
      s << b.x;
      return s;
    }

    struct broken_inequality { int x{}; };

    constexpr bool operator==(const broken_inequality& lhs, const broken_inequality& rhs) noexcept
    {
      return lhs.x != rhs.x;
    }

    constexpr bool operator!=(const broken_inequality& lhs, const broken_inequality& rhs) noexcept
    {
      return lhs.x == rhs.x;
    }

    template<class Stream> Stream& operator<<(Stream& s, const broken_inequality& b)
    {
      s << b.x;
      return s;
    }

    struct broken_copy
    {
      constexpr explicit broken_copy(int a) : x{a} {};

      constexpr broken_copy(const broken_copy& other)
      {
        // Do nothing
      }

      constexpr broken_copy(broken_copy&&) = default;
      
      constexpr broken_copy& operator=(const broken_copy& other) = default;

      constexpr broken_copy& operator=(broken_copy&& other) = default;
      
      int x{};
    };

    constexpr bool operator==(const broken_copy& lhs, const broken_copy& rhs) noexcept
    {
      return lhs.x == rhs.x;
    }

    constexpr bool operator!=(const broken_copy& lhs, const broken_copy& rhs) noexcept
    {
      return !(lhs == rhs);
    }

    template<class Stream> Stream& operator<<(Stream& s, const broken_copy& b)
    {
      s << b.x;
      return s;
    }

    struct broken_move
    {
      constexpr explicit broken_move(int a) : x{a} {};

      constexpr broken_move(const broken_move& other) = default;

      constexpr broken_move(broken_move&&)      
      {
        // Do nothing
      }
      
      constexpr broken_move& operator=(const broken_move& other) = default;

      constexpr broken_move& operator=(broken_move&& other) = default;
      
      int x{};
    };

    constexpr bool operator==(const broken_move& lhs, const broken_move& rhs) noexcept
    {
      return lhs.x == rhs.x;
    }

    constexpr bool operator!=(const broken_move& lhs, const broken_move& rhs) noexcept
    {
      return !(lhs == rhs);
    }

    template<class Stream> Stream& operator<<(Stream& s, const broken_move& b)
    {
      s << b.x;
      return s;
    }

    struct broken_copy_assignment
    {
      constexpr explicit broken_copy_assignment(int a) : x{a} {};

      constexpr broken_copy_assignment(const broken_copy_assignment& other) = default;

      constexpr broken_copy_assignment(broken_copy_assignment&&) = default;
      
      constexpr broken_copy_assignment& operator=(const broken_copy_assignment& other)
      {
        return *this;
      }

      constexpr broken_copy_assignment& operator=(broken_copy_assignment&& other) = default;
      
      int x{};
    };

    constexpr bool operator==(const broken_copy_assignment& lhs, const broken_copy_assignment& rhs) noexcept
    {
      return lhs.x == rhs.x;
    }

    constexpr bool operator!=(const broken_copy_assignment& lhs, const broken_copy_assignment& rhs) noexcept
    {
      return !(lhs == rhs);
    }

    template<class Stream> Stream& operator<<(Stream& s, const broken_copy_assignment& b)
    {
      s << b.x;
      return s;
    }

    struct broken_move_assignment
    {
      constexpr explicit broken_move_assignment(int a) : x{a} {};

      constexpr broken_move_assignment(const broken_move_assignment& other) = default;

      constexpr broken_move_assignment(broken_move_assignment&&) = default;
      
      constexpr broken_move_assignment& operator=(const broken_move_assignment& other)
      {
        return *this;
      }

      constexpr broken_move_assignment& operator=(broken_move_assignment&& other) = default;
      
      int x{};
    };

    constexpr bool operator==(const broken_move_assignment& lhs, const broken_move_assignment& rhs) noexcept
    {
      return lhs.x == rhs.x;
    }

    constexpr bool operator!=(const broken_move_assignment& lhs, const broken_move_assignment& rhs) noexcept
    {
      return !(lhs == rhs);
    }

    template<class Stream> Stream& operator<<(Stream& s, const broken_move_assignment& b)
    {
      s << b.x;
      return s;
    }

    struct perfectly_normal_beast
    {
      int x{};
    };

    constexpr bool operator==(const perfectly_normal_beast& lhs, const perfectly_normal_beast& rhs) noexcept
    {
      return lhs.x == rhs.x;
    }

    constexpr bool operator!=(const perfectly_normal_beast& lhs, const perfectly_normal_beast& rhs) noexcept
    {
      return !(lhs == rhs);
    }

    template<class Stream> Stream& operator<<(Stream& s, const perfectly_normal_beast& b)
    {
      s << b.x;
      return s;
    }
  }
}
