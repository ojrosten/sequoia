////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "FaithfulWrapperTest.hpp"
#include "FaithfulWrapperTestingUtilities.hpp"

#include <vector>

namespace sequoia::testing
{
  using namespace object;

  // a+1, b*2
  constexpr object::faithful_wrapper<data> make(int a, double b)
  {
    object::faithful_wrapper<data> d{a, b};
    d.set(a, b);
    d.mutate([](auto& e) { e.a+=1; e.b*=2; });

    return d;
  }

  [[nodiscard]]
  std::string_view faithful_wrapper_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void faithful_wrapper_test::run_tests()
  {
    test_basic_type();
    test_container_type();
    test_aggregate_type();
  }

  void faithful_wrapper_test::test_basic_type()
  {
    using wrapper = faithful_wrapper<int>;

    static_assert(sizeof(wrapper) == sizeof(int));
    static_assert(uniform_wrapper<wrapper>);
    static_assert(transparent_wrapper<wrapper>);

    wrapper w{};
    constexpr wrapper v{1};

    check_semantics("", w, v, std::weak_ordering::less);

    w.set(2);

    check(equality, LINE(""), w, wrapper{2});

    w.mutate([](auto& u) { u *=2; });

    check(equality, LINE(""), w, wrapper{4});
  }

  void faithful_wrapper_test::test_container_type()
  {
    using wrapper = faithful_wrapper<std::vector<int>>;

    static_assert(sizeof(wrapper) == sizeof(std::vector<int>));
    static_assert(uniform_wrapper<wrapper>);
    static_assert(transparent_wrapper<wrapper>);

    wrapper w{}, v{1};

    // TO DO: prefer MSVC/gcc version once the spaceship fully lands elsewhere
#ifdef __clang__
    check_semantics(LINE(""), w, v);
#else
    check_semantics(LINE(""), w, v, std::weak_ordering::less);
#endif

    w.set(2);

    check(equality, LINE(""), w, wrapper{std::vector<int>{2}});

    // TO DO: prefer MSVC/gcc version once the spaceship fully lands elsewhere
#ifdef __clang__
    check_semantics(LINE(""), w, v);
#else
    check_semantics(LINE(""), w, v, std::weak_ordering::greater);
#endif

    v.mutate([](auto& u) { u.push_back(3); });

    check(equality, LINE(""), v, wrapper{std::vector<int>{1, 3}});

    // TO DO: prefer MSVC/gg version once the spaceship fully lands elsewhere
#ifdef __clang__
    check_semantics(LINE(""), w, v);
#else
    check_semantics(LINE(""), w, v, std::weak_ordering::greater);
#endif

    int& element{ v.mutate([](auto& u) -> int& { return u.emplace_back(4); }) };
    check(equality, LINE(""), element, 4);
  }

  void faithful_wrapper_test::test_aggregate_type()
  {
    using wrapper = faithful_wrapper<data>;

    static_assert(sizeof(wrapper) == sizeof(data));
    static_assert(uniform_wrapper<wrapper>);
    static_assert(transparent_wrapper<wrapper>);

    wrapper w{};
    constexpr wrapper v{make(1, 2.0)};

    check(equality, LINE(""), v, wrapper{2, 4.0});

    check_semantics(LINE("Regular semantics"), w, v);
  }
}
