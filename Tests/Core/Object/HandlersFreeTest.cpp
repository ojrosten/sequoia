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
    struct value_with_null_address_of_operator
    {
      constexpr std::nullptr_t operator&() const noexcept { return nullptr; }
    };

    struct value_with_deleted_address_of_operator
    {
      void operator&() const = delete;
    };
  }

  [[nodiscard]]
  std::filesystem::path handlers_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void handlers_free_test::run_tests()
  {
    test_by_value_get_ptr_bypasses_the_address_of_operator();
  }

  void handlers_free_test::test_by_value_get_ptr_bypasses_the_address_of_operator()
  {
    using namespace object;

    using null_handler_t    = by_value<value_with_null_address_of_operator>;
    using deleted_handler_t = by_value<value_with_deleted_address_of_operator>;

    STATIC_CHECK(handler<null_handler_t>);
    STATIC_CHECK(handler<deleted_handler_t>);

    STATIC_CHECK([](){
      value_with_null_address_of_operator x{};
      return null_handler_t::get_ptr(x) == std::addressof(x);
    }());

    STATIC_CHECK([](){
      const value_with_null_address_of_operator x{};
      return null_handler_t::get_ptr(x) == std::addressof(x);
    }());

    STATIC_CHECK([](){
      value_with_deleted_address_of_operator x{};
      return deleted_handler_t::get_ptr(x) == std::addressof(x);
    }());

    STATIC_CHECK([](){
      const value_with_deleted_address_of_operator x{};
      return deleted_handler_t::get_ptr(x) == std::addressof(x);
    }());
  }
}
