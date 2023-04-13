////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "TypeListFreeTest.hpp"
#include "TypeList.hpp"

namespace sequoia::testing
{
  template<class... Ts>
  struct flatten;

  template<class... Ts>
  struct flatten
  {
    using type = type_list<typename flatten<Ts>::type...>;
  };

  template<class... Ts>
  using flatten_t = typename flatten<Ts...>::type;

  template<class T>
  struct flatten<T>
  {
    using type = T;
  };

  template<class... Ts>
  struct flatten<type_list<Ts...>>
  {
    using type = flatten_t<Ts...>;
  };

  template<class T, class U>
  struct flatten<T, U>
  {
    using type = type_list<flatten_t<T>, flatten_t<U>>;
  };

  template<class T, class U, class... Ts>
  struct flatten<T, U, Ts...>
  {
    using type = flatten_t<flatten_t<T, U>, Ts...>;
  };

  template<class... Ts, class... Us>
  struct flatten<type_list<Ts...>, type_list<Us...>>
  {
    using type = type_list<Ts..., Us...>;
  };

  template<class T, class...Ts>
  struct flatten<T, type_list<Ts...>>
  {
    using type = flatten_t<type_list<flatten_t<T>, flatten_t<Ts>...>>;
  };

  template<class T, class...Ts>
  struct flatten<type_list<Ts...>, T>
  {
    using type = type_list<flatten_t<Ts>..., flatten_t<T>>;
  };

  template<class T>
  struct to_tuple;

  template<class... Ts>
  struct to_tuple<type_list<Ts...>>
  {
    using type = std::tuple<Ts...>;
  };

  template<class T>
  using to_tuple_t = typename to_tuple<T>::type;

  struct composite
  {
    std::string name{};
  };

  template<class T>
  struct entity
  {
    to_tuple_t<flatten_t<T>> attributes;
  };


  [[nodiscard]]
  std::filesystem::path type_list_free_test::source_file() const noexcept
  {
    return std::source_location::current().file_name();
  }

  void type_list_free_test::run_tests()
  {
    test_type_list();
    test_flatten();
  }

  void type_list_free_test::test_type_list()
  {
    {
      using typeList = type_list<>;

      check(report_line(""), []() {
          static_assert(std::is_same_v<head_of_t<>, void>);
          static_assert(std::is_same_v<head_of_t<typeList>, void>);
          return true;
        }()
      );

      check(report_line(""), []() {
          static_assert(std::is_same_v<tail_of_t<>, type_list<>>);
          static_assert(std::is_same_v<tail_of_t<typeList>, type_list<>>);
          return true;
        }()
      );
    }

    {
      using typeList = type_list<int>;

      check(report_line(""), []() {
          static_assert(std::is_same_v<head_of_t<int>, int>);
          static_assert(std::is_same_v<head_of_t<typeList>, int>);
          return true;
        }()
      );

      check(report_line(""), []() {
          static_assert(std::is_same_v<tail_of_t<int>, type_list<>>);
          static_assert(std::is_same_v<tail_of_t<typeList>, type_list<>>);
          return true;
        }()
      );
    }

    {
      using typeList = type_list<int, double>;

      check(report_line(""), []() {
          static_assert(std::is_same_v<head_of_t<int, double>, int>);
          static_assert(std::is_same_v<head_of_t<typeList>, int>);
          return true;
        }()
      );

      check(report_line(""), []() {
          static_assert(std::is_same_v<tail_of_t<int, double>, type_list<double>>);
          static_assert(std::is_same_v<tail_of_t<typeList>, type_list<double>>);
          return true;
        }()
      );
    }

    {
      using typeList = type_list<int, double, char>;

      check(report_line(""), []() {
          static_assert(std::is_same_v<head_of_t<int, double, char>, int>);
          static_assert(std::is_same_v<head_of_t<typeList>, int>);
          return true;
        }()
      );

      check(report_line(""), []() {
          static_assert(std::is_same_v<tail_of_t<int, double, char>, type_list<double, char>>);
          static_assert(std::is_same_v<tail_of_t<typeList>, type_list<double, char>>);
          return true;
        }()
      );
    }
  }

  void type_list_free_test::test_flatten()
  {
    static_assert(std::is_same_v<flatten_t<int>, int>);
    static_assert(std::is_same_v<flatten_t<type_list<int>>, int>);
    static_assert(std::is_same_v<flatten_t<type_list<int, char>>, type_list<int, char>>);

    static_assert(std::is_same_v<flatten_t<int, type_list<char, float>>, type_list<int, char, float>>);
    static_assert(std::is_same_v<flatten_t<int, type_list<char, float, double>>, type_list<int, char, float, double>>);
    static_assert(std::is_same_v<flatten_t<type_list<char, float>, int>, type_list<char, float, int>>);
    static_assert(std::is_same_v<flatten_t<type_list<char, float, double>, int>, type_list<char, float, double, int>>);

    static_assert(std::is_same_v<flatten_t<int, double, type_list<char, float>>, type_list<int, double, char, float>>);
    static_assert(std::is_same_v<flatten_t<type_list<char, float>, int, double>, type_list<char, float, int, double>>);


    static_assert(std::is_same_v<flatten_t<type_list<int, type_list<char, float>>>, type_list<int, char, float>>);

    static_assert(std::is_same_v<flatten_t<type_list<int, type_list<char, type_list<float, double>>>>,
      type_list<int, char, float, double>>);

    static_assert(std::is_same_v<flatten_t<type_list<int, type_list<type_list<char, unsigned>, type_list<float, double>>>>,
      type_list<int, char, unsigned, float, double>>);

    static_assert(std::is_same_v<flatten_t<type_list<int, type_list<type_list<char, unsigned>, type_list<float, type_list<double, long>>>>>,
      type_list<int, char, unsigned, float, double, long>>);

    // Could interpret this as:
    //
    //   int
    //    |
    //    x
    //    |
    //    x
    static_assert(std::is_same_v<flatten_t<type_list<type_list<int>>>, int>);

    //       long bool  unsigned
    //         \   /    /
    //     char float int
    //       \   |   /
    //         double
    static_assert(
      std::is_same_v<
        flatten_t<type_list<double,
                            type_list<char,
                                      type_list<float,
                                                type_list<long, bool>>,
                                      type_list<int,
                                                type_list<unsigned>>
                            >
                  >
        >
        ,
        type_list<double, char, float, long, bool, int, unsigned>
      >
      );

    // int double   float
    //  \    /       /
    // composite composite
    //     \        /
    //      composite

    entity<
      type_list<
        composite,
        type_list<
          composite,
          type_list<int, double>,
          composite,
          type_list<float>
        >
      >
    > e{};
  }
}