////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "StaticQueueTest.hpp"
#include "StaticQueueTestingUtilities.hpp"

#include "StaticQueue.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view test_static_queue::source_file() const noexcept
  {
    return __FILE__;
  }

  void test_static_queue::run_tests()
  {
    check_depth_0();
    check_depth_1();
    check_depth_2();
  }

  void test_static_queue::check_depth_0()
  {
    using namespace data_structures;

    constexpr static_queue<int, 0> s{};
    static_queue<int, 0> t{};

    check_equality(LINE("Equality of null queues"), t, s);

    check_exception_thrown<std::logic_error>(LINE("Can't push to null queue"), [&t]() { t.push(1); });
    check_exception_thrown<std::logic_error>(LINE("Can't construct non-null null queue"), []() { static_queue<int, 0>{1}; });
  }

  void test_static_queue::check_depth_1()
  {
    using namespace data_structures;

    constexpr static_queue<int, 1> s{1};
    static_queue<int, 1> t{};
    t.push(2);

    check_semantics(LINE("Standard Semantics"), s, t);

    check_exception_thrown<std::logic_error>(LINE("Trying to push two elements to queue of depth 1"), [&t]() { t.push(1); });
    check_exception_thrown<std::logic_error>( LINE("Can't construct queue of depth 1 with 2 elements"), []() { static_queue<int, 1>{1, 2}; });

    t.pop();
    t.push(1);
    check_equality(LINE(""), t, s);
  }

  constexpr auto test_static_queue::make_static_queue_2()
  {
    using namespace data_structures;

    static_queue<int, 2> s{};

    s.push(10);
    s.pop();
    s.push(11);

    return s;
  }
  
  void test_static_queue::check_depth_2()
  {
    using namespace data_structures;

    static_queue<int, 2> a{}, b{1}, c{3, 2};
    constexpr static_queue<int, 2> s{make_static_queue_2()};

    check_semantics(LINE("Standard Semantics"), a, b);
    check_semantics(LINE("Standard Semantics"), b, c);
    check_semantics(LINE("Standard Semantics"), a, c);
    check_semantics(LINE("Standard Semantics"), b, s);

    check_exception_thrown<std::logic_error>(LINE("Can't construct queue of depth 2 with 3 elements"), []() { static_queue<int, 2>{1, 2, 3}; });

    check_equality(LINE(""), a.size(), 0ul);
    check(LINE(""), a.empty());
    
    a.push(5);
    a.push(7);
    check_equality(LINE(""), a, static_queue<int, 2>{5, 7});
    
    check_exception_thrown<std::logic_error>(LINE("Trying to push 3 elements to a queue of depth 2"), [&a]() { a.push(0); });
    
    a.pop();
    check_equality(LINE(""), a, static_queue<int, 2>{7});

    a.push(4);
    check_equality(LINE(""), a, static_queue<int, 2>{7, 4});

    check_exception_thrown<std::logic_error>(LINE("Trying to push 3 elements to a queue of depth 2"), [&a]() { a.push(0); });

    a.pop();
    check_equality(LINE(""), a, static_queue<int, 2>{4});

    a.pop();
    check_equality(LINE(""), a, static_queue<int, 2>{});
  }
}
