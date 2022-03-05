////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "ExperimentalTest.hpp"

#include <array>

#include <algorithm>

namespace sequoia::experimental
{
  /// taken from stl_algo.h but rendered constexpr
    template<typename _BidirectionalIterator, typename _Distance,
	   typename _Compare>
    constexpr
    void
    __merge_without_buffer(_BidirectionalIterator __first,
			   _BidirectionalIterator __middle,
			   _BidirectionalIterator __last,
			   _Distance __len1, _Distance __len2,
			   _Compare __comp)
    {
      if (__len1 == 0 || __len2 == 0)
	return;

      if (__len1 + __len2 == 2)
	{
	  if (__comp(__middle, __first))
	    std::iter_swap(__first, __middle);
	  return;
	}

      _BidirectionalIterator __first_cut = __first;
      _BidirectionalIterator __second_cut = __middle;
      _Distance __len11 = 0;
      _Distance __len22 = 0;
      if (__len1 > __len2)
	{
	  __len11 = __len1 / 2;
	  std::advance(__first_cut, __len11);
	  __second_cut
	    = std::__lower_bound(__middle, __last, *__first_cut,
				 __gnu_cxx::__ops::__iter_comp_val(__comp));
	  __len22 = std::distance(__middle, __second_cut);
	}
      else
	{
	  __len22 = __len2 / 2;
	  std::advance(__second_cut, __len22);
	  __first_cut
	    = std::__upper_bound(__first, __middle, *__second_cut,
				 __gnu_cxx::__ops::__val_comp_iter(__comp));
	  __len11 = std::distance(__first, __first_cut);
	}

      _BidirectionalIterator __new_middle
	= std::rotate(__first_cut, __middle, __second_cut);
      experimental::__merge_without_buffer(__first, __first_cut, __new_middle,
				  __len11, __len22, __comp);
      experimental::__merge_without_buffer(__new_middle, __second_cut, __last,
				  __len1 - __len11, __len2 - __len22, __comp);
    }

   /// taken from stl_algo.h but rendered constexpr
   template<typename _RandomAccessIterator, typename _Compare>
   constexpr
   void
    __inplace_stable_sort(_RandomAccessIterator __first,
			  _RandomAccessIterator __last, _Compare __comp)
    {
      if (__last - __first < 15)
	{
      std::__insertion_sort(__first, __last, __comp);
	  return;
    }
      _RandomAccessIterator __middle = __first + (__last - __first) / 2;
      experimental::__inplace_stable_sort(__first, __middle, __comp);
      experimental::__inplace_stable_sort(__middle, __last, __comp);
      experimental::__merge_without_buffer(__first, __middle, __last,
                                           __middle - __first,
                                           __last - __middle,
                                           __comp);
    }

  /// a constexpr version of stable_sort, based on the gcc implementation
  template<typename _RandomAccessIterator, typename _Compare>
    constexpr
    void
    stable_sort(_RandomAccessIterator __first, _RandomAccessIterator __last,
		  _Compare __comp)
    {
      if (std::is_constant_evaluated())
      {
        if (__first == __last)
          return;

        experimental::__inplace_stable_sort(__first, __last, __gnu_cxx::__ops::__iter_comp_iter(__comp));
      }
      else
      {
        std::__stable_sort(__first, __last, __gnu_cxx::__ops::__iter_comp_iter(__comp));
      }
    }
}

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view experimental_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void experimental_test::run_tests()
  {
    {
      using array_t = std::array<int, 3>;
    
      constexpr auto a{
        [](){
          array_t a{42, 1, 4};
          experimental::stable_sort(a.begin(), a.end(), [](int lhs, int rhs){
            return lhs < rhs;
          });

          return a;
        }()
      };


      check(equality, LINE(""), a, array_t{1, 4, 42});
    }

    {
      using pair_t = std::pair<int, int>;
      using array_t = std::array<pair_t, 4>;
      
      constexpr auto a{
        [](){
          array_t a{pair_t{42,42}, pair_t{42, 1}, pair_t{42, 4}, pair_t{1, 100}};
          experimental::stable_sort(a.begin(), a.end(), [](const pair_t& lhs, const pair_t& rhs){
            return lhs.first < rhs.first;
            });

          return a;
        }()
      };


      check(equality, LINE(""), a, array_t{pair_t{1, 100}, pair_t{42,42}, pair_t{42, 1}, pair_t{42, 4}});
    }
  }
}
