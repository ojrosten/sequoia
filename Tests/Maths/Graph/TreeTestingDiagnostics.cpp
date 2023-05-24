////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "TreeTestingDiagnostics.hpp"

#include "sequoia/Maths/Graph/DynamicTree.hpp"

namespace sequoia::testing
{
  using namespace maths;

  namespace
  {
    [[nodiscard]]
    std::string to_string(tree_link_direction dir)
    {
      switch(dir)
      {
      case tree_link_direction::forward:
        return "forward";
      case tree_link_direction::backward:
        return "backward";
      case tree_link_direction::symmetric:
        return "symmetric";
      }

      throw std::logic_error{"Unrecognized option for enum class maths::tree_link_direction"};
    }

    [[nodiscard]]
    std::string to_string(directed_flavour dir)
    {
      switch(dir)
      {
      case directed_flavour::directed:
        return "directed";
      case directed_flavour::undirected:
        return "undirected";
      }

      throw std::logic_error{"Unrecognized option for enum class maths::directed_flavour"};
    }

    template<directed_flavour Directedness, tree_link_direction TreeLinkDir>
    [[nodiscard]]
    std::string make_message(std::string_view m)
    {
      auto message{std::string{"Directedness: "}.append(to_string(Directedness))
               .append(" / Tree Link Dir: ").append(to_string(TreeLinkDir))};

      if(!m.empty()) message.append(" - ").append(m);

      return message;
    }
  }

  [[nodiscard]]
  std::filesystem::path tree_false_positive_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void tree_false_positive_test::run_tests()
  {
    test_tree(directed_type{},   forward_tree_type{});
    test_tree(directed_type{},   backward_tree_type{});
    test_tree(directed_type{},   symmetric_tree_type{});
    test_tree(undirected_type{}, symmetric_tree_type{});
  }

  template<maths::directed_flavour Directedness, maths::tree_link_direction TreeLinkDir>
  void tree_false_positive_test::test_tree(directed_flavour_constant<Directedness>, maths::tree_link_direction_constant<TreeLinkDir>)
  {
    using tree_type = tree<directed_flavour::directed, TreeLinkDir, null_weight, int>;
    using initializer = tree_initializer<int>;

    tree_type x{}, y{{1}}, z{{1, {{2}}}}, w{{1, {{2, {{4}, {5}}}, {3}}}};

    auto message{
      [](std::string_view m){ return make_message<Directedness, TreeLinkDir>(m); }
    };

    check(equivalence, report_line(message("Empty vs non-empty")), x, initializer{1, {}});
    check(equivalence, report_line(message("Incorrect weight")), y, initializer{0, {}});
    check(equivalence, report_line(message("Too many children")), z, initializer{1, {}});
    check(equivalence, report_line(message("Incorrect child weight")), z, initializer{1, {{3}}});
    check(equivalence, report_line(message("Too few children")), z, initializer{1, {{2}, {3}}});

    check(equivalence, report_line(message("Too many grand children")), w, initializer{1, {{2, {{4}}}, {3}}});
    check(equivalence, report_line(message("Incorrect grand child weight")), w, initializer{1, {{2, {{3}, {4}}}, {3}}});
    check(equivalence, report_line(message("Too few grand children")), w, initializer{1, {{2, {{4}, {5}, {6}}}, {3}}});

    check(equality, report_line(message("")), x, y);
    check(equality, report_line(message("")), y, z);
  }
}
