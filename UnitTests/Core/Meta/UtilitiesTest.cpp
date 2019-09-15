////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "UtilitiesTest.hpp"

#include "Utilities.hpp"

namespace sequoia::unit_testing
{
  
  template<class T>
  struct identity { using type = T;};

  void utilities_test::run_tests()
  {
    test_filtered_sequence();
  }

  void utilities_test::test_filtered_sequence()
  {
    check(LINE("One element which survives"), []() {
        static_assert(std::is_same_v<make_filtered_sequence<void, identity, int>, std::index_sequence<0>>);
    
        return true;
      }()
    );

    check(LINE("One element which is filtered"), []() {
        static_assert(std::is_same_v<make_filtered_sequence<int, identity, int>, std::index_sequence<>>);
    
        return true;
      }()
    );
    
    check(LINE("Two elements, both of which survive"), []() {
        static_assert(std::is_same_v<make_filtered_sequence<void, identity, int, double>, std::index_sequence<0, 1>>);
    
        return true;
      }()
    );

    check(LINE("Two elements, the zeroth of which survives"), []() {
        static_assert(std::is_same_v<make_filtered_sequence<double, identity, int, double>, std::index_sequence<0>>);
    
        return true;
      }()
    );

    check(LINE("Two elements, the first of which survives"), []() {
        static_assert(std::is_same_v<make_filtered_sequence<int, identity, int, double>, std::index_sequence<1>>);
    
        return true;
      }()
    );

    check(LINE("Two elements, both of which are filtered"), []() {
        static_assert(std::is_same_v<make_filtered_sequence<int, identity, int, int>, std::index_sequence<>>);
    
        return true;
      }()
    );
  }
}
