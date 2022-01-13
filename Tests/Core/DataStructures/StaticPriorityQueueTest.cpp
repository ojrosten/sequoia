////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "StaticPriorityQueueTest.hpp"
#include "StaticPriorityQueueTestingUtilities.hpp"

#include "sequoia/Core/DataStructures/StaticPriorityQueue.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view test_static_priority_queue::source_file() const noexcept
  {
    return __FILE__;
  }

  void test_static_priority_queue::run_tests()
  {
    check_depth_0();
    check_depth_1();
    check_depth_2();
    check_depth_3();
  }

  void test_static_priority_queue::check_depth_0()
  {
    using namespace data_structures;

    constexpr static_priority_queue<int, 0> s{};
    static_priority_queue<int, 0> t{};

    check(equality, LINE("Equality of null queues"), t, s);

    check_exception_thrown<std::logic_error>(LINE("Can't push to null queue"), [&t]() { t.push(1); });
    check_exception_thrown<std::logic_error>(LINE("Can't construct non-null null queue"), []() { static_priority_queue<int, 0>{1}; });

  }

  void test_static_priority_queue::check_depth_1()
  {
    using namespace data_structures;
    using queue_t = static_priority_queue<int, 1>;

    constexpr queue_t s{1};
    queue_t t{};
    t.push(2);

    check_semantics(LINE("Standard Semantics"), s, t);

    check_exception_thrown<std::logic_error>(LINE("Trying to push two elements to queue of depth 1"), [&t]() { t.push(1); });
    check_exception_thrown<std::logic_error>(LINE("Can't construct queue of depth 1 with 2 elements"), []() { queue_t{1, 2}; });

    t.pop();
    check(equality, LINE(""), t, queue_t{});

    t.push(1);
    check(equality, LINE(""), t, queue_t{1});
  }

  void test_static_priority_queue::check_depth_2()
  {
    using namespace data_structures;

    {
      using queue_t = static_priority_queue<int, 2>;
      constexpr queue_t s{1, 2};
      queue_t t{4, 3};

      check(equality, LINE(""), s.top(), 2);
      check(equality, LINE(""), t.top(), 4);

      check_semantics(LINE("Standard Semantics"), s, t);

      check_exception_thrown<std::logic_error>(LINE("Trying to push three elements to queue of depth 2"), [&t]() { t.push(1); });
      check_exception_thrown<std::logic_error>(LINE("Can't construct queue of depth 2 with 3 elements"), []() { queue_t{1, 2, 3}; });

      t.pop();
      check(equality, LINE(""), t, queue_t{3});

      t.push(5);
      check(equality, LINE(""), t, queue_t{5, 3});

    }

    {
      struct comp
      {
        constexpr comp(const int mod) : m_mod{mod} {}

        constexpr bool operator()(const int a, const int b) noexcept
        {
          return a%m_mod < b%m_mod;
        }
      private:
        int m_mod{};
      };

      constexpr static_priority_queue<int, 2, comp> s{{3, 2}, comp{3}}, t{{4, 6}, comp{2}};
      check(equality, LINE(""), s.top(), 2);

      check_semantics(LINE("Standard semantics"), s, t);
    }
  }

  constexpr auto test_static_priority_queue::make_static_priority_queue_3()
  {
    using namespace data_structures;

    static_priority_queue<int, 3> s{};

    s.push(0);
    s.pop();
    s.push(6);
    s.push(2);
    s.push(8);

    return s;
  }

  void test_static_priority_queue::check_depth_3()
  {
    using namespace data_structures;
    using queue_t = static_priority_queue<int, 3>;

    constexpr queue_t s{make_static_priority_queue_3()};
    check(equality, LINE(""), s, queue_t{8, 2, 6});

    auto t{s};
    check(equality, LINE(""), t, queue_t{8, 2, 6});

    t.pop();
    check(equality, LINE(""), t, queue_t{6, 2});

    check_semantics(LINE("Standard semantics"), s, t);

    t.pop();
    check(equality, LINE(""), t, queue_t{2});

    t.pop();
    check(equality, LINE(""), t, queue_t{});
  }
}
