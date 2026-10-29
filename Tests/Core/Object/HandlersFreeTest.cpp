////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "HandlersFreeTest.hpp"
#include "sequoia/Core/Object/Handlers.hpp"

#include <cstddef>
#include <memory>

namespace sequoia::testing
{
  namespace
  {
    struct element_with_overloaded_address_of
    {
      constexpr std::nullptr_t operator&() const noexcept { return nullptr; }
    };
  }

  [[nodiscard]]
  std::filesystem::path handlers_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void handlers_free_test::run_tests()
  {
    test_by_value_get_ptr_with_overloaded_address_of();
  }

  void handlers_free_test::test_by_value_get_ptr_with_overloaded_address_of()
  {
    using handler_t = object::by_value<element_with_overloaded_address_of>;

    STATIC_CHECK([](){
      element_with_overloaded_address_of x{};
      return handler_t::get_ptr(x) == std::addressof(x);
    }());

    STATIC_CHECK([](){
      const element_with_overloaded_address_of x{};
      return handler_t::get_ptr(x) == std::addressof(x);
    }());
  }
}
