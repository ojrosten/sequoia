////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Useful specializations for the class template detailed_equality_checker.

    The specializations in this header are for various types defined in std. Internally,
    check_equality is used meaning that there will be automatic, recursive dispatch to 
    other specializations of detailed_equality_checker, if appropriate. For example,
    consider two instances of std::pair<my_type1, mytype2>, x and y. The utilities in this
    header means the call

    check_equality("descripion", logger, x, y);

    will automatically call

    check_equality("automatically enhanced desciption", logger, x.first, y,first)

    and similarly for the second element. In turn, this nested check_equality will use
    a specialization of the detailed_equality_checker of my_type1, should it exist. As
    usual, if the specialization for T does not exist, but T may be interpreted as
    a container holding a type U, then everything will simply work, provided either that
    there exists a specialization of the detailed_equality_checker for U or U is serializable.
 */

#include "ConcreteEquivalenceCheckers.hpp"

namespace sequoia::testing
{
  /*! \brief Detailed comparison of the elements of two instances of std::pair<S,T>

      The elements of a std::pair<S,T> are checked using check_equality; this will recursively
      dispatch to other specializations detailed_equality_checker if appropriate for either
      S or T.
   */
  template<class T, class S>
  struct detailed_equality_checker<std::pair<T, S>> : equivalence_checker<std::pair<T, S>>
  {};

  /*! \brief Detailed comparison of the elements of two instances of std::pair<T...> */  
  template<class... T>
  struct detailed_equality_checker<std::tuple<T...>> : equivalence_checker<std::tuple<T...>>
  {};

  template<class Char, class Traits, alloc Allocator>
  struct detailed_equality_checker<std::basic_string<Char, Traits, Allocator>>
  {
    using string_type = std::basic_string<Char, Traits, Allocator>;
    
    template<test_mode Mode, class Advisor>
    static void check(test_logger<Mode>& logger, const string_type& obtained, const string_type& prediction, tutor<Advisor> advisor)
    {
      using checker = detailed_equality_checker<std::basic_string_view<Char, Traits>>;
      
      checker::check(logger, std::string_view{obtained}, std::string_view{prediction}, std::move(advisor));
    }
  };
}
