////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "VecTest.hpp"

#include "sequoia/TestFramework/StateTransitionUtilities.hpp"

#include <complex>

namespace sequoia::testing
{
  using namespace maths;

  namespace
  {
    enum vec_1_label{ neg_one, zero, one, two };
  }

  [[nodiscard]]
  std::filesystem::path vec_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void vec_test::run_tests()
  {
    test_vec_1<float>();
    //test_vec_1<std::complex<double>>();
  }

  template<class T>
  void vec_test::test_vec_1()
  {
      using vec_t       = vec<my_vec_space<T, 1>>;
      using angle_graph = transition_checker<vec_t>::transition_graph;
      using edge_t      = transition_checker<vec_t>::edge;

      angle_graph g{
        {
          {
            edge_t{vec_1_label::one,     "- -1",  [](vec_t v) -> vec_t { return -v;  }, std::weak_ordering::greater},
            edge_t{vec_1_label::neg_one, "+ -1",  [](vec_t v) -> vec_t { return +v;  }, std::weak_ordering::equivalent}
          }, // neg_one
          {
            edge_t{vec_1_label::one, "0 + 1",  [](vec_t v) -> vec_t { return v + vec_t{T(1)};  }, std::weak_ordering::greater},
            edge_t{vec_1_label::one, "0 += 1", [](vec_t v) -> vec_t { return v += vec_t{T(1)}; }, std::weak_ordering::greater}
          }, // zero
          {
            edge_t{vec_1_label::neg_one, "-1",        [](vec_t v) -> vec_t { return -v;               }, std::weak_ordering::less},
            edge_t{vec_1_label::zero,    "1 - 1",     [](vec_t v) -> vec_t { return v - vec_t{T(1)};  }, std::weak_ordering::less},
            edge_t{vec_1_label::zero,    "1 -= 1",    [](vec_t v) -> vec_t { return v -= vec_t{T(1)}; }, std::weak_ordering::less},
            edge_t{vec_1_label::zero,    "1 * T{}",   [](vec_t v) -> vec_t { return v * T{};          }, std::weak_ordering::less},
            //edge_t{vec_1_label::zero,    "1 * 0",     [](vec_t v) -> vec_t { return v * 0;           }, std::weak_ordering::less},
            edge_t{vec_1_label::zero,    "T{} * 1",   [](vec_t v) -> vec_t { return T{} *v;           }, std::weak_ordering::less},
            //edge_t{vec_1_label::zero,    "0 * 1",     [](vec_t v) -> vec_t { return 0 * v;          }, std::weak_ordering::less},
            edge_t{vec_1_label::zero,    "1 *= T(0)", [](vec_t v) -> vec_t { return v *= T{};         }, std::weak_ordering::less},
            //edge_t{vec_1_label::zero,    "1 *= 0",    [](vec_t v) -> vec_t { return v *= 0;          }, std::weak_ordering::less},
            edge_t{vec_1_label::one,     "-1",        [](vec_t v) -> vec_t { return +v;               }, std::weak_ordering::equivalent},
            edge_t{vec_1_label::two,     "1 + 1",     [](vec_t v) -> vec_t { return v + v;            }, std::weak_ordering::greater},
            edge_t{vec_1_label::two,     "1 += 1",    [](vec_t v) -> vec_t { return v += v;           }, std::weak_ordering::greater},
            edge_t{vec_1_label::two,     "1 * T(2)",  [](vec_t v) -> vec_t { return v * T(2);         }, std::weak_ordering::greater},
            //edge_t{vec_1_label::two,     "1 * 2",     [](vec_t v) -> vec_t { return v * 2;           }, std::weak_ordering::greater},
            edge_t{vec_1_label::two,     "1 *= T(2)", [](vec_t v) -> vec_t { return v *= T(2);        }, std::weak_ordering::greater},
            //edge_t{vec_1_label::two,     "1 *= 2",    [](vec_t v) -> vec_t { return v *= 2;          }, std::weak_ordering::greater},
            edge_t{vec_1_label::two,     "T(2) * 1",  [](vec_t v) -> vec_t { return T(2) * v;         }, std::weak_ordering::greater},
            //edge_t{vec_1_label::two,     "2 * 1",     [](vec_t v) -> vec_t { return 2 * v;           }, std::weak_ordering::greater}
          }, // one
          {
            edge_t{vec_1_label::one, "2 / T(2)",  [](vec_t v) -> vec_t { return v / T(2);        }, std::weak_ordering::less},
            //edge_t{vec_1_label::one, "2 / 2",     [](vec_t v) -> vec_t { return v / 2;          }, std::weak_ordering::less},
            edge_t{vec_1_label::one, "2 /= T(2)", [](vec_t v) -> vec_t { return v /= T(2);       }, std::weak_ordering::less},
            //edge_t{vec_1_label::one, "2 /= 2",    [](vec_t v) -> vec_t { return v /= 2;         }, std::weak_ordering::less},
            edge_t{vec_1_label::one, "2 - 1",     [](vec_t v) -> vec_t { return v - vec_t{T(1)}; }, std::weak_ordering::less},
          }, // two
        },
        {vec_t{T(-1)}, vec_t{}, vec_t{T(1)}, vec_t{T(2)}}
      };

      auto checker{
          [this](std::string_view description, vec_t obtained, vec_t prediction, vec_t parent, std::weak_ordering ordering) {
            check(equality, description, obtained, prediction);
            if(ordering != std::weak_ordering::equivalent)
              check_semantics(description, prediction, parent, ordering);
          }
      };

      transition_checker<vec_t>::check(report_line(""), g, checker);
  }
}
