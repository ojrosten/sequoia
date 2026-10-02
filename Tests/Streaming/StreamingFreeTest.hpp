////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "sequoia/TestFramework/FreeTestCore.hpp"

namespace sequoia::testing
{
  class streaming_free_test final : public free_test
  {
  public:
    using free_test::free_test;

    static std::filesystem::path source_file();

    void run_tests();
  private:
    void test_files();

    void test_parse_integer();

    void test_extract_field();

    void test_extract_text();
  };
}
