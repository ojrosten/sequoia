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
    test_vec_1_orderable<float>();
    test_vec_1_orderable<double>();
    test_vec_1_unorderable<std::complex<float>>();
  }

  template<class T>
  void vec_test::test_vec_1_orderable()
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
            edge_t{vec_1_label::zero,    "T{} * 1",   [](vec_t v) -> vec_t { return T{} *v;           }, std::weak_ordering::less},
            edge_t{vec_1_label::zero,    "1 *= T(0)", [](vec_t v) -> vec_t { return v *= T{};         }, std::weak_ordering::less},
            edge_t{vec_1_label::one,     "-1",        [](vec_t v) -> vec_t { return +v;               }, std::weak_ordering::equivalent},
            edge_t{vec_1_label::two,     "1 + 1",     [](vec_t v) -> vec_t { return v + v;            }, std::weak_ordering::greater},
            edge_t{vec_1_label::two,     "1 += 1",    [](vec_t v) -> vec_t { return v += v;           }, std::weak_ordering::greater},
            edge_t{vec_1_label::two,     "1 * T(2)",  [](vec_t v) -> vec_t { return v * T(2);         }, std::weak_ordering::greater},
            edge_t{vec_1_label::two,     "1 *= T(2)", [](vec_t v) -> vec_t { return v *= T(2);        }, std::weak_ordering::greater},
            edge_t{vec_1_label::two,     "T(2) * 1",  [](vec_t v) -> vec_t { return T(2) * v;         }, std::weak_ordering::greater}
          }, // one
          {
            edge_t{vec_1_label::one, "2 / T(2)",  [](vec_t v) -> vec_t { return v / T(2);        }, std::weak_ordering::less},
            edge_t{vec_1_label::one, "2 /= T(2)", [](vec_t v) -> vec_t { return v /= T(2);       }, std::weak_ordering::less},
            edge_t{vec_1_label::one, "2 - 1",     [](vec_t v) -> vec_t { return v - vec_t{T(1)}; }, std::weak_ordering::less}
          }, // two
        },
        {vec_t{T(-1)}, vec_t{}, vec_t{T(1)}, vec_t{T(2)}}
      };

      auto checker{
          [this](std::string_view description, const vec_t& obtained, const vec_t& prediction, const vec_t& parent, std::weak_ordering ordering) {
            check(equality, description, obtained, prediction);
            if(ordering != std::weak_ordering::equivalent)
              check_semantics(description, prediction, parent, ordering);
          }
      };

      transition_checker<vec_t>::check(report_line(""), g, checker);
  }

  template<class T>
  void vec_test::test_vec_1_unorderable()
  {
    using vec_t = vec<my_vec_space<T, 1>>;
    using angle_graph = transition_checker<vec_t>::transition_graph;
    using edge_t = transition_checker<vec_t>::edge;

    angle_graph g{
      {
        {
          edge_t{vec_1_label::one,     "- -1",  [](vec_t v) -> vec_t { return -v;  }},
          edge_t{vec_1_label::neg_one, "+ -1",  [](vec_t v) -> vec_t { return +v;  }}
        }, // neg_one
        {
          edge_t{vec_1_label::one, "0 + 1",  [](vec_t v) -> vec_t { return v + vec_t{T(1)};  }},
          edge_t{vec_1_label::one, "0 += 1", [](vec_t v) -> vec_t { return v += vec_t{T(1)}; }}
        }, // zero
        {
          edge_t{vec_1_label::neg_one, "-1",        [](vec_t v) -> vec_t { return -v;               }},
          edge_t{vec_1_label::zero,    "1 - 1",     [](vec_t v) -> vec_t { return v - vec_t{T(1)};  }},
          edge_t{vec_1_label::zero,    "1 -= 1",    [](vec_t v) -> vec_t { return v -= vec_t{T(1)}; }},
          edge_t{vec_1_label::zero,    "1 * T{}",   [](vec_t v) -> vec_t { return v * T{};          }},
          edge_t{vec_1_label::zero,    "T{} * 1",   [](vec_t v) -> vec_t { return T{} *v;           }},
          edge_t{vec_1_label::zero,    "1 *= T(0)", [](vec_t v) -> vec_t { return v *= T{};         }},
          edge_t{vec_1_label::two,     "1 + 1",     [](vec_t v) -> vec_t { return v + v;            }},
          edge_t{vec_1_label::two,     "1 += 1",    [](vec_t v) -> vec_t { return v += v;           }},
          edge_t{vec_1_label::two,     "1 * T(2)",  [](vec_t v) -> vec_t { return v * T(2);         }},
          edge_t{vec_1_label::two,     "1 *= T(2)", [](vec_t v) -> vec_t { return v *= T(2);        }},
          edge_t{vec_1_label::two,     "T(2) * 1",  [](vec_t v) -> vec_t { return T(2) * v;         }}
        }, // one
        {
          edge_t{vec_1_label::one, "2 / T(2)",  [](vec_t v) -> vec_t { return v / T(2);        }},
          edge_t{vec_1_label::one, "2 /= T(2)", [](vec_t v) -> vec_t { return v /= T(2);       }},
          edge_t{vec_1_label::one, "2 - 1",     [](vec_t v) -> vec_t { return v - vec_t{T(1)}; }}
        }, // two
      },
      {vec_t{T(-1)}, vec_t{}, vec_t{T(1)}, vec_t{T(2)}}
    };

    auto checker{
        [this](std::string_view description, const vec_t& obtained, const vec_t& prediction, const vec_t& parent, std::size_t host, std::size_t target) {
          check(equality, description, obtained, prediction);
          if(host!= target) check_semantics(description, prediction, parent);
        }
    };

    transition_checker<vec_t>::check(report_line(""), g, checker);
  }
}
