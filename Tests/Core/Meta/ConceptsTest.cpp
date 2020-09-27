////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "ConceptsTest.hpp"

#include "Concepts.hpp"

#include "AllocationTestUtilities.hpp"

#include <complex>
#include <set>
#include <vector>


namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view concepts_test::source_file() const noexcept
  {
    return __FILE__;
  }

  struct serializable_thing
  {
    template<class Stream>
    friend Stream& operator<<(Stream& s, const serializable_thing&)
    {
      return s;
    }
  };

  struct non_serializable
  {};

  template<class> struct foo;

  template<>
  struct foo<int> {};

  void concepts_test::run_tests()
  {
    test_is_range();
    test_is_allocator();    
    test_is_serializable();    
    test_is_class_template_instantiable();
    test_has_allocator_type();
  }

  void concepts_test::test_is_range()
  {
    check(LINE(""), []() {
        static_assert(range<std::vector<double>>);
        static_assert(!range<double>);
        return true;
      }()
    );
  }

  void concepts_test::test_is_allocator()
  {
    check(LINE(""), []() {
        static_assert(!alloc<int>);
        static_assert(alloc<std::allocator<int>>);
        return true;
      }()
    );
  }

  void concepts_test::test_has_allocator_type()
  {
    check(LINE(""), []() {
        static_assert(has_allocator_type<std::vector<double>>);
        static_assert(!has_allocator_type<double>);
        return true;
      }()
    );
  }

  void concepts_test::test_is_serializable()
  {
    
    check(LINE(""), []() {
        static_assert(serializable_to<int, std::stringstream>);
        static_assert(serializable_to<serializable_thing, std::stringstream>);
        static_assert(!serializable_to<non_serializable, std::stringstream>);
        return true;
      }()
    );
  }

  void concepts_test::test_is_class_template_instantiable()
  {
    check(LINE(""), []() {
        static_assert(class_template_is_default_instantiable<foo, int>);
        static_assert(!class_template_is_default_instantiable<foo, double>);
        return true;
      }()
    );
  }
}
