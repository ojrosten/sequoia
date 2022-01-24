////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/TestFramework/FreeTestCore.hpp"

namespace sequoia::testing
{
  class concepts_test final : public free_test
  {
  public:
    using free_test::free_test;

    [[nodiscard]]
    std::string_view source_file() const noexcept final;
  private:
    void run_tests() final;

    void test_is_range();

    void test_is_allocator();

    void test_is_serializable();

    void test_is_class_template_instantiable();

    void test_has_allocator_type();

    void test_deep_equality_comparable();
  };
}
