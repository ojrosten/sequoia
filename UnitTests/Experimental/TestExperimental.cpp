#include "TestExperimental.hpp"

#include "Algorithms.hpp"

#include <array>

namespace sequoia
{
  template< class ForwardIt >
  constexpr ForwardIt rotate(ForwardIt first, ForwardIt n_first, ForwardIt last)
  {
    if(first == n_first) return last;
    if(last == n_first) return first;

    using namespace std;

    const auto distToEnd{distance(n_first, last)};
    const auto distFromBegin{distance(first, n_first)};
    const auto retIter{next(first, distToEnd)};
    
    const auto dist{min(distToEnd, distFromBegin)};

    const auto unswapped{next(first, dist)};

    while(first != unswapped)
    {
      iter_swap(first++, n_first++);
    }

    if(distToEnd > distFromBegin)
      rotate(unswapped, next(unswapped, dist), last);
    else if(distToEnd < distFromBegin)
      rotate(unswapped, last - dist, last);

    return retIter;
  }
}

namespace sequoia::unit_testing
{
  template<class A>
  constexpr A test_experimental::rotate(const A& a, const int pos)
  {
    A b{a};
    sequoia::rotate(b.begin(), b.begin() + pos, b.end());

    return b;
  }

    
  void test_experimental::run_tests()
  {
    {
      std::array<int, 0> a{};

      auto i{sequoia::rotate(a.begin(), a.begin(), a.end())};

      check_equality(i, a.end(), LINE(""));
    }

    {
      std::array<int, 1> a{1};

      auto i{sequoia::rotate(a.begin(), a.begin(), a.end())};
      check_equality(i, a.end(), LINE(""));

      i = sequoia::rotate(a.begin(), a.end(), a.end());
      check_equality(i, a.begin(), LINE(""));
    }

    {
      using array_t = std::array<int, 2>;
      array_t a{1, 2};

      auto i{sequoia::rotate(a.begin(), a.begin(), a.end())};
      check_equality(i, a.end(), LINE(""));
      check_equality(a, array_t{1,2}, LINE(""));

      i = sequoia::rotate(a.begin(), a.begin() + 1, a.end());
      check_equality(i, a.begin() + 1, LINE(""));
      check_equality(a, array_t{2, 1}, LINE(""));

      i = sequoia::rotate(a.begin(), a.begin() + 2, a.end());
      check_equality(i, a.begin(), LINE(""));
      check_equality(a, array_t{2, 1}, LINE(""));
    }

    {
      using array_t = std::array<int, 3>;
      array_t a{1, 2, 3};

      auto i{sequoia::rotate(a.begin(), a.begin(), a.end())};
      check_equality(i, a.end(), LINE(""));
      check_equality(a, array_t{1,2,3}, LINE(""));

      i = sequoia::rotate(a.begin(), a.begin() + 1, a.end());
      check_equality(i, a.begin() + 2, LINE(""));
      check_equality(a, array_t{2, 3, 1}, LINE(""));

      i = sequoia::rotate(a.begin(), a.begin() + 2, a.end());
      check_equality(i, a.begin() + 1, LINE(""));
      check_equality(a, array_t{1, 2, 3}, LINE(""));

      i = sequoia::rotate(a.begin(), a.begin() + 3, a.end());
      check_equality(i, a.begin(), LINE(""));
      check_equality(a, array_t{1, 2, 3}, LINE(""));
    }

    {
      using array_t = std::array<int, 4>;
      array_t a{1, 2, 3, 4};

      auto i{sequoia::rotate(a.begin(), a.begin(), a.end())};
      check_equality(i, a.end(), LINE(""));
      check_equality(a, array_t{1,2,3,4}, LINE(""));

      i = sequoia::rotate(a.begin(), a.begin() + 1, a.end());
      check_equality(i, a.begin() + 3, LINE(""));
      check_equality(a, array_t{2, 3, 4, 1}, LINE(""));

      i = sequoia::rotate(a.begin(), a.begin() + 2, a.end());
      check_equality(i, a.begin() + 2, LINE(""));
      check_equality(a, array_t{4, 1, 2, 3}, LINE(""));

      i = sequoia::rotate(a.begin(), a.begin() + 3, a.end());
      check_equality(i, a.begin() + 1, LINE(""));
      check_equality(a, array_t{3, 4, 1, 2}, LINE(""));

      i = sequoia::rotate(a.begin(), a.begin() + 4, a.end());
      check_equality(i, a.begin(), LINE(""));
      check_equality(a, array_t{3, 4, 1, 2}, LINE(""));
    }

    {
      using array_t = std::array<int, 5>;
      array_t a{1, 2, 3, 4, 5};

      auto i{sequoia::rotate(a.begin(), a.begin(), a.end())};
      check_equality(i, a.end(), LINE(""));
      check_equality(a, array_t{1,2,3,4,5}, LINE(""));

      i = sequoia::rotate(a.begin(), a.begin() + 1, a.end());
      check_equality(i, a.begin() + 4, LINE(""));
      check_equality(a, array_t{2, 3, 4, 5, 1}, LINE(""));

      i = sequoia::rotate(a.begin(), a.begin() + 2, a.end());
      check_equality(i, a.begin() + 3, LINE(""));
      check_equality(a, array_t{4, 5, 1, 2, 3}, LINE(""));

      i = sequoia::rotate(a.begin(), a.begin() + 3, a.end());
      check_equality(i, a.begin() + 2, LINE(""));
      check_equality(a, array_t{2, 3, 4, 5, 1}, LINE(""));

      i = sequoia::rotate(a.begin(), a.begin() + 4, a.end());
      check_equality(i, a.begin() + 1, LINE(""));
      check_equality(a, array_t{1, 2, 3, 4, 5}, LINE(""));

      i = sequoia::rotate(a.begin(), a.begin() + 5, a.end());
      check_equality(i, a.begin(), LINE(""));
      check_equality(a, array_t{1, 2, 3, 4, 5}, LINE(""));
    }

    {
      using array_t = std::array<int, 6>;
      constexpr auto a{rotate(array_t{6,5,4,3,2,1}, 4)};
      check_equality(array_t{2,1,6,5,4,3}, a, LINE(""));
    }
  }
}
