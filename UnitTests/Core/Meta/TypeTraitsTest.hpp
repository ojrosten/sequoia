////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "RegularTestCore.hpp"

namespace sequoia::unit_testing
{
  class type_traits_test final : public free_test
  {
  public:
    using free_test::free_test;

    [[nodiscard]]
    std::string_view source_file_name() const noexcept final;
  private:
    void run_tests() final;

    void test_variadic_traits();

    void test_base_of_head();
    
    void test_resolve_to_copy_constructor();

    void test_is_const_pointer();

    void test_is_const_reference();

    void test_is_orderable();

    void test_is_equal_to_comparable();

    void test_is_not_equal_to_comparable();

    void test_is_container();

    void test_is_allocator();
    
    void test_has_default_constructor();

    void test_has_allocator_type();

    void test_serializability();

    void test_class_template_instantantiability();
  };
}
