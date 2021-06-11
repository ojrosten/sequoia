////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "UniformWrapperTest.hpp"
#include "UniformWrapperTestingUtilities.hpp"

#include <vector>

namespace sequoia::testing
{
  // a+1, b*2
  constexpr utilities::uniform_wrapper<data> make(int a, double b)
  {
    utilities::uniform_wrapper<data> d{a, b};
    d.set(a, b);
    d.mutate([](auto& e) { e.a+=1; e.b*=2; });

    return d;
  }

  [[nodiscard]]
  std::string_view uniform_wrapper_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void uniform_wrapper_test::run_tests()
  {
    test_basic_type();
    test_container_type();
    test_aggregate_type();   
  }

  void uniform_wrapper_test::test_basic_type()
  {
    using namespace utilities;

    using wrapper = uniform_wrapper<int>;

    static_assert(sizeof(wrapper) == sizeof(int));

    wrapper w{};
    constexpr wrapper v{1};

    check_semantics("", w, v, std::weak_ordering::less);

    w.set(2);

    check_equality(LINE(""), w, wrapper{2});

    w.mutate([](auto& u) { u *=2; });

    check_equality(LINE(""), w, wrapper{4});
  }
  
  void uniform_wrapper_test::test_container_type()
  {
    using namespace utilities;

    using wrapper = uniform_wrapper<std::vector<int>>;

    static_assert(sizeof(wrapper) == sizeof(std::vector<int>));

    wrapper w{}, v{1};

    // TO DO: prefer MSVC version once the spaceship fully lands elsewhere
#ifdef _MSC_VER
      check_semantics(LINE(""), w, v, std::weak_ordering::less);
#else
      check_semantics(LINE(""), w, v);
#endif

    w.set(2);

    check_equality(LINE(""), w, wrapper{std::vector<int>{2}});

    // TO DO: prefer MSVC version once the spaceship fully lands elsewhere
#ifdef _MSC_VER
      check_semantics(LINE(""), w, v, std::weak_ordering::greater);
#else
      check_semantics(LINE(""), w, v);
#endif

    v.mutate([](auto& u) { u.push_back(3); });

    check_equality(LINE(""), v, wrapper{std::vector<int>{1, 3}});

    // TO DO: prefer MSVC version once the spaceship fully lands elsewhere
#ifdef _MSC_VER
      check_semantics(LINE(""), w, v, std::weak_ordering::greater);
#else
      check_semantics(LINE(""), w, v);
#endif
  }
  
  void uniform_wrapper_test::test_aggregate_type()
  {
    using namespace utilities;

    using wrapper = uniform_wrapper<data>;

    static_assert(sizeof(wrapper) == sizeof(data));

    wrapper w{};
    constexpr wrapper v{make(1, 2.0)};

    check_equality(LINE(""), v, wrapper{2, 4.0});

    check_semantics(LINE("Regular semantics"), w, v);
  }
}
