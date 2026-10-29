////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "ContainerAdaptorUtilitiesFreeTest.hpp"
#include "sequoia/Core/ContainerUtilities/ContainerAdaptorUtilities.hpp"
#include "sequoia/Core/DataStructures/PriorityQueue.hpp"
#include "sequoia/Core/DataStructures/Queue.hpp"
#include "sequoia/Core/DataStructures/Stack.hpp"

#include <queue>
#include <stack>
#include <vector>

namespace sequoia::testing
{
  namespace
  {
    template<class Adaptor>
    inline constexpr bool element_popped_next_admits_v{
      requires(const Adaptor& adaptor) { utilities::element_popped_next(adaptor); }
    };

    struct with_front_and_top
    {
      [[nodiscard]]
      int front() const { return 0; }

      [[nodiscard]]
      int top() const { return 0; }

      void pop() {}
    };
  }

  [[nodiscard]]
  std::filesystem::path container_adaptor_utilities_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void container_adaptor_utilities_free_test::run_tests()
  {
    test_admitted_types();
    test_element_popped_next();
  }

  void container_adaptor_utilities_free_test::test_admitted_types()
  {
    STATIC_CHECK(element_popped_next_admits_v<data_structures::queue<int>>);
    STATIC_CHECK(element_popped_next_admits_v<data_structures::stack<int>>);
    STATIC_CHECK(element_popped_next_admits_v<data_structures::priority_queue<int>>);
    STATIC_CHECK(element_popped_next_admits_v<std::queue<int>>);
    STATIC_CHECK(element_popped_next_admits_v<std::stack<int>>);
    STATIC_CHECK(element_popped_next_admits_v<std::priority_queue<int>>);

    // A vector has a front, but no pop to remove it
    STATIC_CHECK(!element_popped_next_admits_v<std::vector<int>>);
    STATIC_CHECK(!element_popped_next_admits_v<with_front_and_top>);
  }

  void container_adaptor_utilities_free_test::test_element_popped_next()
  {
    STATIC_CHECK([]() {
      data_structures::queue<int> q{};
      q.push(1);
      q.push(2);
      return utilities::element_popped_next(q);
    }() == 1);

    STATIC_CHECK([]() {
      data_structures::stack<int> s{};
      s.push(1);
      s.push(2);
      return utilities::element_popped_next(s);
    }() == 2);

    STATIC_CHECK([]() {
      data_structures::priority_queue<int> q{};
      q.push(1);
      q.push(3);
      q.push(2);
      return utilities::element_popped_next(q);
    }() == 3);

    {
      std::queue<int> q{};
      q.push(1);
      q.push(2);
      check(equality, "The front of a std::queue", utilities::element_popped_next(q), 1);
      check("A reference to the front of a std::queue", &utilities::element_popped_next(q) == &q.front());
    }

    {
      std::stack<int> s{};
      s.push(1);
      s.push(2);
      check(equality, "The top of a std::stack", utilities::element_popped_next(s), 2);
    }

    {
      std::priority_queue<int> q{};
      q.push(1);
      q.push(3);
      q.push(2);
      check(equality, "The top of a std::priority_queue", utilities::element_popped_next(q), 3);
    }

    {
      std::stack<bool, std::vector<bool>> s{};
      s.push(false);
      s.push(true);
      // element_popped_next returns a proxy here, which converts to bool
      const bool top{utilities::element_popped_next(s)};
      check(equality, "The top of a std::stack over a std::vector<bool>", top, true);
    }
  }
}
