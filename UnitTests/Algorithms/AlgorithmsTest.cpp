////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////


#include "AlgorithmsTest.hpp"

#include "Algorithms.hpp"
#include "ProtectiveWrapper.hpp"
#include "Edge.hpp"
#include "SharingPolicies.hpp"

#include <array>

namespace sequoia
{
  namespace unit_testing
  {
    template<class T, std::size_t N, class Comparer=std::less<T>>
    constexpr std::array<T, N> sort(std::array<T,N> a, Comparer comp = Comparer{})
    {
      sequoia::sort(std::begin(a), std::end(a), comp);      
      return a;
    }

    template<class T, std::size_t N, class Comparer=std::equal_to<T>>
    constexpr std::array<T, N> cluster(std::array<T,N> a, Comparer comp = Comparer{})
    {
      sequoia::cluster(std::begin(a), std::end(a), comp);      
      return a;
    }

    template<class A>
    constexpr A rotate(A a, const int pos)
    {
      sequoia::rotate(a.begin(), a.begin() + pos, a.end());

      return a;
    }

    void algorithms_test::run_tests()
    {
      sort_basic_type();
      sort_protective_wrapper();
      sort_partial_edge();

      cluster_basic_type();

      lower_bound_basic_type();
      lower_bound_protective_wrapper();
      lower_bound_partial_edge();

      upper_bound_basic_type();

      equal_range_basic_type();

      equality();

      test_rotate();
    }  
    
    void algorithms_test::sort_basic_type()
    {
      {
        constexpr std::array<int, 0> a{};
        constexpr auto b = sort(a);
        check_equality(b, a, LINE("Sort an empty array"));
      }

      {
        constexpr std::array<int, 1> a{1};
        constexpr auto b = sort(a);
        check_equality(b, a, LINE("Sort an array of one element"));
      }

      {
        constexpr std::array<int, 2> s{1,2};
        constexpr auto b = sort(s);
        check_equality(b, s, LINE("Sort a sorted array of two elements"));

        constexpr std::array<int, 2> u{2,1};
        constexpr auto c = sort(u);
        check_equality(c, s, LINE("Sort an unsorted array of two elements"));

        constexpr std::array<int, 2> t{1,1};
        constexpr auto d = sort(t);
        check_equality(d, t, LINE("Sort an array of two identical elements"));
      }

      {
        constexpr std::array<int, 10> a{5,4,7,8,6,1,2,0,9,3};
        constexpr auto b = sort(a);
        check_equality(b, {0,1,2,3,4,5,6,7,8,9}, LINE("Sort digits from 0--9"));

        constexpr auto c = sort(a, std::greater<int>{});
        check_equality(c, {9,8,7,6,5,4,3,2,1,0}, LINE("Reverse sort digits from 0--9"));
      }

      {
        constexpr std::array<int, 11> a{5,4,7,8,6,1,10,2,0,9,3};
        constexpr auto b = sort(a);
        check_equality(b, {0,1,2,3,4,5,6,7,8,9,10}, LINE("Sort digits from 0--10"));

        constexpr auto c = sort(a, std::greater<int>{});
        check_equality(c, {10,9,8,7,6,5,4,3,2,1,0}, LINE("Reverse sort digits from 0--10"));
      }
    }

    void algorithms_test::sort_protective_wrapper()
    {
      using wrapper = utilities::protective_wrapper<int>;
      constexpr std::array<wrapper, 4> a{wrapper{3}, wrapper{2}, wrapper{4}, wrapper{1}};
      constexpr auto b = sort(a);
      for(int i{}; i<4; ++i)
        check_equality(b[i].get(), i+1, LINE("Check array of wrapped ints, element " + std::to_string(i)));
    }  

    void algorithms_test::sort_partial_edge()
    {
      struct null_weight{};
      using edge = maths::partial_edge<null_weight, data_sharing::independent, utilities::protective_wrapper<null_weight>>;
      constexpr std::array<edge, 3> a{edge{1}, edge{2}, edge{0}};
      constexpr auto b = sort(a, [](const edge& lhs, const edge& rhs) { return lhs.target_node() < rhs.target_node();});

      for(std::size_t i{}; i<3; ++i)
        check_equality(b[i].target_node(), i, LINE("Check array of partial edges, element " + std::to_string(i)));
    }    

    void algorithms_test::cluster_basic_type()
    {
      {
        constexpr std::array<int, 9> a{1,2,2,1,3,1,2,2,1};
        constexpr auto b = cluster(a);
        check_equality(b, {1,1,1,1,3,2,2,2,2}, LINE("Cluster 9 digits"));
      }
    }

    void algorithms_test::lower_bound_basic_type()
    {
      {
        constexpr std::array<int, 0> a{};
        auto i{lower_bound(a.begin(), a.end(), 1, [](int a, int b){ return a <b;})};
        check(i == a.end(), LINE(""));
      }
      
      {
        constexpr std::array<int, 1> a{2};
        constexpr auto i{*lower_bound(a.begin(), a.end(), 1)};
        check_equality(i, 2, LINE(""));

        constexpr auto j{*lower_bound(a.begin(), a.end(), 2)};
        check_equality(j, 2, LINE(""));

        auto iter{lower_bound(a.begin(), a.end(), 3)};
        check(iter == a.end(), LINE(""));
      }

      {
        constexpr std::array<int, 2> a{1,2};
        constexpr auto i{*lower_bound(a.begin(), a.end(), 0)};
        check_equality(i, 1, LINE(""));
        
        constexpr auto j{*lower_bound(a.begin(), a.end(), 1)};
        check_equality(j, 1, LINE(""));

        constexpr auto k{*lower_bound(a.begin(), a.end(), 2)};
        check_equality(k, 2, LINE(""));

        auto iter{lower_bound(a.begin(), a.end(), 3)};
        check(iter == a.end(), LINE(""));
      }

      {
        constexpr std::array<int, 2> a{1,1};
        constexpr auto i{*lower_bound(a.begin(), a.end(), 0)};
        check_equality(i, 1, LINE(""));
        
        auto iter{lower_bound(a.begin(), a.end(), 1)};
        check(iter == a.begin(), LINE(""));
        check_equality(*iter, 1, LINE(""));

        iter = lower_bound(a.begin(), a.end(), 2);
        check(iter == a.end(), LINE(""));
      }

      

      {
        constexpr std::array<int, 3> a{1,2,3};
        constexpr auto i{*lower_bound(a.begin(), a.end(), 0)};
        check_equality(i, 1, LINE(""));
        
        constexpr auto j{*lower_bound(a.begin(), a.end(), 1)};
        check_equality(j, 1, LINE(""));

        constexpr auto k{*lower_bound(a.begin(), a.end(), 2)};
        check_equality(k, 2, LINE(""));

        constexpr auto l{*lower_bound(a.begin(), a.end(), 3)};
        check_equality(l, 3, LINE(""));

        auto iter{lower_bound(a.begin(), a.end(), 4)};
        check(iter == a.end(), LINE(""));
      }

      {
        constexpr std::array<int, 3> a{1,1,2};
        constexpr auto i{*lower_bound(a.begin(), a.end(), 0)};
        check_equality(i, 1, LINE(""));
        
        auto iter{lower_bound(a.begin(), a.end(), 1)};
        check(iter == a.begin(), LINE(""));
        check_equality(*iter, 1, LINE(""));

        constexpr auto k{*lower_bound(a.begin(), a.end(), 2)};
        check_equality(k, 2, LINE(""));

        iter = lower_bound(a.begin(), a.end(), 3);
        check(iter == a.end(), LINE(""));
      }

      {
        constexpr std::array<int, 3> a{1,2,2};
        constexpr auto i{*lower_bound(a.begin(), a.end(), 0)};
        check_equality(i, 1, LINE(""));
        
        constexpr auto j{*lower_bound(a.begin(), a.end(), 1)};
        check_equality(j, 1, LINE(""));

        auto iter{lower_bound(a.begin(), a.end(), 2)};
        check(iter == a.begin()+1, LINE(""));
        check_equality(*iter, 2, LINE(""));

        iter = lower_bound(a.begin(), a.end(), 3);
        check(iter == a.end(), LINE(""));
      }

      {
        constexpr std::array<int, 4> a{1,1,2,2};
        auto iter{lower_bound(a.begin(), a.end(), 1)};
        check(iter == a.begin(), LINE(""));
        check_equality(*iter, 1, LINE(""));

        iter = lower_bound(a.begin(), a.end(), 2);
        check(iter == a.begin()+2, LINE(""));
        check_equality(*iter, 2, LINE(""));
      }

      {
        constexpr std::array<int, 5> a{0,1,2,4,5};
        constexpr auto i{*lower_bound(a.begin(), a.end(), 3)};
        check_equality(i, 4, LINE(""));
      }
    }

    void algorithms_test::lower_bound_protective_wrapper()
    {
      using wrapper = utilities::protective_wrapper<int>;
      constexpr std::array<wrapper, 3> a{wrapper{-1}, wrapper{-1}, wrapper{1}};
      constexpr auto w{*lower_bound(a.begin(), a.end(), wrapper{})};
      check_equality(w.get(), 1, LINE(""));
        
    }

    void algorithms_test::lower_bound_partial_edge()
    {
      struct null_weight{};
      using edge = maths::partial_edge<null_weight, data_sharing::independent, utilities::protective_wrapper<null_weight>>;
      constexpr std::array<edge, 3> a{edge{0}, edge{2}, edge{3}};
      constexpr auto e{*lower_bound(a.begin(), a.end(), edge{1}, [](const edge& lhs, const edge& rhs) {
            return lhs.target_node() < rhs.target_node();
          })
      };

      check_equality(2ul, e.target_node(), LINE(""));
    }

    void algorithms_test::upper_bound_basic_type()
    {
      {
        constexpr std::array<int, 0> a{};
        auto i{upper_bound(a.begin(), a.end(), 1, [](int a, int b){ return a <b;})};
        check(i == a.end(), LINE(""));
      }

      {
        constexpr std::array<int, 1> a{2};
        constexpr auto i{*upper_bound(a.begin(), a.end(), 1)};
        check_equality(i, 2, LINE(""));

        auto iter{upper_bound(a.begin(), a.end(), 2)};
        check(iter == a.end(), LINE(""));
      }

      {
        constexpr std::array<int, 2> a{1,2};
        constexpr auto i{*upper_bound(a.begin(), a.end(), 0)};
        check_equality(i, 1, LINE(""));
        
        constexpr auto j{*upper_bound(a.begin(), a.end(), 1)};
        check_equality(j, 2, LINE(""));

        auto iter{upper_bound(a.begin(), a.end(), 2)};
        check(iter == a.end(), LINE(""));
      }

      {
        constexpr std::array<int, 2> a{1,1};
        constexpr auto i{*upper_bound(a.begin(), a.end(), 0)};
        check_equality(i, 1, LINE(""));
        
        auto iter{upper_bound(a.begin(), a.end(), 1)};
        check(iter == a.end(), LINE(""));
      }

      {
        constexpr std::array<int, 3> a{1,1,2};
        auto iter{upper_bound(a.begin(), a.end(), 0)};
        check(iter == a.begin(), LINE(""));
        
        check_equality(*iter, 1, LINE(""));
        
        iter = upper_bound(a.begin(), a.end(), 1);
        check_equality(*iter, 2, LINE(""));

        iter = upper_bound(a.begin(), a.end(), 2);
        check(iter == a.end(), LINE(""));
      }

      {
        constexpr std::array<int, 3> a{1,2,2};
        constexpr auto i{*upper_bound(a.begin(), a.end(), 0)};
        check_equality(i, 1, LINE(""));

        auto iter{upper_bound(a.begin(), a.end(), 1)};
        check(iter == a.begin()+1, LINE(""));
        check_equality(*iter, 2, LINE(""));

        iter = upper_bound(a.begin(), a.end(), 2);
        check(iter == a.end(), LINE(""));
      }

      {
        constexpr std::array<int, 4> a{1,1,2,2};
        auto iter{upper_bound(a.begin(), a.end(), 0)};
        check(iter == a.begin(), LINE(""));
        check_equality(*iter, 1, LINE(""));
        
        iter = upper_bound(a.begin(), a.end(), 1);
        check(iter == a.begin() + 2, LINE(""));
        check_equality(*iter, 2, LINE(""));

        iter = upper_bound(a.begin(), a.end(), 2);
        check(iter == a.end(), LINE(""));
      }
      
      {
        constexpr std::array<int, 5> a{0,1,2,4,5};
        constexpr auto i{*upper_bound(a.begin(), a.end(), 4)};
        check_equality(i, 5, LINE(""));
      }
    }

    void algorithms_test::equal_range_basic_type()
    {
      {
        constexpr std::array<double, 0> a{};
        auto iters{equal_range(a.begin(), a.end(), 0)};
        check(iters.first == a.end(), LINE(""));
        check(iters.second == a.end(), LINE(""));
      }

      {
        constexpr std::array<double, 0> a{1};
        auto iters{equal_range(a.begin(), a.end(), 0)};
        check(iters.first == a.begin(), LINE(""));
        check(iters.second == a.begin(), LINE(""));

        iters = equal_range(a.begin(), a.end(), 1);
        check(iters.first == a.begin(), LINE(""));
        check(iters.second == a.end(), LINE(""));
      }
    }

    void algorithms_test::equality()
    {
      constexpr std::array<int, 2> a{1,2};
      constexpr std::array<int, 3> b{1,2, 3};

      constexpr bool f{equal(a.begin(), a.end(), b.begin(), b.end())};
      check(!f, LINE(""));
      
      constexpr bool t{equal(a.begin(), a.end(), b.begin(), b.end() - 1)};
      check(t, LINE(""));
    }

    void algorithms_test::test_rotate()
    {
      {
        std::array<int, 0> a{};

        auto i{sequoia::rotate(a.begin(), a.begin(), a.end())};

        check_equality(a.end(), i, LINE(""));
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
        check_equality(a, array_t{2,1,6,5,4,3}, LINE(""));
      }
    }
  }
}
