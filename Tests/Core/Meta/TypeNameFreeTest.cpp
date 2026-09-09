////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TypeNameFreeTest.hpp"
#include "sequoia/Core/Meta/TypeName.hpp"

namespace sequoia::testing
{
  /** Probes for `type_name`. Deliberately *not* in an anonymous namespace: the three compilers
      spell one of those three different ways, so the names would not be comparable.
   */

  namespace type_name_probes
  {
    class probe_class {};
    struct probe_struct {};
    enum class probe_enum { value };
  }

  using namespace meta;

  [[nodiscard]]
  std::filesystem::path type_name_free_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void type_name_free_test::run_tests()
  {
    test_tidy_type_name();
    test_type_name();
  }

  void type_name_free_test::test_tidy_type_name()
  {
    STATIC_CHECK(tidy_type_name("class foo")  == "foo");
    STATIC_CHECK(tidy_type_name("struct foo") == "foo");
    STATIC_CHECK(tidy_type_name("enum foo")   == "foo");
    STATIC_CHECK(tidy_type_name("foo")        == "foo");
    STATIC_CHECK(tidy_type_name("")           == "");

    // The trailing space in each keyword is what spares a name merely beginning with one.
    STATIC_CHECK(tidy_type_name("classic")    == "classic");
    STATIC_CHECK(tidy_type_name("structure")  == "structure");
    STATIC_CHECK(tidy_type_name("enumerator") == "enumerator");

    // Only the leading keyword: an argument keeps its own.
    STATIC_CHECK(tidy_type_name("class foo<class bar>") == "foo<class bar>");
  }

  /** These hold on clang and gcc whether or not the keyword is removed, since only MSVC emits one.
      Windows is the column that can fail here, and it can do so from a build alone.
   */

  void type_name_free_test::test_type_name()
  {
    using namespace type_name_probes;

    STATIC_CHECK(tidy_type_name(type_name<probe_class>())  == "sequoia::testing::type_name_probes::probe_class");
    STATIC_CHECK(tidy_type_name(type_name<probe_struct>()) == "sequoia::testing::type_name_probes::probe_struct");
    STATIC_CHECK(tidy_type_name(type_name<probe_enum>())   == "sequoia::testing::type_name_probes::probe_enum");
    STATIC_CHECK(tidy_type_name(type_name<int>())          == "int");
  }
}
