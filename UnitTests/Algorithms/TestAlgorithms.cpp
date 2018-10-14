#include "TestAlgorithms.hpp"

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

    void test_algorithms::run_tests()
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
    }  
    
    void test_algorithms::sort_basic_type()
    {
      {
        constexpr std::array<int, 0> a{};
        constexpr auto b = sort(a);
        check_equality(a, b, LINE("Sort an empty array"));
      }

      {
        constexpr std::array<int, 1> a{1};
        constexpr auto b = sort(a);
        check_equality(a, b, LINE("Sort an array of one element"));
      }

      {
        constexpr std::array<int, 2> s{1,2};
        constexpr auto b = sort(s);
        check_equality(s, b, LINE("Sort a sorted array of two elements"));

        constexpr std::array<int, 2> u{2,1};
        constexpr auto c = sort(u);
        check_equality(s, c, LINE("Sort an unsorted array of two elements"));

        constexpr std::array<int, 2> t{1,1};
        constexpr auto d = sort(t);
        check_equality(t, d, LINE("Sort an array of two identical elements"));
      }

      {
        constexpr std::array<int, 10> a{5,4,7,8,6,1,2,0,9,3};
        constexpr auto b = sort(a);
        check_equality({0,1,2,3,4,5,6,7,8,9}, b, LINE("Sort digits from 0--9"));

        constexpr auto c = sort(a, std::greater<int>{});
        check_equality({9,8,7,6,5,4,3,2,1,0}, c, LINE("Reverse sort digits from 0--9"));
      }

      {
        constexpr std::array<int, 11> a{5,4,7,8,6,1,10,2,0,9,3};
        constexpr auto b = sort(a);
        check_equality({0,1,2,3,4,5,6,7,8,9,10}, b, LINE("Sort digits from 0--10"));

        constexpr auto c = sort(a, std::greater<int>{});
        check_equality({10,9,8,7,6,5,4,3,2,1,0}, c, LINE("Reverse sort digits from 0--10"));
      }
    }

    void test_algorithms::sort_protective_wrapper()
    {
      using wrapper = utilities::protective_wrapper<int>;
      constexpr std::array<wrapper, 4> a{wrapper{3}, wrapper{2}, wrapper{4}, wrapper{1}};
      constexpr auto b = sort(a);
      for(int i{}; i<4; ++i)
        check_equality(i+1, b[i].get(), LINE("Check array of wrapped ints, element " + std::to_string(i)));
    }  

    void test_algorithms::sort_partial_edge()
    {
      struct null_weight{};
      using edge = maths::partial_edge<null_weight, data_sharing::independent, utilities::protective_wrapper<null_weight>>;
      constexpr std::array<edge, 3> a{edge{1}, edge{2}, edge{0}};
      constexpr auto b = sort(a, [](const edge& lhs, const edge& rhs) { return lhs.target_node() < rhs.target_node();});

      for(std::size_t i{}; i<3; ++i)
        check_equality(i, b[i].target_node(), LINE("Check array of partial edges, element " + std::to_string(i)));
    }    

    void test_algorithms::cluster_basic_type()
    {
      {
        constexpr std::array<int, 9> a{1,2,2,1,3,1,2,2,1};
        constexpr auto b = cluster(a);
        check_equality({1,1,1,1,3,2,2,2,2}, b, LINE("Cluster 9 digits"));
      }
    }

    void test_algorithms::lower_bound_basic_type()
    {
      {
        constexpr std::array<int, 0> a{};
        auto i{lower_bound(a.begin(), a.end(), 1, [](int a, int b){ return a <b;})};
        check(i == a.end(), LINE(""));
      }
      
      {
        constexpr std::array<int, 1> a{2};
        constexpr auto i{*lower_bound(a.begin(), a.end(), 1)};
        check_equality(2, i, LINE(""));

        constexpr auto j{*lower_bound(a.begin(), a.end(), 2)};
        check_equality(2, j, LINE(""));

        auto iter{lower_bound(a.begin(), a.end(), 3)};
        check(iter == a.end(), LINE(""));
      }

      {
        constexpr std::array<int, 2> a{1,2};
        constexpr auto i{*lower_bound(a.begin(), a.end(), 0)};
        check_equality(1, i, LINE(""));
        
        constexpr auto j{*lower_bound(a.begin(), a.end(), 1)};
        check_equality(1, j, LINE(""));

        constexpr auto k{*lower_bound(a.begin(), a.end(), 2)};
        check_equality(2, k, LINE(""));

        auto iter{lower_bound(a.begin(), a.end(), 3)};
        check(iter == a.end(), LINE(""));
      }

      {
        constexpr std::array<int, 2> a{1,1};
        constexpr auto i{*lower_bound(a.begin(), a.end(), 0)};
        check_equality(1, i, LINE(""));
        
        auto iter{lower_bound(a.begin(), a.end(), 1)};
        check(iter == a.begin(), LINE(""));
        check_equality(1, *iter, LINE(""));

        iter = lower_bound(a.begin(), a.end(), 2);
        check(iter == a.end(), LINE(""));
      }

      

      {
        constexpr std::array<int, 3> a{1,2,3};
        constexpr auto i{*lower_bound(a.begin(), a.end(), 0)};
        check_equality(1, i, LINE(""));
        
        constexpr auto j{*lower_bound(a.begin(), a.end(), 1)};
        check_equality(1, j, LINE(""));

        constexpr auto k{*lower_bound(a.begin(), a.end(), 2)};
        check_equality(2, k, LINE(""));

        constexpr auto l{*lower_bound(a.begin(), a.end(), 3)};
        check_equality(3, l, LINE(""));

        auto iter{lower_bound(a.begin(), a.end(), 4)};
        check(iter == a.end(), LINE(""));
      }

      {
        constexpr std::array<int, 3> a{1,1,2};
        constexpr auto i{*lower_bound(a.begin(), a.end(), 0)};
        check_equality(1, i, LINE(""));
        
        auto iter{lower_bound(a.begin(), a.end(), 1)};
        check(iter == a.begin(), LINE(""));
        check_equality(1, *iter, LINE(""));

        constexpr auto k{*lower_bound(a.begin(), a.end(), 2)};
        check_equality(2, k, LINE(""));

        iter = lower_bound(a.begin(), a.end(), 3);
        check(iter == a.end(), LINE(""));
      }

      {
        constexpr std::array<int, 3> a{1,2,2};
        constexpr auto i{*lower_bound(a.begin(), a.end(), 0)};
        check_equality(1, i, LINE(""));
        
        constexpr auto j{*lower_bound(a.begin(), a.end(), 1)};
        check_equality(1, j, LINE(""));

        auto iter{lower_bound(a.begin(), a.end(), 2)};
        check(iter == a.begin()+1, LINE(""));
        check_equality(2, *iter, LINE(""));

        iter = lower_bound(a.begin(), a.end(), 3);
        check(iter == a.end(), LINE(""));
      }

      {
        constexpr std::array<int, 4> a{1,1,2,2};
        auto iter{lower_bound(a.begin(), a.end(), 1)};
        check(iter == a.begin(), LINE(""));
        check_equality(1, *iter, LINE(""));

        iter = lower_bound(a.begin(), a.end(), 2);
        check(iter == a.begin()+2, LINE(""));
        check_equality(2, *iter, LINE(""));
      }

      {
        constexpr std::array<int, 5> a{0,1,2,4,5};
        constexpr auto i{*lower_bound(a.begin(), a.end(), 3)};
        check_equality(4, i, LINE(""));
      }
    }

    void test_algorithms::lower_bound_protective_wrapper()
    {
      using wrapper = utilities::protective_wrapper<int>;
      constexpr std::array<wrapper, 3> a{wrapper{-1}, wrapper{-1}, wrapper{1}};
      constexpr auto w{*lower_bound(a.begin(), a.end(), wrapper{})};
      check_equality(1, w.get(), LINE(""));
        
    }

    void test_algorithms::lower_bound_partial_edge()
    {
      struct null_weight{};
      using edge = maths::partial_edge<null_weight, data_sharing::independent, utilities::protective_wrapper<null_weight>>;
      constexpr std::array<edge, 3> a{edge{0}, edge{2}, edge{3}};
      constexpr auto e{*lower_bound(a.begin(), a.end(), edge{1}, [](const edge& lhs, const edge& rhs) {
            return lhs.target_node() < rhs.target_node();
          })
      };

      check_equality<std::size_t>(2, e.target_node());
    }

    void test_algorithms::upper_bound_basic_type()
    {
      {
        constexpr std::array<int, 0> a{};
        auto i{upper_bound(a.begin(), a.end(), 1, [](int a, int b){ return a <b;})};
        check(i == a.end(), LINE(""));
      }

      {
        constexpr std::array<int, 1> a{2};
        constexpr auto i{*upper_bound(a.begin(), a.end(), 1)};
        check_equality(2, i, LINE(""));

        auto iter{upper_bound(a.begin(), a.end(), 2)};
        check(iter == a.end(), LINE(""));
      }

      {
        constexpr std::array<int, 2> a{1,2};
        constexpr auto i{*upper_bound(a.begin(), a.end(), 0)};
        check_equality(1, i, LINE(""));
        
        constexpr auto j{*upper_bound(a.begin(), a.end(), 1)};
        check_equality(2, j, LINE(""));

        auto iter{upper_bound(a.begin(), a.end(), 2)};
        check(iter == a.end(), LINE(""));
      }

      {
        constexpr std::array<int, 2> a{1,1};
        constexpr auto i{*upper_bound(a.begin(), a.end(), 0)};
        check_equality(1, i, LINE(""));
        
        auto iter{upper_bound(a.begin(), a.end(), 1)};
        check(iter == a.end(), LINE(""));
      }

      {
        constexpr std::array<int, 3> a{1,1,2};
        auto iter{upper_bound(a.begin(), a.end(), 0)};
        check(iter == a.begin(), LINE(""));
        
        check_equality(1, *iter, LINE(""));
        
        iter = upper_bound(a.begin(), a.end(), 1);
        check_equality(2, *iter, LINE(""));

        iter = upper_bound(a.begin(), a.end(), 2);
        check(iter == a.end(), LINE(""));
      }

      {
        constexpr std::array<int, 3> a{1,2,2};
        constexpr auto i{*upper_bound(a.begin(), a.end(), 0)};
        check_equality(1, i, LINE(""));

        auto iter{upper_bound(a.begin(), a.end(), 1)};
        check(iter == a.begin()+1, LINE(""));
        check_equality(2, *iter, LINE(""));

        iter = upper_bound(a.begin(), a.end(), 2);
        check(iter == a.end(), LINE(""));
      }

      {
        constexpr std::array<int, 4> a{1,1,2,2};
        auto iter{upper_bound(a.begin(), a.end(), 0)};
        check(iter == a.begin(), LINE(""));
        check_equality(1, *iter, LINE(""));
        
        iter = upper_bound(a.begin(), a.end(), 1);
        check(iter == a.begin() + 2, LINE(""));
        check_equality(2, *iter, LINE(""));

        iter = upper_bound(a.begin(), a.end(), 2);
        check(iter == a.end(), LINE(""));
      }
      
      {
        constexpr std::array<int, 5> a{0,1,2,4,5};
        constexpr auto i{*upper_bound(a.begin(), a.end(), 4)};
        check_equality(5, i, LINE(""));
      }
    }

    void test_algorithms::equal_range_basic_type()
    {
      {
        constexpr std::array<double, 0> a{};
        auto iters{equal_range(a.begin(), a.end(), 0)};
        check(iters.first == a.end());
        check(iters.second == a.end());
      }

      {
        constexpr std::array<double, 0> a{1};
        auto iters{equal_range(a.begin(), a.end(), 0)};
        check(iters.first == a.begin());
        check(iters.second == a.begin());

        iters = equal_range(a.begin(), a.end(), 1);
        check(iters.first == a.begin());
        check(iters.second == a.end());
      }
    }
  }
}
