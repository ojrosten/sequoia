////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "ArrayUtilitiesTest.hpp"

#include "ArrayUtilities.hpp"

namespace sequoia::unit_testing
{
  class no_default_constructor
  {
  public:
    constexpr explicit no_default_constructor(int i) : m_i{i} {}

    constexpr int get() const noexcept { return m_i; }

    constexpr friend bool operator==(const no_default_constructor& lhs, const no_default_constructor& rhs)
    {
      return lhs.get() == rhs.get();
    }

    template<class Stream> friend Stream& operator<<(Stream& stream, const no_default_constructor& ndc)
    {
      stream << ndc.get();
      return stream;
    }
  private:
    int m_i{};
  };
  
  void array_utilities_test::run_tests()
  {
    using namespace utilities;

    {
      constexpr auto a{to_array<int, 0>({})};
      check_equality({}, a, LINE(""));
    }
    
    {
      constexpr auto a{to_array<int, 5>({3, 6, 9, 1, 0})};
      check_equality({3, 6, 9, 1, 0}, a, LINE(""));
    }

    {
      constexpr auto a{to_array<int, 3>({1, 2, 3}, [](const int e) { return e * 3; })};
      check_equality({3, 6, 9}, a, LINE(""));
    }

    {
      using ndc_t = no_default_constructor;
      constexpr std::array<no_default_constructor, 2>
        a{to_array<ndc_t, 2>({2, 3}, [](int i){ return no_default_constructor{i}; })};

      check_equality({ndc_t{2}, ndc_t{3}}, a, LINE(""));
    }
  }
}
