////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "CreatorFreeTest.hpp"
#include "sequoia/Core/Object/Creator.hpp"

namespace sequoia::testing
{
  using namespace object;

  struct aggregate_type
  {
    int i{};

    [[nodiscard]]
    friend auto operator<=>(const aggregate_type&, const aggregate_type&) = default;

    template<class Stream>
    friend Stream& operator<<(Stream& s, const aggregate_type& a)
    {
      s << a.i;
      return s;
    }
  };

  [[nodiscard]]
  std::filesystem::path creator_free_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void creator_free_test::run_tests()
  {
    {
      using producer_t = producer<int, int>;
      static_assert(creator<producer_t>);

      check(equality, "", producer_t{}.make(42), 42);
    }

    {
      using producer_t = producer<int, aggregate_type>;
      static_assert(creator<producer_t>);

      check(equality, "", producer_t{}.make(42), aggregate_type{42});
    }
  }
}
