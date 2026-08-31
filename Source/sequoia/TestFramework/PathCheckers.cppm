////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

module;

#include "sequoia/PlatformSpecific/Macros.hpp"
#include "sequoia/TestFramework/Macros.hpp"

export module sequoia.test_framework:PathCheckers;

import std;

import :Advice;
import :BinaryRelationships;
import :CoreInfrastructure;
import :FailureInfo;
import :FileEditors;
import :FileSystemUtilities;
import :FreeCheckers;
import :Output;
import :ProjectPaths;
import :StringCheckers;
import :TestLogger;
import :TestMode;
export import sequoia.core.container_utilities;
export import sequoia.core.meta;
export import sequoia.core.object;
export import sequoia.file_system;
export import sequoia.platform_specific;
export import sequoia.streaming;
export import sequoia.text_processing;

/** \file
    \brief Specializations of the class template value_tester for `std::filesystem::path`, and the file comparers it dispatches to.

    See ConcreteTypeCheckers.hpp for how nested checks dispatch, and for a single
    header pulling in every specialization at once.
 */

export namespace sequoia::testing
{
  /** \brief The common preamble to a path-comparison failure message. */

  [[nodiscard]]
  std::string path_check_preamble(std::string_view prefix, const std::filesystem::path& path, const std::filesystem::path& prediction);

  /** \brief Function object for comparing files via reading their contents into strings. */

  struct string_based_file_comparer
  {
    template<test_mode Mode>
    void operator()(test_logger<Mode>& logger, const std::filesystem::path& file, const std::filesystem::path& prediction) const
    {
      const auto [reducedWorking, reducedPrediction] {get_reduced_file_content(file, prediction)};

      testing::check(report_failed_read(file),       logger, static_cast<bool>(reducedWorking));
      testing::check(report_failed_read(prediction), logger, static_cast<bool>(reducedPrediction));

      if(reducedWorking && reducedPrediction)
      {
        check(equality, path_check_preamble("Contents of", file, prediction), logger, reducedWorking.value(), reducedPrediction.value());
      }
    }
  };

  template<class T>
  inline constexpr bool is_file_comparer_v{
    std::invocable<T, test_logger<test_mode::standard>&, std::filesystem::path, std::filesystem::path>
  };

  /** \brief A file checker, which accepts a variadic set of file comparison function objects
   */

  template<class DefaultComparer, class... Comparers>
    requires (is_file_comparer_v<DefaultComparer> && (is_file_comparer_v<Comparers> && ...))
  class general_file_checker
  {
  public:
    [[nodiscard]]
    constexpr static std::size_t size() noexcept
    {
      return 1 + sizeof...(Comparers);
    }

    template<class... Extensions>
        requires (sizeof...(Extensions) == size()) && (std::is_constructible_v<std::string, Extensions> && ...)
    general_file_checker(Extensions... extensions)
      : m_Factory{std::move(extensions)...}
    {}

    template<test_mode Mode>
    void check_file(test_logger<Mode>& logger, const std::filesystem::path& file, const std::filesystem::path& prediction) const
    {
      const auto checker{m_Factory.template make_or<DefaultComparer>(file.extension().string())};
      std::visit([&logger, &file, &prediction](auto&& fn){ fn(logger, file, prediction); }, checker);
    }
  private:
    using factory = object::factory<DefaultComparer, Comparers...>;

    factory m_Factory;
  };

  template<class DefaultComparer, class... Comparers>
  using equivalence_with_bespoke_file_checker_t = general_equivalence_check_t<general_file_checker<DefaultComparer, Comparers...>>;

  template<class DefaultComparer, class... Comparers>
  using weak_equivalence_with_bespoke_file_checker_t = general_weak_equivalence_check_t<general_file_checker<DefaultComparer, Comparers...>>;

  /** \brief Checks equivalence of filesystem paths.

      For the overloads of `test` accepting either `equivalence` or `weak_equivalence` as the
      first argument, all file comparisons are performed using string_based_file_comparer.
      Other overloads allow clients to customize the way in which files are compared.

      Files are considered equivalent if they have the same name and the same contents;
      similarly directories.

      Files are considered weakly equivalent if they have the same contents;
      similarly directories. The names of both are ignored.
   */

  template<>
  struct value_tester<std::filesystem::path>
  {
    template<class DefaultComparer, class... Comparers, test_mode Mode>
    static void test(equivalence_with_bespoke_file_checker_t<DefaultComparer, Comparers...> checker,
                     test_logger<Mode>& logger,
                     const std::filesystem::path& path,
                     const std::filesystem::path& prediction)
    {
      namespace fs = std::filesystem;

      auto pred{
        [&logger](const fs::path& pathFinalToken, const fs::path& predictionFinalToken)
        {
           return check(equality, "Final path token", logger, pathFinalToken, predictionFinalToken);
        }
      };

      check_path(logger, checker.customizer, path, prediction, pred);
    }

    template<test_mode Mode>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const std::filesystem::path& path, const std::filesystem::path& prediction)
    {
      test(basic_path_equivalence, logger, path, prediction);
    }

    template<class DefaultComparer, class... Comparers, test_mode Mode>
    static void test(weak_equivalence_with_bespoke_file_checker_t<DefaultComparer, Comparers...> checker,
                     test_logger<Mode>& logger,
                     const std::filesystem::path& path,
                     const std::filesystem::path& prediction)
    {
      namespace fs = std::filesystem;
      check_path(logger, checker.customizer, path, prediction, [](const fs::path&, const fs::path&) { return true; });
    }

    template<test_mode Mode>
    static void test(weak_equivalence_check_t, test_logger<Mode>& logger, const std::filesystem::path& path, const std::filesystem::path& prediction)
    {
      test(basic_path_weak_equivalence, logger, path, prediction);
    }
  private:
    constexpr static std::array<std::string_view, 2>
      excluded_files{".DS_Store", ".keep"};

    constexpr static std::array<std::string_view, 1>
      excluded_extensions{seqpat};

    using basic_file_checker_t = general_file_checker<string_based_file_comparer>;

    static const basic_file_checker_t basic_file_checker;

    static const general_equivalence_check_t<basic_file_checker_t>      basic_path_equivalence;
    static const general_weak_equivalence_check_t<basic_file_checker_t> basic_path_weak_equivalence;

    template<test_mode Mode, class Customization, invocable_r<bool, std::filesystem::path, std::filesystem::path> FinalTokenComparison>
    static void check_path(test_logger<Mode>& logger, const Customization& custom, const std::filesystem::path& path, const std::filesystem::path& prediction, FinalTokenComparison compare)
    {
      namespace fs = std::filesystem;

      const auto pathType{fs::status(path).type()};
      const auto predictionType{fs::status(prediction).type()};

      if(check(equality, path_check_preamble("Path type", path, prediction), logger, pathType, predictionType))
      {
        if(!path.empty())
        {
          const auto pathFinalToken{back(path)};
          const auto predictionFinalToken{back(prediction)};
          if(compare(pathFinalToken, predictionFinalToken))
          {
            switch(pathType)
            {
            case fs::file_type::regular:
              check_file(logger, custom, path, prediction);
              break;
            case fs::file_type::directory:
              check_directory(logger, custom, path, prediction, compare);
              break;
            default:
              throw std::logic_error{std::string{"Detailed equivalance check for paths of type '"}
                .append(serializer<fs::file_type>::make(pathType)).append("' not currently implemented")};
            }
          }
        }
      }
    }

    template<test_mode Mode, class Customization, invocable_r<bool, std::filesystem::path, std::filesystem::path> FinalTokenComparison>
    static void check_directory(test_logger<Mode>& logger, const Customization& custom, const std::filesystem::path& dir, const std::filesystem::path& prediction, FinalTokenComparison compare)
    {
      namespace fs = std::filesystem;

      auto generator{
        [](const fs::path& dirPath) {
          std::vector<fs::path> paths{};
          for(const auto& p : fs::directory_iterator(dirPath))
          {
            if(    std::ranges::find(excluded_files,      p.path().filename())  == excluded_files.end()
                && std::ranges::find(excluded_extensions, p.path().extension()) == excluded_extensions.end())
            {
               paths.push_back(p);
            }
          }

          std::ranges::sort(paths);

          return paths;
        }
      };

      const std::vector<fs::path> paths{generator(dir)}, predictedPaths{generator(prediction)};

      check(
	equality,
	std::string{"Number of directory entries for "}.append(dir.generic_string()),
        logger,
        paths.size(),
        predictedPaths.size()
      );

      const auto iters{std::ranges::mismatch(paths, predictedPaths,
          [&dir,&prediction](const fs::path& lhs, const fs::path& rhs) {
            return fs::relative(lhs, dir) == fs::relative(rhs, prediction);
          })};
      if((iters.in1 != paths.end()) && (iters.in2 != predictedPaths.end()))
      {
        check(equality, "First directory entry mismatch", logger, *iters.in1, *iters.in2);
      }
      else if(iters.in1 != paths.end())
      {
        check(equality, "First directory entry mismatch", logger, *iters.in1, fs::path{});
      }
      else if(iters.in2 != predictedPaths.end())
      {
        check(equality, "First directory entry mismatch", logger, fs::path{}, *iters.in2);
      }
      else
      {
        for(std::size_t i{}; i < paths.size(); ++i)
        {
          check_path(logger, custom, paths[i], predictedPaths[i], compare);
        }
      }
    }

    template<test_mode Mode, class Customization>
    static void check_file(test_logger<Mode>& logger, const Customization& custom, const std::filesystem::path& file, const std::filesystem::path& prediction)
    {
      custom.check_file(logger, file, prediction);
    }
  };

}
