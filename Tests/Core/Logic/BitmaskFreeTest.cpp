////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "BitmaskFreeTest.hpp"
#include "sequoia/Core/Logic/Bitmask.hpp"

namespace sequoia::testing
{
  namespace
  {
    enum class mask { none = 0, a = 1, b = 2, c = 4 };

    template<class Stream>
    Stream& operator<<(Stream& stream, mask m)
    {
      return (stream << static_cast<int>(m));
    }
  }
}

namespace sequoia
{
  template<>
  struct as_bitmask<testing::mask> : std::true_type {};
}

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view bitmask_free_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void bitmask_free_test::run_tests()
  {
    mask m{mask::none};
    m |= mask::a;

    check(equality, LINE("|="), m, mask::a);
  }
}
