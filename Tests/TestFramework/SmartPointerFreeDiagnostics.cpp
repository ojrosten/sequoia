////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "SmartPointerFreeDiagnostics.hpp"
#include "ElementaryFreeDiagnosticsUtilities.hpp"

#include "sequoia/TestFramework/ConcreteTypeCheckers.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path smart_pointer_false_negative_free_diagnostics::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void smart_pointer_false_negative_free_diagnostics::run_tests()
  {
    test_unique_ptr();
    test_shared_ptr();
    test_weak_ptr();
  }

  void smart_pointer_false_negative_free_diagnostics::test_unique_ptr()
  {
    {
      using ptr_t = std::unique_ptr<int>;
      check(equality, "Equality for null vs. not null", ptr_t{}, std::make_unique<int>(42));
      check(equality, "Equality for not null vs. null ", std::make_unique<int>(42), ptr_t{});
      check(equality, "Equality for different pointers", std::make_unique<int>(42), std::make_unique<int>(42));
      check(equivalence, "Equivalence for null vs. not null", ptr_t{}, std::make_unique<int>(42));
      check(equivalence, "Equivalence for not null vs. null ", std::make_unique<int>(42), ptr_t{});
      check(equivalence, "Different pointers pointing to different values", std::make_unique<int>(42), std::make_unique<int>(43));

      check(equivalence,
            reporter{"Advice for different pointers pointing to different values"},
            std::make_unique<int>(42),
            std::make_unique<int>(43),
            tutor{[](int, int) { return "int advice"; }});
    }

    {
      check(equivalence,
            reporter{"Different pointers pointing to different values"},
            std::make_unique<only_equivalence_checkable>(1.0),
            std::make_unique<only_equivalence_checkable>(2.0));
    }

    {
      check(equivalence,
            reporter{"Different pointers pointing to different values"},
            std::make_unique<only_weakly_checkable>(42, 1.0),
            std::make_unique<only_weakly_checkable>(43, -2.0));
    }

    {
      using type = std::tuple<int, only_equivalence_checkable, only_weakly_checkable>;
      check(equivalence,
            reporter{"Different pointers pointing to different values"},
            std::make_unique<type>(1, only_equivalence_checkable{2.0}, only_weakly_checkable{42, 1.0}),
            std::make_unique<type>(-1, only_equivalence_checkable{3.0}, only_weakly_checkable{43, -2.0}));
    }
  }

  void smart_pointer_false_negative_free_diagnostics::test_shared_ptr()
  {
    {
      using ptr_t = std::shared_ptr<int>;
      check(equality, "Equality for null vs. not null", ptr_t{}, std::make_shared<int>(42));
      check(equality, "Equality for not null vs. null ", std::make_shared<int>(42), ptr_t{});
      check(equality, "Equality for different pointers", std::make_shared<int>(42), std::make_shared<int>(42));
      check(equivalence, "Equivalence for null vs. not null", ptr_t{}, std::make_shared<int>(42));
      check(equivalence, "Equivalence for not null vs. null ", std::make_shared<int>(42), ptr_t{});
      check(equivalence, "Different pointers pointing to different values", std::make_shared<int>(42), std::make_shared<int>(43));

      check(equivalence,
            reporter{"Advice for different pointers pointing to different values"},
            std::make_shared<int>(42),
            std::make_shared<int>(43),
            tutor{[](int, int) { return "int advice"; }});
    }

    {
      using type = std::tuple<int, only_equivalence_checkable, only_weakly_checkable>;
      check(equivalence,
            reporter{"Different pointers pointing to different values"},
            std::make_shared<type>(1, only_equivalence_checkable{2.0}, only_weakly_checkable{42, 1.0}),
            std::make_shared<type>(-1, only_equivalence_checkable{3.0}, only_weakly_checkable{43, -2.0}));
    }
  }

  void smart_pointer_false_negative_free_diagnostics::test_weak_ptr()
  {

    {
      using wptr_t = std::weak_ptr<int>;

      auto p{std::make_shared<int>(42)}, q{std::make_shared<int>(43)};
      wptr_t wp{p}, wq{q};

      check(equality, "Equality for null vs. not null", wptr_t{}, wp);
      check(equality, "Equality for not null vs. null ", wp, wptr_t{});
      check(equality, "Equality for different pointers", wp, wq);
      check(equivalence, "Equivalence for null vs. not null", wptr_t{}, wp);
      check(equivalence, "Equivalence for not null vs. null ", wp, wptr_t{});
      check(equivalence, "Different pointers pointing to different values", wp, wq);
    }
  }

  [[nodiscard]]
  std::filesystem::path smart_pointer_false_positive_free_diagnostics::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void smart_pointer_false_positive_free_diagnostics::run_tests()
  {
    test_unique_ptr();
    test_shared_ptr();
    test_weak_ptr();
  }

  void smart_pointer_false_positive_free_diagnostics::test_unique_ptr()
  {
    {
      using ptr_t = std::unique_ptr<int>;
      ptr_t p{}, q{std::make_unique<int>(42)};
      check(equality, "Equality of null pointer with itself", p, p);
      check(equality, "Equality of non-null pointer with itself", q, q);
      check(equivalence, "Different pointers pointing to identical values", std::make_unique<int>(42), std::make_unique<int>(42));
    }

    {
      check(equivalence,
            reporter{"Different pointers pointing to identical values"},
            std::make_unique<only_equivalence_checkable>(1.0),
            std::make_unique<only_equivalence_checkable>(1.0));
    }

    {
      check(equivalence,
            reporter{"Different pointers pointing to identical values"},
            std::make_unique<only_weakly_checkable>(42, -2.0),
            std::make_unique<only_weakly_checkable>(42, -2.0));
    }

    {
      using type = std::tuple<int, only_equivalence_checkable, only_weakly_checkable>;
      check(equivalence,
            reporter{"Different pointers pointing to identical values"},
            std::make_unique<type>(-1, only_equivalence_checkable{2.0}, only_weakly_checkable{42, 1.0}),
            std::make_unique<type>(-1, only_equivalence_checkable{2.0}, only_weakly_checkable{42, 1.0}));
    }
  }

  void smart_pointer_false_positive_free_diagnostics::test_shared_ptr()
  {
    {
      using ptr_t = std::shared_ptr<int>;
      ptr_t p{}, q{std::make_shared<int>(42)};
      check(equality, "Equality of null pointer with itself", p, p);
      check(equality, "Equality of non-null pointer with itself", q, q);
      check(equivalence, "Different pointers pointing to identical values", std::make_shared<int>(42), std::make_shared<int>(42));
    }

    {
      using type = std::tuple<int, only_equivalence_checkable, only_weakly_checkable>;
      check(equivalence,
            reporter{"Different pointers pointing to identical values"},
            std::make_shared<type>(-1, only_equivalence_checkable{2.0}, only_weakly_checkable{42, 1.0}),
            std::make_shared<type>(-1, only_equivalence_checkable{2.0}, only_weakly_checkable{42, 1.0}));
    }
  }

  void smart_pointer_false_positive_free_diagnostics::test_weak_ptr()
  {
    {
      using wptr_t = std::weak_ptr<int>;
      wptr_t wp{};
      check(equality, "Equality of null pointer with itself", wp, wp);

      auto q{std::make_shared<int>(42)}, r{std::make_shared<int>(42)};
      wptr_t wq{q}, wr{r};

      check(equivalence, "Different pointers weakly pointing to identical values", wq, wr);
    }
  }
}
