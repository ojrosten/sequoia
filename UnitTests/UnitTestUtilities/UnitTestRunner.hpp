#pragma once

#include "UnitTestUtils.hpp"

namespace sequoia
{
  namespace unit_testing
  {
    class unit_test_runner
    {
    public:
      unit_test_runner(int argc, char** argv);

      unit_test_runner(const unit_test_runner&) = delete;
      unit_test_runner(unit_test_runner&&)      = default;

      void add_test_family(test_family&& f);

      void execute();
    private:
      std::vector<test_family> m_Families;
      std::set<std::string> m_SpecificCases{};
      bool m_Asynchronous{};

      template<class Iter> static void pad_right(Iter begin, Iter end, std::string_view suffix);
      template<class Iter> static void pad_left(Iter begin, Iter end);
    };
  }
}
