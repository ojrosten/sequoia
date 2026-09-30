////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestRunnerTest.hpp"
#include "TestRunnerDiagnosticsUtilities.hpp"
#include "Parsing/CommandLineArgumentsTestingUtilities.hpp"
#include "Utilities/TestUtilities.hpp"
#include "TestFramework/BuildArtefactsTestingUtilities.hpp"

#include "sequoia/PlatformSpecific/Preprocessor.hpp"
#include "sequoia/Runtime/ShellCommands.hpp"

#include <fstream>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    struct foo
    {
      int x{};
    };

    /** These doubles have no source file of their own, so they mint one. Keying it on the test's
        class rather than on its display name keeps the two from drifting apart, and spares the
        space-to-underscore substitution a class name never needs.
     */

    template<concrete_test T>
    [[nodiscard]]
    fs::path make_fake_file_path(std::string_view group = "") {
      return fs::path{std::source_location::current().file_name()}.parent_path().parent_path() / group / (std::string{test_name<T>()} + ".cpp");
    }
  }

  template<>
  struct value_tester<foo>
  {
    template<test_mode Mode>
    static void test(equality_check_t, test_logger<Mode>&, const foo&, const foo&)
    {
      throw std::runtime_error{"This is bad"};
    }
  };
  
  namespace
  {
    class foo_test final : public regular_test
    {
    public:
      using regular_test::regular_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return make_fake_file_path<foo_test>();
      }

      void run_tests()
      {
        check(equality, reporter{"Throw during check"}, foo{}, foo{});
      }
    };

    class passing_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return make_fake_file_path<passing_test>();
      }

      void run_tests()
      {
        check(equality, reporter{"Integer equality"}, 42, 42);
      }
    };

    class failing_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return make_fake_file_path<failing_test>("Failing");
      }

      void run_tests()
      {
        check(equality, {"Standard Failure"}, 43, 42);
      }
    };

    class throwing_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return make_fake_file_path<throwing_test>();
      }

      void run_tests()
      {
        check_exception_thrown<std::runtime_error>("Exception", [](){ throw std::runtime_error{"Oops"}; });
      }
    };

    class platform_specific_throwing_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return make_fake_file_path<platform_specific_throwing_test>();
      }

      [[nodiscard]]
      static std::string output_discriminator(const cmake_cache&) { return "Platypus"; }

      [[nodiscard]]
      static std::string summary_discriminator(const cmake_cache&) { return "Release"; }

      void run_tests()
      {
        check_exception_thrown<std::runtime_error>("Another Exception", [](){ throw std::runtime_error{"Oh Dear"}; });
      }
    };

    class failing_fp_test final : public free_false_negative_test
    {
    public:
      using free_false_negative_test::free_false_negative_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return make_fake_file_path<failing_fp_test>("Failing");
      }

      void run_tests()
      {
        check(equality, {"False positive failure"}, 42, 42);
      }
    };

    class failing_fn_test final : public free_false_positive_test
    {
    public:
      using free_false_positive_test::free_false_positive_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return make_fake_file_path<failing_fn_test>("Failing");
      }

      void run_tests()
      {
        check(equality, {"False negative failure"}, 43, 42);
      }
    };

    struct flipper
    {
      flipper() { x = !x; }

      inline static bool x{};
    };

    class flipper_free_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return make_fake_file_path<flipper_free_test>();
      }

      void run_tests()
      {
        check(equality, {"Flipper"}, flipper{}.x, true);
      }
    };

    template<std::size_t N>
    struct periodic
    {
      periodic() { x = (x+1) % N;}
      
      inline static int x{};
    };

    class periodic_free_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return make_fake_file_path<periodic_free_test>();
      }

      void run_tests()
      {
        check(equality, {"Period 4"}, periodic<4>{}.x, 1);
      }
    };


    class multi_periodic_free_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return make_fake_file_path<multi_periodic_free_test>();
      }

      void run_tests()
      {
        check(equality, {"Period 2: Pass/Fail/Pass"}, periodic<2>{}.x, 1);
        check({"Period 3: Pass/Pass/Fail"}, periodic<3>{}.x > 0);
      }
    };

    class failing_plus_instabilities_free_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return make_fake_file_path<failing_plus_instabilities_free_test>();
      }

      void run_tests()
      {
        check({"Always fails"}, false);

        check(equality, {"Flipper"}, flipper{}.x, true);
      }
    };

    class consistently_failing_free_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return make_fake_file_path<consistently_failing_free_test>();
      }

      void run_tests()
      {
        check({"Always fails"}, false);
      }
    };

    /** Two classes rather than one template: the pair exists to be registered together, and a
        test's name - and so its output path - now comes from its class.
     */

    class consistently_passing_free_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return make_fake_file_path<consistently_passing_free_test>();
      }

      void run_tests()
      {
        check({"Always passes"}, true);
      }
    };

    class another_consistently_passing_free_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return make_fake_file_path<another_consistently_passing_free_test>();
      }

      void run_tests()
      {
        check({"Always passes"}, true);
      }
    };

    class fake_performance_test final : public performance_test
    {
    public:
      using performance_test::performance_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return make_fake_file_path<fake_performance_test>();
      }

      void run_tests()
      {
        check(equality, "Performance", 42, 42);
      }
    };

    class critical_free_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return make_fake_file_path<critical_free_test>();
      }

      void run_tests()
      {
        if(flipper{}.x)
          throw std::runtime_error{"Error"};
      }
    };

    /// The next two put a suite and a test which are siblings under one name: `namesake_test` names the
    /// test, and the directory holding `under_namesake_test` beside it; the test's source is named otherwise,
    /// lest the materials prefixes nest. The test sorts first, so its node exists when the suite is wanted.
    class namesake_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return make_fake_file_path<namesake_test>("Namesakes").replace_filename("NamesakeTest.cpp");
      }

      void run_tests()
      {
        check(equality, reporter{"Namesake"}, 42, 42);
      }
    };

    class under_namesake_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return make_fake_file_path<under_namesake_test>("Namesakes/namesake_test");
      }

      void run_tests()
      {
        check(equality, reporter{"Under the namesake"}, 42, 42);
      }
    };

    namespace another_namespace
    {
      /// Holds a `foo_test` of its own. `test_name` strips the qualification from both, and a
      /// test's name is the leaf of its materials path, so the runner must admit only one.
      class foo_test final : public free_test
      {
      public:
        using free_test::free_test;

        [[nodiscard]]
        static fs::path source_file()
        {
          return make_fake_file_path<foo_test>();
        }

        void run_tests() {}
      };
    }

    /** Makes `test` a candidate for update, by failing a check.

        The function writes a `Kept.txt` into the working materials. The test's predictions hold a
        `Kept.txt` with other contents, and an `Obsolete.txt`, which the function does not write. So an
        update overwrites `Kept.txt` and deletes `Obsolete.txt`.

        The source files of the update fakes below are relative, so that their materials resolve
        inside the fake project.
     */

    void make_update_candidate(free_test& test)
    {
      write_to_file(test.working_materials() /= "Kept.txt", "Obtained\n", std::ios_base::out);
      test.check("Predictions are stale", false);
    }

    class stale_predictions_free_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return "Tests/Updating/StalePredictionsFreeTest.cpp";
      }

      void run_tests()
      {
        make_update_candidate(*this);
      }
    };

    class throwing_stale_predictions_free_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return "Tests/Updating/ThrowingStalePredictionsFreeTest.cpp";
      }

      void run_tests()
      {
        make_update_candidate(*this);
        throw std::runtime_error{"Thrown after a failed check"};
      }
    };

    /** Writes a regular file `B` where its predictions hold a directory. The update does not handle
        a change of type, and throws on copying the file over the directory. `A` sorts before `B`, so
        the update has by then deleted `A/old.txt`, which the predictions hold and the test does not write.
     */
    class type_swapped_predictions_free_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return "Tests/Updating/TypeSwappedPredictionsFreeTest.cpp";
      }

      void run_tests()
      {
        fs::create_directory(working_materials() /= "A");
        write_to_file(working_materials() /= "B", "", std::ios_base::out);
        check("Predictions are stale", false);
      }
    };

    /** A variant of `stale_predictions_free_test`, with materials under two discriminators: `Platypus`
        and `Echidna`. The test's materials discriminator is `Platypus`.

        Under each discriminator, the working copy and the auxiliary materials hold a `Discriminator.txt`
        which holds the discriminator. The predictions hold the same file, so the update leaves the file
        alone.
     */
    class variant_free_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static std::filesystem::path source_file()
      {
        return "Tests/Updating/VariantFreeTest.cpp";
      }

      [[nodiscard]]
      static std::string materials_discriminator(const cmake_cache&) { return "Platypus"; }

      void run_tests()
      {
        check(equality,
              "Working copy of the declared discriminator",
              read_to_string(working_materials() /= "Discriminator.txt", std::ios_base::in).value_or(""),
              std::string{"Platypus\n"});

        check(equality,
              "Auxiliary materials of the declared discriminator",
              read_to_string(auxiliary_materials() /= "Discriminator.txt", std::ios_base::in).value_or(""),
              std::string{"Platypus\n"});

        make_update_candidate(*this);
      }
    };

    /** Each fake below declares the discriminator hooks in one shape, for the probes to tell the
        shapes apart. Each fake declares all three hooks, which differ only in their names.
     */
    class static_hooks_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static std::filesystem::path source_file() { return make_fake_file_path<static_hooks_test>(); }

      [[nodiscard]]
      static std::string output_discriminator(const cmake_cache&) { return "Platypus"; }

      [[nodiscard]]
      static std::string summary_discriminator(const cmake_cache&) { return "Platypus"; }

      [[nodiscard]]
      static std::string materials_discriminator(const cmake_cache&) { return "Platypus"; }

      void run_tests() {}
    };

    class const_member_hooks_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static std::filesystem::path source_file() { return make_fake_file_path<const_member_hooks_test>(); }

      [[nodiscard]]
      std::string output_discriminator(const cmake_cache&) const { return "Platypus"; }

      [[nodiscard]]
      std::string summary_discriminator(const cmake_cache&) const { return "Platypus"; }

      [[nodiscard]]
      std::string materials_discriminator(const cmake_cache&) const { return "Platypus"; }

      void run_tests() {}
    };

    class mutable_member_hooks_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static std::filesystem::path source_file() { return make_fake_file_path<mutable_member_hooks_test>(); }

      [[nodiscard]]
      std::string output_discriminator(const cmake_cache&) { return "Platypus"; }

      [[nodiscard]]
      std::string summary_discriminator(const cmake_cache&) { return "Platypus"; }

      [[nodiscard]]
      std::string materials_discriminator(const cmake_cache&) { return "Platypus"; }

      void run_tests() {}
    };

    class nullary_hooks_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static std::filesystem::path source_file() { return make_fake_file_path<nullary_hooks_test>(); }

      [[nodiscard]]
      static std::string output_discriminator() { return "Platypus"; }

      [[nodiscard]]
      static std::string summary_discriminator() { return "Platypus"; }

      [[nodiscard]]
      static std::string materials_discriminator() { return "Platypus"; }

      void run_tests() {}
    };

    class overloaded_member_hooks_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static std::filesystem::path source_file() { return make_fake_file_path<overloaded_member_hooks_test>(); }

      [[nodiscard]]
      std::string output_discriminator(const cmake_cache&) const { return "Platypus"; }

      [[nodiscard]]
      std::string output_discriminator(int) const { return "Platypus"; }

      [[nodiscard]]
      std::string summary_discriminator(const cmake_cache&) const { return "Platypus"; }

      [[nodiscard]]
      std::string summary_discriminator(int) const { return "Platypus"; }

      [[nodiscard]]
      std::string materials_discriminator(const cmake_cache&) const { return "Platypus"; }

      [[nodiscard]]
      std::string materials_discriminator(int) const { return "Platypus"; }

      void run_tests() {}
    };

    class overloaded_nullary_hooks_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static std::filesystem::path source_file() { return make_fake_file_path<overloaded_nullary_hooks_test>(); }

      [[nodiscard]]
      static std::string output_discriminator() { return "Platypus"; }

      [[nodiscard]]
      static std::string output_discriminator(int) { return "Platypus"; }

      [[nodiscard]]
      static std::string summary_discriminator() { return "Platypus"; }

      [[nodiscard]]
      static std::string summary_discriminator(int) { return "Platypus"; }

      [[nodiscard]]
      static std::string materials_discriminator() { return "Platypus"; }

      [[nodiscard]]
      static std::string materials_discriminator(int) { return "Platypus"; }

      void run_tests() {}
    };

    class view_valued_hooks_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static std::filesystem::path source_file() { return make_fake_file_path<view_valued_hooks_test>(); }

      [[nodiscard]]
      static std::string_view output_discriminator(const cmake_cache&) { return "Platypus"; }

      [[nodiscard]]
      static std::string_view summary_discriminator(const cmake_cache&) { return "Platypus"; }

      [[nodiscard]]
      static std::string_view materials_discriminator(const cmake_cache&) { return "Platypus"; }

      void run_tests() {}
    };

    /** \brief A test whose summary discriminator gives it the summary file of `summary_collider_test_twin`, but
        for case
     */
    class summary_collider_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static std::filesystem::path source_file()
      {
        return make_fake_file_path<summary_collider_test>();
      }

      [[nodiscard]]
      static std::string summary_discriminator(const cmake_cache&) { return "Twin"; }

      void run_tests() {}
    };

    class summary_collider_test_twin final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static std::filesystem::path source_file()
      {
        return make_fake_file_path<summary_collider_test_twin>();
      }

      void run_tests() {}
    };

    [[nodiscard]]
    test_runner make_fake_runner(commandline_arguments& args, std::stringstream& outputStream)
    {
      return test_runner{args.size(),
                         args.get(),
                         "Oliver J. Rosten",
                         "  ",
                         {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{"TestShared/SharedIncludes.hpp"}},
                         outputStream};
    }

    test_runner make_failing_suite(commandline_arguments args, std::stringstream& outputStream)
    {
      auto runner{make_fake_runner(args, outputStream)};

      runner.register_test<failing_test>();
      runner.register_test<failing_fp_test>();
      runner.register_test<failing_fn_test>();

      return runner;
    }

    /// The materials prefixes of the next two nest: `inner_free_test`'s source lies in the directory whose
    /// path is that of `outer_free_test`'s source less the extension.
    class outer_free_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return make_fake_file_path<outer_free_test>("Nesting");
      }

      void run_tests() {}
    };

    class inner_free_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return make_fake_file_path<inner_free_test>("Nesting/outer_free_test");
      }

      void run_tests() {}
    };

    /// As `inner_free_test`, but beneath a directory differing from `outer_free_test`'s materials prefix only in case.
    class differently_cased_inner_free_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return make_fake_file_path<differently_cased_inner_free_test>("Nesting/OUTER_FREE_TEST");
      }

      void run_tests() {}
    };

    /// Beneath a directory whose name begins with `outer_free_test`'s materials prefix but is not it.
    class adjacent_free_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return make_fake_file_path<adjacent_free_test>("Nesting/outer_free_test_adjacent");
      }

      void run_tests() {}
    };

    /// Shares `outer_free_test`'s source.
    class cohabiting_free_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return outer_free_test::source_file();
      }

      void run_tests() {}
    };

    /// As `inner_free_test`, but two directories beneath `outer_free_test`'s materials prefix.
    class deeply_inner_free_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return make_fake_file_path<deeply_inner_free_test>("Nesting/outer_free_test/Deeper");
      }

      void run_tests() {}
    };

    /// Named as `foo_test` but for case, which the filesystems of macOS and Windows ignore.
    class Foo_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return make_fake_file_path<Foo_test>("Cased");
      }

      void run_tests() {}
    };

    /// Beneath a directory whose name has non-ASCII bytes, spelt as escapes so every compiler reads them alike.
    class non_ascii_free_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return make_fake_file_path<non_ascii_free_test>("Nesting/Caf\xC3\xA9");
      }

      void run_tests() {}
    };

    /// Named with a non-ASCII letter, spelt as a universal character name so every compiler reads it alike.
    class caf\u00E9_free_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return make_fake_file_path<caf\u00E9_free_test>("NonAscii").replace_filename("CafeFreeTest.cpp");
      }

      void run_tests() {}
    };

    class sourceless_free_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return {};
      }

      void run_tests() {}
    };

    /// The time a record or the run's stamp names after `started`; the times sort as their text does
    [[nodiscard]]
    std::string start_named_by(const fs::path& file)
    {
      std::ifstream stream{file};
      std::string word{}, start{};
      stream >> word >> start;

      return start;
    }

    /// The label of each line of a test's execution record: the text before the line's last space
    [[nodiscard]]
    std::vector<std::string> execution_record_labels(const fs::path& record)
    {
      std::vector<std::string> labels{};
      std::ifstream file{record};
      for(std::string line{}; std::getline(file, line);)
      {
        labels.push_back(line.substr(0, line.rfind(' ')));
      }

      return labels;
    }

    /// The value of the line of a test's execution record labelled `label`, or nothing if there is no such line
    [[nodiscard]]
    std::string execution_record_value(const fs::path& record, std::string_view label)
    {
      std::ifstream file{record};
      for(std::string line{}; std::getline(file, line);)
      {
        if(line.starts_with(label) && (line.size() > label.size()) && (line[label.size()] == ' '))
          return line.substr(label.size() + 1);
      }

      return "";
    }

    /// Checks its own execution record while it executes
    class record_reading_free_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static std::filesystem::path source_file()
      {
        return make_fake_file_path<record_reading_free_test>();
      }

      void run_tests()
      {
        const test_execution_record_path record{source_file(), name(), get_project_paths()};
        check(equality,
              "While a test executes, its record names its start and no execution duration",
              execution_record_labels(record.file_path()),
              std::vector<std::string>{"started"});

        check("While a test executes, the run's stamp already exists",
              fs::exists(get_project_paths().execution_records().stamp()));
      }
    };

    /// An exception escapes the body of this test, rather than being caught by a check
    class escaping_exception_free_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static std::filesystem::path source_file()
      {
        return make_fake_file_path<escaping_exception_free_test>();
      }

      void run_tests()
      {
        throw std::runtime_error{"Escapes the test body"};
      }
    };
  }
  
  [[nodiscard]]
  fs::path test_runner_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void test_runner_test::run_tests()
  {
    test_discriminator_hooks();
    test_exceptions();
    test_critical_errors();
    test_basic_output();
    test_help_output();
    test_verbose_output();
    test_serial_verbose_output();
    test_throwing_tests();
    test_execution_records();
    test_filtered_suites();
    test_suites_not_found();
    test_prune_basic_output();
    test_prune_with_changed_toolchain();
    test_prune_selects_a_test_this_executable_lacks();
    test_post_run_failure();
    test_materials_update();
    test_no_materials_update_after_critical_failure();
    test_partial_materials_update();
    test_discriminated_materials_update();
    test_materials_preparation_failure();
    test_versioned_output_failure();
    test_nested_suite();
    test_nested_suite_verbose();
    test_suite_named_as_a_sibling_test();
    test_excluded_performance_tests();
    test_excluded_tests();
    test_excluded_tests_are_rerun();
    test_dump_comparison();
    test_thread_pool();
    test_instability_analysis();
    test_instability_analysis_in_sandboxes_from_a_path_with_a_space();
    test_exit_statuses();
  }

  void test_runner_test::test_discriminator_hooks()
  {
    test_discriminator_probe<output_discriminator_probe>();
    test_discriminator_probe<summary_discriminator_probe>();
    test_discriminator_probe<materials_discriminator_probe>();
  }

  /** Checks the traits for the hook that `Probe` probes for:
      -# A static, string-valued hook taking `const cmake_cache&` is declared and conforming;
      -# A test without the hook is neither;
      -# Each of these other shapes of hook is declared but not conforming: a const member, a non-const
         member, a static hook taking no arguments, and a view-valued hook;
      -# So is an overloaded hook, either a const member with an overload taking `const cmake_cache&`,
         or static with an overload taking no arguments.
   */
  template<template<class> class Probe>
  void test_runner_test::test_discriminator_probe()
  {
    STATIC_CHECK(Probe<static_hooks_test>::declared_v);
    STATIC_CHECK(Probe<static_hooks_test>::conforming_v);

    STATIC_CHECK(!Probe<throwing_test>::declared_v);
    STATIC_CHECK(!Probe<throwing_test>::conforming_v);

    STATIC_CHECK(Probe<const_member_hooks_test>::declared_v);
    STATIC_CHECK(!Probe<const_member_hooks_test>::conforming_v);

    STATIC_CHECK(Probe<mutable_member_hooks_test>::declared_v);
    STATIC_CHECK(!Probe<mutable_member_hooks_test>::conforming_v);

    STATIC_CHECK(Probe<nullary_hooks_test>::declared_v);
    STATIC_CHECK(!Probe<nullary_hooks_test>::conforming_v);

    STATIC_CHECK(Probe<view_valued_hooks_test>::declared_v);
    STATIC_CHECK(!Probe<view_valued_hooks_test>::conforming_v);

    STATIC_CHECK(Probe<overloaded_member_hooks_test>::declared_v);
    STATIC_CHECK(!Probe<overloaded_member_hooks_test>::conforming_v);

    STATIC_CHECK(Probe<overloaded_nullary_hooks_test>::declared_v);
    STATIC_CHECK(!Probe<overloaded_nullary_hooks_test>::conforming_v);
  }

  [[nodiscard]]
  fs::path test_runner_test::fake_project() const
  {
    return auxiliary_materials() /= "FakeProject";
  }

  [[nodiscard]]
  fs::path test_runner_test::minimal_fake_path() const
  {
    return fake_project().append("build/CMade/FakeExe.txt");
  }

  [[nodiscard]]
  std::string test_runner_test::zeroth_arg() const
  {
    return (minimal_fake_path()).generic_string();
  }

  void test_runner_test::write(std::string_view dirName, std::stringstream& output) const
  {
    const auto outputDir{working_materials() /= dirName};
    fs::create_directory(outputDir);

    if(const auto filePath{outputDir / "io.txt"}; std::ofstream file{filePath})
    {
      file << output.str();
    }

    output.str("");
  }

  void test_runner_test::check_output(reporter description, std::string_view dirName, std::stringstream& output)
  {
    write(dirName, output);
    check(equivalence, description, working_materials() /= dirName, predictive_materials() /= dirName);
  }

  void test_runner_test::test_exceptions()
  {
    check_exception_thrown<std::runtime_error>(
      reporter{"Test Main has empty path"},
      [this]() {
        std::stringstream outputStream{};
        commandline_arguments args{{zeroth_arg()}};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", "  ",  {.main_cpp{""}, .common_includes{"TestShared/SharedIncludes.hpp"}}, outputStream};
      });

    check_exception_thrown<std::runtime_error>(
      reporter{"Test Main does not exist"},
      [this]() {
        std::stringstream outputStream{};
        commandline_arguments args{{zeroth_arg()}};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", "  ",  {.main_cpp{"FooMain.cpp"}, .common_includes{"TestShared/SharedIncludes.hpp"}}, outputStream};
      });

    check_exception_thrown<std::runtime_error>(
      reporter{"Include Target has empty path"},
      [this]() {
        std::stringstream outputStream{};
        commandline_arguments args{{zeroth_arg()}};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", "  ",  {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{""}}, outputStream};
      });

    check_exception_thrown<std::runtime_error>(
      reporter{"Include Target does not exist"},
      [this]() {
        std::stringstream outputStream{};
        commandline_arguments args{{zeroth_arg()}};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", "  ",  {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{"FooPath.hpp"}}, outputStream};
      });

    check_exception_thrown<std::runtime_error>(
      reporter{"Project root is empty"},
      []() {
        std::stringstream outputStream{};
        commandline_arguments args{{""}};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", "  ",  {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{"TestShared/SharedIncludes.hpp"}}, outputStream};
      });

    check_exception_thrown<std::runtime_error>(
      reporter{"Project root does not exist"},
      [this]() {
        std::stringstream outputStream{};
        commandline_arguments args{{(fake_project() / "FooRepo").generic_string()}};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", "  ",  {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{"TestShared/SharedIncludes.hpp"}}, outputStream};
      },
      [](const project_paths& projPaths, std::string message){
        constexpr auto npos{std::string::npos};
        if(const auto pos{message.find(projPaths.project_root().generic_string())}; pos < npos)
        {
          const auto start{pos + projPaths.project_root().generic_string().size()};
          if(const auto end{message.find_first_of("\"]", start)}; end < npos)
            message = "canonical - file not found: " + message.substr(start, end - start);
        }

        return message;
      });

    check_exception_thrown<std::runtime_error>(
      reporter{"Project root not findable"},
      [this]() {
        const auto zerothArg{fake_project().append("TestShared").generic_string()};
        std::stringstream outputStream{};
        commandline_arguments args{{zerothArg}};
        test_runner tr{args.size(), args.get(), "Oliver J. Rosten", "  ",  {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{"TestShared/SharedIncludes.hpp"}}, outputStream};
      });

    check_exception_thrown<std::logic_error>(
      reporter{"The same test registered twice"},
      [this](){
        commandline_arguments args{{zeroth_arg()}};
        std::stringstream outputStream{};
  
        auto runner{make_fake_runner(args, outputStream)};

        runner.register_test<foo_test>();
        runner.register_test<foo_test>();
      });

    check_exception_thrown<std::logic_error>(
      reporter{"Two tests whose unqualified names coincide"},
      [this](){
        commandline_arguments args{{zeroth_arg()}};
        std::stringstream outputStream{};

        auto runner{make_fake_runner(args, outputStream)};

        runner.register_test<foo_test>();
        runner.register_test<another_namespace::foo_test>();
      });

    check_exception_thrown<std::logic_error>(
      reporter{"A source in the directory sharing another's materials prefix"},
      [this](){
        commandline_arguments args{{zeroth_arg()}};
        std::stringstream outputStream{};

        test_runner runner{args.size(),
                           args.get(),
                           "Oliver J. Rosten",
                           "  ",
                           {.main_cpp{"TestSandbox/TestSandbox.cpp"},
                            .common_includes{"TestShared/SharedIncludes.hpp"}},
                           outputStream};

        runner.register_test<outer_free_test>();
        runner.register_test<inner_free_test>();
      });

    check_exception_thrown<std::logic_error>(
      reporter{"A source whose materials prefix is a directory holding another's source"},
      [this](){
        commandline_arguments args{{zeroth_arg()}};
        std::stringstream outputStream{};

        test_runner runner{args.size(),
                           args.get(),
                           "Oliver J. Rosten",
                           "  ",
                           {.main_cpp{"TestSandbox/TestSandbox.cpp"},
                            .common_includes{"TestShared/SharedIncludes.hpp"}},
                           outputStream};

        runner.register_test<inner_free_test>();
        runner.register_test<outer_free_test>();
      });

    check_exception_thrown<std::logic_error>(
      reporter{"A source in a directory sharing another's materials prefix but for case"},
      [this](){
        commandline_arguments args{{zeroth_arg()}};
        std::stringstream outputStream{};

        test_runner runner{args.size(),
                           args.get(),
                           "Oliver J. Rosten",
                           "  ",
                           {.main_cpp{"TestSandbox/TestSandbox.cpp"},
                            .common_includes{"TestShared/SharedIncludes.hpp"}},
                           outputStream};

        runner.register_test<outer_free_test>();
        runner.register_test<differently_cased_inner_free_test>();
      });

    check_exception_thrown<std::logic_error>(
      reporter{"Two tests whose names differ only in case"},
      [this](){
        commandline_arguments args{{zeroth_arg()}};
        std::stringstream outputStream{};

        test_runner runner{args.size(),
                           args.get(),
                           "Oliver J. Rosten",
                           "  ",
                           {.main_cpp{"TestSandbox/TestSandbox.cpp"},
                            .common_includes{"TestShared/SharedIncludes.hpp"}},
                           outputStream};

        runner.register_test<foo_test>();
        runner.register_test<Foo_test>();
      });

    check_exception_thrown<std::logic_error>(
      reporter{"A source two directories beneath another's materials prefix"},
      [this](){
        commandline_arguments args{{zeroth_arg()}};
        std::stringstream outputStream{};

        test_runner runner{args.size(),
                           args.get(),
                           "Oliver J. Rosten",
                           "  ",
                           {.main_cpp{"TestSandbox/TestSandbox.cpp"},
                            .common_includes{"TestShared/SharedIncludes.hpp"}},
                           outputStream};

        runner.register_test<outer_free_test>();
        runner.register_test<deeply_inner_free_test>();
      });

    check_exception_thrown<std::logic_error>(
      reporter{"A source whose materials prefix, but for case, is a directory holding another's source"},
      [this](){
        commandline_arguments args{{zeroth_arg()}};
        std::stringstream outputStream{};

        test_runner runner{args.size(),
                           args.get(),
                           "Oliver J. Rosten",
                           "  ",
                           {.main_cpp{"TestSandbox/TestSandbox.cpp"},
                            .common_includes{"TestShared/SharedIncludes.hpp"}},
                           outputStream};

        runner.register_test<differently_cased_inner_free_test>();
        runner.register_test<outer_free_test>();
      });

    check_exception_thrown<std::logic_error>(
      reporter{"Sources beside a materials prefix and sharing it are admitted; one beneath it then is not"},
      [this](){
        commandline_arguments args{{zeroth_arg()}};
        std::stringstream outputStream{};

        test_runner runner{args.size(),
                           args.get(),
                           "Oliver J. Rosten",
                           "  ",
                           {.main_cpp{"TestSandbox/TestSandbox.cpp"},
                            .common_includes{"TestShared/SharedIncludes.hpp"}},
                           outputStream};

        runner.register_test<outer_free_test>();
        runner.register_test<adjacent_free_test>();
        runner.register_test<cohabiting_free_test>();
        runner.register_test<inner_free_test>();
      });

    check_exception_thrown<std::logic_error>(
      reporter{"A source whose materials prefix contains a non-ASCII byte"},
      [this](){
        commandline_arguments args{{zeroth_arg()}};
        std::stringstream outputStream{};

        test_runner runner{args.size(),
                           args.get(),
                           "Oliver J. Rosten",
                           "  ",
                           {.main_cpp{"TestSandbox/TestSandbox.cpp"},
                            .common_includes{"TestShared/SharedIncludes.hpp"}},
                           outputStream};

        runner.register_test<non_ascii_free_test>();
      });

    check_exception_thrown<std::logic_error>(
      reporter{"A test whose name contains a non-ASCII letter"},
      [this](){
        commandline_arguments args{{zeroth_arg()}};
        std::stringstream outputStream{};

        test_runner runner{args.size(),
                           args.get(),
                           "Oliver J. Rosten",
                           "  ",
                           {.main_cpp{"TestSandbox/TestSandbox.cpp"},
                            .common_includes{"TestShared/SharedIncludes.hpp"}},
                           outputStream};

        runner.register_test<caf\u00E9_free_test>();
      });

    check_exception_thrown<std::logic_error>(
      reporter{"A test with no source file"},
      [this](){
        commandline_arguments args{{zeroth_arg()}};
        std::stringstream outputStream{};

        test_runner runner{args.size(),
                           args.get(),
                           "Oliver J. Rosten",
                           "  ",
                           {.main_cpp{"TestSandbox/TestSandbox.cpp"},
                            .common_includes{"TestShared/SharedIncludes.hpp"}},
                           outputStream};

        runner.register_test<sourceless_free_test>();
      });

    // The check is made whichever tests are selected: a test that runs alone would overwrite the other test's summary
    for(const auto& selection : {std::vector<std::string>{},
                                 {"select", summary_collider_test_twin::source_file().generic_string()}})
    {
      std::string_view selected{selection.empty() ? "both" : "one"};
      check_exception_thrown<std::runtime_error>(
        reporter{std::format("Two tests whose summaries are one file, {} selected", selected)},
        [this, &selection](){
          std::vector<std::string> argList{zeroth_arg()};
          argList.append_range(selection);
          commandline_arguments args{argList};
          std::stringstream outputStream{};

          test_runner runner{args.size(),
                             args.get(),
                             "Oliver J. Rosten",
                             "  ",
                             {.main_cpp{"TestSandbox/TestSandbox.cpp"},
                              .common_includes{"TestShared/SharedIncludes.hpp"}},
                             outputStream};

          runner.register_test<summary_collider_test>();
          runner.register_test<summary_collider_test_twin>();
        });
    }

    check_exception_thrown<std::runtime_error>(
      reporter{"Invalid repetitions for instability analysis"},
      [this](){
        test_instability_analysis("", "", "foo", return_code::critical_failures, critical_free_test{});
      }
    );

    check_exception_thrown<std::runtime_error>(
      reporter{"Insufficient repetitions for instability analysis"},
      [this](){
        test_instability_analysis("", "",  "1", return_code::critical_failures, critical_free_test{});
      }
    );
  }

  void test_runner_test::test_critical_errors()
  {
    std::stringstream outputStream{};

    // This is scoped to ensure destruction of the runner - and therefore loggers -
    // before dumping output to a file. The destructors are not trivial in recovery mode.
    {
      commandline_arguments args{{(minimal_fake_path()).generic_string(), "-v", "recover", "dump"}};
  
      auto runner{make_fake_runner(args, outputStream)};

      runner.register_test<bar_free_test>();
      runner.register_test<foo_test>();

      check(equality, "Recovery and dump return code", runner.execute(), return_code::critical_failures);
    }

    const auto outputDir{working_materials() /= "RecoveryAndDumpOutput"};
    fs::create_directory(outputDir);

    if(std::ofstream file{outputDir / "io.txt"})
    {
      file << outputStream.str();
    }

    fs::copy(fake_project() / "output" / "Recovery" / "Recovery.txt", working_materials() /= "RecoveryAndDumpOutput");
    fs::copy(fake_project() / "output" / "Recovery" / "Dump.txt", working_materials() /= "RecoveryAndDumpOutput");
    fs::copy(fake_project() / "output" / "TestSummaries",
             working_materials() /= "RecoveryAndDumpOutput/TestSummaries",
             fs::copy_options::recursive);

    check(equivalence, "Recovery and Dump",
                      working_materials() /= "RecoveryAndDumpOutput",
                      predictive_materials() /= "RecoveryAndDumpOutput");
  }

  void test_runner_test::test_basic_output()
  {
    std::stringstream outputStream{};
    commandline_arguments args{{(minimal_fake_path()).generic_string()}};

    auto runner{make_fake_runner(args, outputStream)};

    check(equality, "No tests return code", runner.execute(), return_code::success);
    check_output("No Tests", "NoTests", outputStream);

    runner.register_test<failing_test>();
    runner.register_test<failing_fp_test>();
    runner.register_test<failing_fn_test>();

    check(equality, "Basic output return code", runner.execute(), return_code::soft_failures);
    check_output("Basic Output", "BasicOutput", outputStream);
  }

  void test_runner_test::test_help_output()
  {
    const std::array<std::pair<std::vector<std::string>, std::string_view>, 4> requests{{
      {{"init"},                   "InitHelpOutput"         },
      {{"create"},                 "CreateHelpOutput"       },
      {{"create", "regular_test"}, "CreateRegularHelpOutput"},
      {{"test"},                   "TestHelpOutput"         }
    }};

    // Failing tests are registered so that a run which went ahead would show in the return code
    for(const auto& [commands, dirName] : requests)
    {
      std::vector<std::string> argList{zeroth_arg()};
      argList.append_range(commands);
      argList.push_back("--help");

      std::stringstream outputStream{};
      auto runner{make_failing_suite(argList, outputStream)};

      check(equality, std::format("{} return code", dirName), runner.execute(), return_code::success);
      check_output(std::format("{} output", dirName), dirName, outputStream);
    }
  }

  void test_runner_test::test_verbose_output()
  {
    std::stringstream outputStream{};
    auto runner{make_failing_suite({{(minimal_fake_path()).generic_string(), "-v"}}, outputStream)};

    check(equality, "Verbose output return code", runner.execute(), return_code::soft_failures);
    check_output("Basic Verbose Output", "BasicVerboseOutput", outputStream);
  }

  void test_runner_test::test_serial_verbose_output()
  {
    std::stringstream outputStream{};
    auto runner{make_failing_suite({{(minimal_fake_path()).generic_string(), "-v", "--serial"}}, outputStream)};

    check(equality, "Serial verbose output return code", runner.execute(), return_code::soft_failures);
    check_output("Basic Serial Verbose Output", "BasicSerialVerboseOutput", outputStream);
  }

  void test_runner_test::test_throwing_tests()
  {
    std::stringstream outputStream{};
    commandline_arguments args{{(minimal_fake_path()).generic_string()}};

    auto runner{make_fake_runner(args, outputStream)};

    runner.register_test<throwing_test>();
    runner.register_test<platform_specific_throwing_test>();

    check(equality, "Throwing tests return code", runner.execute(), return_code::success);
    check_output("Throwing Output", "ThrowingOutput", outputStream);

    const fs::path diagnosticsDir{working_materials() /= "ThrowingDiagnostics"};
    fs::create_directory(diagnosticsDir);
    fs::copy(fake_project() / "output/DiagnosticsOutput/Tests", diagnosticsDir);

    check(equivalence, "Exception Output", predictive_materials() / "ThrowingDiagnostics", diagnosticsDir);
  }

  /** The fake tests tell the mechanism from its rivals: `record_reading_free_test` sees its record while it executes,
      which a start written only at the end would not produce; `escaping_exception_free_test` throws, which an
      execution duration written only on normal completion would miss. The records directory is removed first, so that
      only this run can have written the stamp.
   */
  void test_runner_test::test_execution_records()
  {
    std::stringstream outputStream{};
    commandline_arguments args{{(minimal_fake_path()).generic_string()}};
    test_runner runner{args.size(),
                       args.get(),
                       "Oliver J. Rosten",
                       "  ",
                       {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{"TestShared/SharedIncludes.hpp"}},
                       outputStream};

    const auto& projPaths{runner.proj_paths()};
    fs::remove_all(projPaths.execution_records().dir());

    runner.register_test<record_reading_free_test>();
    runner.register_test<escaping_exception_free_test>();

    check(equality, "Execution records return code", runner.execute(), return_code::critical_failures);

    const test_execution_record_path
      passingRecord{record_reading_free_test::source_file(),     test_name<record_reading_free_test>(),     projPaths},
      throwingRecord{escaping_exception_free_test::source_file(), test_name<escaping_exception_free_test>(), projPaths};

    const std::vector<std::string> finishedRecordLabels{"started", "execution duration", "runner overhead"};
    check(equality,
          "The record of a test which passed names its start, its execution duration and the runner's overhead",
          execution_record_labels(passingRecord.file_path()),
          finishedRecordLabels);

    check(equality,
          "The record of a test whose body threw names its start, its execution duration and the runner's overhead",
          execution_record_labels(throwingRecord.file_path()),
          finishedRecordLabels);

    const auto runStart{start_named_by(projPaths.execution_records().stamp())};
    check("The run's stamp names a start", !runStart.empty());
    const bool runStartedFirst{   (runStart <= start_named_by(passingRecord.file_path()))
                               && (runStart <= start_named_by(throwingRecord.file_path()))};
    check("The run started no later than either test", runStartedFirst);
  }

  void test_runner_test::test_filtered_suites()
  {
    std::stringstream outputStream{};
    commandline_arguments args{{(minimal_fake_path()).generic_string(), "test", "Failing"}};

    auto runner{make_fake_runner(args, outputStream)};

    runner.register_test<passing_test>();

    runner.register_test<failing_test>();
    runner.register_test<failing_fp_test>();
    runner.register_test<failing_fn_test>();

    check(equality, "Filtered suites return code", runner.execute(), return_code::soft_failures);
    check_output("Filtered Suite Output", "FilteredSuiteOutput", outputStream);
  }

  void test_runner_test::test_suites_not_found()
  {
    // Neither suite matches a test. Only the suite spelt as a source file draws a hint to use 'select'
    std::stringstream outputStream{};
    commandline_arguments args{{(minimal_fake_path()).generic_string(), "test", "Absent", "test", "absent_test.cpp"}};

    test_runner runner{args.size(),
                       args.get(),
                       "Oliver J. Rosten",
                       "  ",
                       {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{"TestShared/SharedIncludes.hpp"}},
                       outputStream};

    runner.register_test<passing_test>();
    runner.register_test<failing_test>();

    check(equality, "Suites not found return code", runner.execute(), return_code::success);
    check_output("Suites Not Found Output", "SuitesNotFoundOutput", outputStream);
  }

  void test_runner_test::test_prune_basic_output()
  {
    fs::remove_all(output_paths{fake_project()}.dir());

    std::stringstream outputStream{};
    commandline_arguments args{{(minimal_fake_path()).generic_string(), "prune"}};

    auto runner{make_fake_runner(args, outputStream)};

    check(equality, "Prune with no stamp return code", runner.execute(), return_code::success);
    check_output("Prune with no stamp", "PruneWithNoStamp", outputStream);

    check(equality, "Prune with no tests return code", runner.execute(), return_code::success);
    check_output("Prune with no tests", "PruneWithNoTests", outputStream);

    // An exclusion does not turn prune off, and one matching nothing is reported under prune
    fs::remove_all(output_paths{fake_project()}.dir());

    std::stringstream excludingStream{};
    commandline_arguments excludingArgs{{minimal_fake_path().generic_string(), "prune", "exclude", "Failing/absent_test.cpp"}};

    test_runner excludingRunner{excludingArgs.size(),
                                excludingArgs.get(),
                                "Oliver J. Rosten",
                                "  ",
                                {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{"TestShared/SharedIncludes.hpp"}},
                                excludingStream};

    check(equality, "Prune with an exclusion return code", excludingRunner.execute(), return_code::success);
    check_output("Prune with an exclusion", "PruneWithExclusionOutput", excludingStream);
  }

  [[nodiscard]]
  test_runner_test::fake_build test_runner_test::write_fake_build()
  {
    fs::remove_all(output_paths{fake_project()}.dir());

    // A build of one test's source, Tests/ThingTest.cpp, which read one toolchain header
    const auto buildDir{minimal_fake_path().parent_path()};
    const fake_build build{.source{fake_project() / "Tests" / "ThingTest.cpp"}, .toolchainHeader{fake_project() / "Toolchain" / "vector"}};
    fs::create_directories(build.toolchainHeader.parent_path());
    fs::create_directories(buildDir / "CMakeFiles" / "4.1.2");
    write_to_file(build.source, "", std::ios_base::out);
    write_to_file(build.toolchainHeader, "", std::ios_base::out);
    write_to_file(buildDir / "CMakeFiles" / "4.1.2" / "CMakeCXXCompiler.cmake",
                  std::format("set(CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES \"{}\")\n", build.toolchainHeader.parent_path().generic_string()),
                  std::ios_base::out);
    write_to_file(buildDir / "build.ninja",
                  std::format("build CMakeFiles/x.dir/ThingTest.cpp.o: CXX_COMPILER {}\n", build.source.generic_string()),
                  std::ios_base::out);
    write_ninja_deps(buildDir / ".ninja_deps",
                     std::vector<compilation_record>{{"CMakeFiles/x.dir/ThingTest.cpp.o", {build.source, build.toolchainHeader}}});

    return build;
  }

  void test_runner_test::test_prune_with_changed_toolchain()
  {
    // The toolchain header modified after the previous run's stamp
    const auto build{write_fake_build()};

    commandline_arguments args{{(minimal_fake_path()).generic_string(), "prune"}};
    const project_paths projPaths{args.size(), args.get(), {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{"TestShared/SharedIncludes.hpp"}}};
    const auto stamp{projPaths.prune().stamp()};
    fs::create_directories(stamp.parent_path());
    write_to_file(stamp, "", std::ios_base::out);

    // Both files the build read are stamped strictly before the executable: where last_write_time
    // resolves to whole seconds, a file written in the same second as the executable is out of date
    using namespace std::chrono_literals;
    const auto now{std::chrono::file_clock::now()};
    fs::last_write_time(stamp, now - 2s);
    fs::last_write_time(build.source, now - 1s);
    fs::last_write_time(build.toolchainHeader, now - 1s);
    fs::last_write_time(projPaths.executable(), now);

    std::stringstream outputStream{};
    auto runner{make_fake_runner(args, outputStream)};

    check(equality, "Prune with changed toolchain return code", runner.execute(), return_code::success);
    check_output("Prune with changed toolchain", "PruneWithChangedToolchain", outputStream);
  }

  void test_runner_test::test_prune_selects_a_test_this_executable_lacks()
  {
    // The source modified after the previous run's stamp, the toolchain before it: prune selects
    // ThingTest.cpp, which no test registered here defines, and that is not a selection to report
    const auto build{write_fake_build()};

    commandline_arguments args{{(minimal_fake_path()).generic_string(), "prune"}};
    const project_paths::customizer customization{
      .main_cpp{"TestSandbox/TestSandbox.cpp"},
      .common_includes{"TestShared/SharedIncludes.hpp"}
    };
    const project_paths projPaths{args.size(), args.get(), customization};
    const auto stamp{projPaths.prune().stamp()};
    fs::create_directories(stamp.parent_path());
    write_to_file(stamp, "", std::ios_base::out);

    using namespace std::chrono_literals;
    const auto now{std::chrono::file_clock::now()};
    fs::last_write_time(build.toolchainHeader, now - 3s);
    fs::last_write_time(stamp, now - 2s);
    fs::last_write_time(build.source, now - 1s);
    fs::last_write_time(projPaths.executable(), now);

    std::stringstream outputStream{};
    auto runner{make_fake_runner(args, outputStream)};

    runner.register_test<passing_test>();
    check(equality, "Prune selecting an unregistered test return code", runner.execute(), return_code::success);
    check_output("Prune selecting an unregistered test", "PruneSelectsUnregisteredOutput", outputStream);
  }

  void test_runner_test::test_post_run_failure()
  {
    std::stringstream outputStream{};
    commandline_arguments args{{(minimal_fake_path()).generic_string(), "test", "Failing"}};

    auto runner{make_fake_runner(args, outputStream)};

    runner.register_test<passing_test>();
    runner.register_test<failing_test>();

    // A filtered run merges its results into the previous failures, so a malformed record there fails the prune write
    const auto failuresFile{runner.proj_paths().prune().to_rerun(std::nullopt)};
    fs::create_directories(failuresFile.parent_path());
    const transient_file malformedFailures{failuresFile, "garbage\n"};

    check(equality, "Post-run failure return code", runner.execute(), return_code::soft_failures | return_code::post_run_failures);
    check_output("Post-Run Failure Output", "PostRunFailureOutput", outputStream);
  }

  void test_runner_test::test_materials_update()
  {
    std::stringstream outputStream{};
    commandline_arguments args{{(minimal_fake_path()).generic_string(), "u"}};

    auto runner{make_fake_runner(args, outputStream)};

    runner.register_test<stale_predictions_free_test>();

    check(equality, "Materials update return code", runner.execute(), return_code::soft_failures);
    check_output("Materials Update Output", "MaterialsUpdateOutput", outputStream);

    const auto predictions{
      fake_project() / "TestMaterials/Updating/StalePredictionsFreeTest" / "stale_predictions_free_test/Prediction"
    };

    check(equality,
          "Prediction overwritten",
          read_to_string(predictions / "Kept.txt", std::ios_base::in).value_or(""),
          std::string{"Obtained\n"});

    check("Prediction deleted", !fs::exists(predictions / "Obsolete.txt"));
  }

  /** As `test_materials_update`, which shows the update happening, except that the fake throws after
      its failed check. The soft failure is still recorded, so only the critical failure can stop the update.
   */
  void test_runner_test::test_no_materials_update_after_critical_failure()
  {
    std::stringstream outputStream{};
    commandline_arguments args{{(minimal_fake_path()).generic_string(), "u"}};

    auto runner{make_fake_runner(args, outputStream)};

    runner.register_test<throwing_stale_predictions_free_test>();

    check(equality,
          "No materials update after a critical failure return code",
          runner.execute(),
          return_code::soft_failures | return_code::critical_failures);

    check_output("No Materials Update After a Critical Failure Output",
                 "NoMaterialsUpdateAfterCriticalFailureOutput",
                 outputStream);

    const auto predictions{
      fake_project() / "TestMaterials/Updating/ThrowingStalePredictionsFreeTest" / "throwing_stale_predictions_free_test/Prediction"
    };

    check(equality,
          "Prediction not overwritten",
          read_to_string(predictions / "Kept.txt", std::ios_base::in).value_or(""),
          std::string{"Predicted\n"});

    check("Prediction not deleted", fs::exists(predictions / "Obsolete.txt"));
  }

  void test_runner_test::test_partial_materials_update()
  {
    std::stringstream outputStream{};
    commandline_arguments args{{(minimal_fake_path()).generic_string(), "u"}};

    auto runner{make_fake_runner(args, outputStream)};

    runner.register_test<type_swapped_predictions_free_test>();

    check(equality,
          "Partial materials update return code",
          runner.execute(),
          return_code::soft_failures | return_code::post_run_failures);

    check_output("Partial Materials Update Output", "PartialMaterialsUpdateOutput", outputStream);
  }

  /** The counterpart of `test_materials_update` for a test whose materials are discriminated. The
      declared discriminator's materials are prepared and updated, and the other's are left alone.
   */
  void test_runner_test::test_discriminated_materials_update()
  {
    std::stringstream outputStream{};
    commandline_arguments args{{(minimal_fake_path()).generic_string(), "u"}};
    auto runner{make_fake_runner(args, outputStream)};

    runner.register_test<variant_free_test>();

    check(equality, "Discriminated materials update return code", runner.execute(), return_code::soft_failures);
    check_output("Discriminated Materials Update Output", "DiscriminatedMaterialsUpdateOutput", outputStream);

    const auto materials{fake_project() / "TestMaterials/Updating/VariantFreeTest/variant_free_test"};

    check(equality,
          "Declared discriminator's prediction overwritten",
          read_to_string(materials / "Platypus/Prediction/Kept.txt", std::ios_base::in).value_or(""),
          std::string{"Obtained\n"});

    check("Declared discriminator's prediction deleted", !fs::exists(materials / "Platypus/Prediction/Obsolete.txt"));

    check(equality,
          "Other discriminator's prediction not overwritten",
          read_to_string(materials / "Echidna/Prediction/Kept.txt", std::ios_base::in).value_or(""),
          std::string{"Predicted\n"});

    check("Other discriminator's prediction not deleted", fs::exists(materials / "Echidna/Prediction/Obsolete.txt"));
  }

  void test_runner_test::test_nested_suite()
  {
      std::stringstream outputStream{};
      commandline_arguments args{{(minimal_fake_path()).generic_string()}};

      auto runner{make_fake_runner(args, outputStream)};

      using namespace object;

      runner.register_test<failing_test>();
      runner.register_test<failing_fp_test>();
      runner.register_test<failing_fn_test>();

      check(equality, "Nested suite return code", runner.execute(), return_code::soft_failures);
      check_output("Basic Nested Output", "BasicNestedOutput", outputStream);
  }

  void test_runner_test::test_suite_named_as_a_sibling_test()
  {
    std::stringstream outputStream{};
    commandline_arguments args{{(minimal_fake_path()).generic_string(), "-v"}};

    auto runner{make_fake_runner(args, outputStream)};

    runner.register_test<namesake_test>();
    runner.register_test<under_namesake_test>();

    check(equality, "Suite named as a sibling test return code", runner.execute(), return_code::success);
    check_output("Suite Named As A Sibling Test", "SuiteNamedAsASiblingTest", outputStream);
  }

  void test_runner_test::test_nested_suite_verbose()
  {
    std::stringstream outputStream{};
    commandline_arguments args{{(minimal_fake_path()).generic_string(), "-v"}};

    auto runner{make_fake_runner(args, outputStream)};

    using namespace object;

    runner.register_test<failing_test>();
    runner.register_test<failing_fp_test>();
    runner.register_test<failing_fn_test>();

    check(equality, "Nested suite verbose return code", runner.execute(), return_code::soft_failures);
    check_output("Verbose Nested Output", "VerboseNestedOutput", outputStream);
  }

  void test_runner_test::test_excluded_performance_tests()
  {
    auto run{
      [this](std::string_view description, std::string_view outputDirName, std::initializer_list<std::string_view> extraArgs){
        std::stringstream outputStream{};

        std::vector<std::string> argList{(minimal_fake_path()).generic_string()};
        argList.insert(argList.end(), extraArgs.begin(), extraArgs.end());
        commandline_arguments args{argList};

        auto runner{make_fake_runner(args, outputStream)};

        runner.register_test<passing_test>();
        runner.register_test<fake_performance_test>();

        check(equality, append_lines(description, "Return code"), runner.execute(), return_code::success);
        check_output(description, outputDirName, outputStream);
      }
    };

    // The same registrations both ways, so the option is the only thing which differs.
    run("Performance tests included", "IncludedPerformanceOutput", {});
    run("Performance tests excluded", "ExcludedPerformanceOutput", {"--exclude-performance"});
  }

  void test_runner_test::test_excluded_tests()
  {
    auto run{
      [this](std::string_view description, std::string_view outputDirName, std::initializer_list<std::string> extraArgs, return_code expected){
        std::stringstream outputStream{};

        std::vector<std::string> argList{minimal_fake_path().generic_string()};
        argList.insert(argList.end(), extraArgs.begin(), extraArgs.end());
        commandline_arguments args{argList};

        auto runner{make_fake_runner(args, outputStream)};

        runner.register_test<passing_test>();
        runner.register_test<failing_test>();
        runner.register_test<fake_performance_test>();

        check(equality, append_lines(description, "Return code"), runner.execute(), expected);
        check_output(description, outputDirName, outputStream);
      }
    };

    // The same registrations each way, so the arguments are the only thing which differs; the
    // failing test's return code says whether it ran
    const auto failing{failing_test::source_file().generic_string()};
    const auto performance{fake_performance_test::source_file().generic_string()};

    run("Failing test excluded",                          "ExcludedTestOutput",
        {"exclude", failing},                              return_code::success);
    run("Exclusion matching no test",                     "ExclusionNotFoundOutput",
        {"exclude", "Failing/absent_test.cpp"},            return_code::soft_failures);
    run("Exclusion naming a suite",                       "ExclusionOfSuiteOutput",
        {"exclude", "Failing"},                            return_code::soft_failures);
    run("Selected test excluded, and so found both ways", "SelectedTestExcludedOutput",
        {"select", failing, "exclude", failing},           return_code::success);
    run("Performance test excluded both ways, and found", "PerformanceTestExcludedOutput",
        {"--exclude-performance", "exclude", performance}, return_code::soft_failures);
    run("Exclusion by alias",                             "ExcludedTestOutput",
        {"e", failing},                                    return_code::success);
  }

  void test_runner_test::test_excluded_tests_are_rerun()
  {
    // A run with no selection is full: it stamps, and whatever it left out is recorded for the
    // next run, since that test's status is unknown
    auto run{
      [this](std::string_view description, std::initializer_list<std::string> extraArgs, std::vector<fs::path> toRerun) {
        fs::remove_all(output_paths{fake_project()}.dir());

        std::vector<std::string> argList{minimal_fake_path().generic_string()};
        argList.insert(argList.end(), extraArgs.begin(), extraArgs.end());
        commandline_arguments args{argList};

        std::stringstream outputStream{};
        auto runner{make_fake_runner(args, outputStream)};

        runner.register_test<passing_test>();
        runner.register_test<fake_performance_test>();

        const auto prunePaths{runner.proj_paths().prune()};
        check(equality, append_lines(description, "Return code"), runner.execute(), return_code::success);
        check(equality, append_lines(description, "Stamped"),     fs::exists(prunePaths.stamp()), true);

        // The file holds paths relative to the project, so the names are compared
        auto name{[](const prune_record& r){ return r.test_path.filename(); }};
        const auto recorded{read_tests(prunePaths.to_rerun(std::nullopt)) | std::views::transform(name) | std::ranges::to<std::vector>()};
        check(equality, append_lines(description, "To rerun"), recorded, toRerun);
      }
    };

    const auto performance{fake_performance_test::source_file()};

    run("A full run",                        {},                                        {});
    run("A run excluding a test",            {"exclude", performance.generic_string()}, {performance.filename()});
    run("A run excluding performance tests", {"--exclude-performance"},                 {performance.filename()});
  }

  void test_runner_test::test_dump_comparison()
  {
    enum class registrations { passing_and_failing, passing, passing_failing_and_performance };

    auto run{
      [this](std::string_view description,
             std::string_view outputDirName,
             std::initializer_list<std::string> extraArgs,
             registrations registered,
             return_code expected) {
        std::vector<std::string> argList{minimal_fake_path().generic_string(), "dump"};
        argList.insert(argList.end(), extraArgs.begin(), extraArgs.end());
        commandline_arguments args{argList};

        std::stringstream outputStream{};
        auto runner{make_fake_runner(args, outputStream)};

        runner.register_test<passing_test>();
        if(registered != registrations::passing)
          runner.register_test<failing_test>();

        if(registered == registrations::passing_failing_and_performance)
          runner.register_test<fake_performance_test>();

        check(equality, append_lines(description, "Return code"), runner.execute(), expected);
        check_output(description, outputDirName, outputStream);
      }
    };

    auto failing{
      [this](std::string_view description, std::initializer_list<std::string> extraArgs) {
        check_exception_thrown<std::runtime_error>(description, [this, extraArgs](){
          std::vector<std::string> argList{minimal_fake_path().generic_string(), "dump"};
          argList.insert(argList.end(), extraArgs.begin(), extraArgs.end());
          commandline_arguments args{argList};

          std::stringstream outputStream{};
          auto runner{make_fake_runner(args, outputStream)};

          runner.register_test<passing_test>();
          return runner.execute();
        });
      }
    };

    fs::remove_all(output_paths{fake_project()}.dir());
    const auto recovery{output_paths{fake_project()}.recovery()};

    run("A dump kept under a name", "DumpKeptOutput", {"--as", "before"}, registrations::passing_and_failing, return_code::soft_failures);
    check(equality, "The kept dump exists", fs::exists(recovery.kept_dump("before")), true);

    run("The same checks as the kept dump",    "DumpUnchangedOutput",
        {"--against", "before"}, registrations::passing_and_failing,             return_code::soft_failures);
    run("A check missing since the kept dump", "DumpMissingOutput",
        {"--against", "before"}, registrations::passing,                         return_code::success);
    run("A check added since the kept dump",   "DumpAddedOutput",
        {"--against", "before"}, registrations::passing_failing_and_performance, return_code::soft_failures);

    // Compared before kept, so one run may take the name it compared against
    run("A dump compared against a name and then kept under it", "DumpAddedOutput",
        {"--against", "before", "--as", "before"}, registrations::passing_failing_and_performance, return_code::soft_failures);
    run("The kept dump is the later one", "DumpUnchangedThreeOutput",
        {"--against", "before"}, registrations::passing_failing_and_performance, return_code::soft_failures);

    failing("Comparison against a dump never kept", {"--against", "never"});
    failing("A dump kept under no name",            {"--as", ""});
    failing("Comparison against no name",           {"--against", ""});

    // recover, like dump, writes under output/Recovery on a tree which has no output directory yet
    fs::remove_all(output_paths{fake_project()}.dir());
    std::stringstream recoveringStream{};
    commandline_arguments recoveringArgs{{minimal_fake_path().generic_string(), "recover"}};
    test_runner recoveringRunner{recoveringArgs.size(),
                                 recoveringArgs.get(),
                                 "Oliver J. Rosten",
                                 "  ",
                                 {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{"TestShared/SharedIncludes.hpp"}},
                                 recoveringStream};

    recoveringRunner.register_test<passing_test>();
    check(equality, "recover on a fresh tree", recoveringRunner.execute(), return_code::success);
    check("A recovery run which ran a check leaves a recovery file", fs::exists(recovery.recovery_file()));

    // A run which records nothing must not leave the previous run's record looking like the new run's
    std::stringstream emptyRecoveryStream{};
    test_runner emptyRecoveryRunner{recoveringArgs.size(),
                                    recoveringArgs.get(),
                                    "Oliver J. Rosten",
                                    "  ",
                                    {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{"TestShared/SharedIncludes.hpp"}},
                                    emptyRecoveryStream};

    check(equality, "recover with no tests", emptyRecoveryRunner.execute(), return_code::success);
    check("A recovery run which ran no check leaves no recovery file", !fs::exists(recovery.recovery_file()));
  }

  void test_runner_test::test_thread_pool()
  {
    auto run{
      [this](std::string_view description, std::string_view outputDirName, std::string_view poolSize) {
        commandline_arguments args{{zeroth_arg(), "--thread-pool", std::string{poolSize}}};

        std::stringstream outputStream{};
        auto runner{make_fake_runner(args, outputStream)};

        runner.register_test<passing_test>();
        check(equality, append_lines(description, "Return code"), runner.execute(), return_code::success);
        check_output(description, outputDirName, outputStream);
      }
    };

    run("A pool of no threads is refused, and the run goes ahead", "ThreadPoolOfNoThreadsOutput", "0");
    run("A pool of two threads running one test",                  "ThreadPoolForOneTestOutput",  "2");
  }

  void test_runner_test::test_instability_analysis()
  {
    test_instability_analysis("Instability comprising pass/failure",
                              "BinaryInstabilityAnalysis",
                              "2",
                              return_code::soft_failures,
                              {"--serial"},
                              flipper_free_test{});

    test_instability_analysis("Instability comprising pass/multiple distinct failures",
                              "MultiInstabilityAnalysis",
                              "4",
                              return_code::soft_failures,
                              {"--serial"},
                              periodic_free_test{});

    test_instability_analysis("Instability comprising failures from two checks",
                              "MultiCheckInstabilityAnalysis",
                              "6",
                              return_code::soft_failures,
                              {"--serial"},
                              multi_periodic_free_test{});

    test_instability_analysis("Instability following consistent failure",
                              "BinaryInstabilityFollowingFailures",
                              "2",
                              return_code::soft_failures,
                              {"--serial"},
                              failing_plus_instabilities_free_test{});

    test_instability_analysis("Failure but no instability",
                              "ConsistentFailureNoInstability",
                              "2",
                              return_code::soft_failures,
                              {"--serial"},
                              consistently_failing_free_test{});

    test_instability_analysis("Always passes",
                              "ConsistentSuccessNoInstability",
                              "2",
                              return_code::success,
                              {"--serial"},
                              consistently_passing_free_test{});

    test_instability_analysis("Critical failure instability",
                              "CriticalFailureInstability",
                              "2",
                              return_code::critical_failures,
                              {"--serial"},
                              critical_free_test{});

    test_instability_analysis("Two tests always passing",
                              "ConsistentSuccessTwoTests",
                              "2",
                              return_code::success,
                              {"--serial"},
                              consistently_passing_free_test{},
                              another_consistently_passing_free_test{});

    test_instability_analysis("Consistent success/consistent failure/instability",
                              "MixedBag",
                              "6",
                              return_code::soft_failures,
                              {"--serial"},
                              consistently_passing_free_test{},
                              consistently_failing_free_test{},
                              flipper_free_test{},
                              multi_periodic_free_test{}
                             );
  }

  template<std::invocable<test_runner&> Manipulator, concrete_test... Ts>
  void test_runner_test::test_instability_analysis(std::string_view message,
                                                   std::string_view outputDirName,
                                                   std::string_view numRuns,
                                                   return_code expected,
                                                   std::initializer_list<std::string_view> extraArgs,
                                                   Manipulator manipulator,
                                                   Ts&&...)
  {
    std::stringstream outputStream{};

    auto argGenerator{
      [this,&extraArgs, numRuns](){
         std::vector<std::string> argList{(minimal_fake_path()).generic_string(), "locate-instabilities", std::string{numRuns}};
         argList.insert(argList.end(), extraArgs.begin(), extraArgs.end());
         return argList;
      }
    };

    commandline_arguments args{argGenerator()};


    auto runner{make_fake_runner(args, outputStream)};

    (runner.register_test<std::remove_cvref_t<Ts>>(), ...);

    manipulator(runner);

    check(equality, reporter{append_lines(message, "Return code")}, runner.execute(), expected);

    const auto outputDir{working_materials() /= outputDirName};
    fs::create_directory(outputDir);

    if(std::ofstream file{outputDir / "io.txt"})
    {
      file << outputStream.str();
    }

    check(equivalence, reporter(append_lines(message, make_type_info<Ts...>())),
                      outputDir,
                      predictive_materials() /= outputDirName);
  }

  template<concrete_test... Ts>
  void test_runner_test::test_instability_analysis(std::string_view message,
                                                   std::string_view outputDirName,
                                                   std::string_view numRuns,
                                                   return_code expected,
                                                   std::initializer_list<std::string_view> extraArgs,
                                                   Ts&&... ts)
  {
    test_instability_analysis(message, outputDirName, numRuns, expected, extraArgs, [](test_runner&){}, std::forward<Ts>(ts)...);
  }

  template<concrete_test... Ts>
  void test_runner_test::test_instability_analysis(std::string_view message,
                                                   std::string_view outputDirName,
                                                   std::string_view numRuns,
                                                   return_code expected,
                                                   Ts&&... ts)
  {
    test_instability_analysis(message, outputDirName, numRuns, expected, {}, [](test_runner&){}, std::forward<Ts>(ts)...);
  }

  namespace
  {
    class selected_in_spaced_suite_free_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return make_fake_file_path<selected_in_spaced_suite_free_test>("Spaced Suite");
      }

      void run_tests() {}
    };

    class excluded_from_spaced_suite_free_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return make_fake_file_path<excluded_from_spaced_suite_free_test>("Spaced Suite");
      }

      void run_tests() {}
    };
  }

  /** A copy of the fake project lies in a directory whose name holds a space, and so does the name of
      its executable. The executable is a script which stands in for the test runner in each sandbox: it
      records its arguments, one per line, and creates the directory to which a sandboxed run writes its
      analysis. The run selects a suite and a source, and excludes a source, each named with a space. The
      shell splits an unquoted word at the space. So if the coordinator does not quote the path, no
      sandbox runs, and if it does not quote a selection, the sandboxes record it as two arguments.
   */
  void test_runner_test::test_instability_analysis_in_sandboxes_from_a_path_with_a_space()
  {
    using runtime::quote_for_shell;

    const auto spacedProject{scratchpad_materials() /= "Spaced Fake Project"};
    fs::remove_all(spacedProject);
    fs::copy(fake_project(), spacedProject, fs::copy_options::recursive);

    const auto outputDir{working_materials() /= "SandboxesFromAPathWithASpace"};
    fs::create_directory(outputDir);

    const auto quotedArgumentsFile{quote_for_shell((outputDir / "Arguments.txt").string())},
               quotedAnalysisDir{quote_for_shell(output_paths::instability_analysis(fs::canonical(spacedProject)).string())};

    const auto executable{spacedProject / "build/CMade" / (with_windows_v ? "Run Tests.bat" : "Run Tests")};
    const auto script{
      with_windows_v ? std::format("@echo off\n"
                                   "mkdir {} 2>nul\n"
                                   "for %%a in (%*) do >>{} echo %%~a\n"
                                   "exit /b 0\n",
                                   quotedAnalysisDir,
                                   quotedArgumentsFile)
                     : std::format("#!/bin/sh\n"
                                   "mkdir -p {}\n"
                                   "printf '%s\\n' \"$@\" >> {}\n",
                                   quotedAnalysisDir,
                                   quotedArgumentsFile)
    };

    write_to_file(executable, script, std::ios_base::out);
    fs::permissions(executable, fs::perms::owner_exec, fs::perm_options::add);

    std::stringstream outputStream{};
    commandline_arguments args{{executable.generic_string(),
                                "locate", "2", "--sandbox",
                                "test", "Spaced Suite",
                                "select", selected_in_spaced_suite_free_test::source_file().generic_string(),
                                "exclude", excluded_from_spaced_suite_free_test::source_file().generic_string()}};

    auto runner{make_fake_runner(args, outputStream)};
    runner.register_test<selected_in_spaced_suite_free_test>();
    runner.register_test<excluded_from_spaced_suite_free_test>();

    check(equality, "Both sandboxes run, and the run succeeds", runner.execute(), return_code::success);
    check(equivalence,
          "Each sandbox is given the repetitions, its runner id and the selections, each as one argument",
          outputDir,
          predictive_materials() /= "SandboxesFromAPathWithASpace");
  }

  namespace
  {
    /// Its original materials hold `Stray.txt` beside the working copy, where nothing uses it
    class stray_materials_free_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return "Tests/Preparation/StrayMaterialsFreeTest.cpp";
      }

      void run_tests()
      {
        check("Run despite stray materials", true);
      }
    };

    class unaffected_free_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return "Tests/Preparation/UnaffectedFreeTest.cpp";
      }

      void run_tests()
      {
        check("Run beside a test whose materials could not be prepared", true);
      }
    };
  }

  /** A failure to prepare a test's materials is that test's critical failure: it does not run, and
      the run goes on to the next test.
   */
  void test_runner_test::test_materials_preparation_failure()
  {
    std::stringstream outputStream{};
    commandline_arguments args{{(minimal_fake_path()).generic_string()}};

    auto runner{make_fake_runner(args, outputStream)};

    runner.register_test<stray_materials_free_test>();
    runner.register_test<unaffected_free_test>();

    check(equality, "Materials preparation failure return code", runner.execute(), return_code::critical_failures);
    check_output("Materials Preparation Failure Output", "MaterialsPreparationFailureOutput", outputStream);

    const test_execution_record_path
      record{stray_materials_free_test::source_file(), test_name<stray_materials_free_test>(), runner.proj_paths()};

    check(equality,
          "A test whose materials could not be prepared has an execution duration of zero: preparing them is overhead",
          execution_record_value(record.file_path(), "execution duration"),
          std::string{"0us"});
  }

  namespace
  {
    class unwritable_output_free_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return "Tests/Output/UnwritableOutputFreeTest.cpp";
      }

      void run_tests()
      {
        check("Run, though its versioned output cannot be written", true);
      }
    };

    class beside_unwritable_output_free_test final : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return "Tests/Output/BesideUnwritableOutputFreeTest.cpp";
      }

      void run_tests()
      {
        check("Run beside a test whose versioned output could not be written", true);
      }
    };
  }

  /** A failure to write a test's versioned output is that test's critical failure, and the run
      completes: the other test's results and the grand totals are still reported. A directory where
      the test's exceptions file belongs makes the write fail.
   */
  void test_runner_test::test_versioned_output_failure()
  {
    std::stringstream outputStream{};
    commandline_arguments args{{(minimal_fake_path()).generic_string()}};

    test_runner runner{args.size(),
                       args.get(),
                       "Oliver J. Rosten",
                       "  ",
                       {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{"TestShared/SharedIncludes.hpp"}},
                       outputStream};

    runner.register_test<unwritable_output_free_test>();
    runner.register_test<beside_unwritable_output_free_test>();

    fs::create_directories(fake_project() / "output/DiagnosticsOutput/Tests/Output/unwritable_output_free_test_Exceptions.txt");

    check(equality, "Versioned output failure return code", runner.execute(), return_code::critical_failures);
    check_output("Versioned Output Failure Output", "VersionedOutputFailureOutput", outputStream);
  }

  void test_runner_test::test_exit_statuses()
  {
    // The expected statuses are literals, so that the expectations do not restate the implementation's formula.
    check(equality, "Success exits with 0",                   to_exit_code(return_code::success),                0);
    check(equality, "Versioned output diffs exit with 81",    to_exit_code(return_code::versioned_output_diffs), 81);
    check(equality, "Soft failures exit with 82",             to_exit_code(return_code::soft_failures),          82);
    check(equality, "Critical failures exit with 84",         to_exit_code(return_code::critical_failures),      84);
    check(equality, "An incomplete run exits with 88",        to_exit_code(return_code::incomplete_run),         88);
    check(equality, "Post-run failures exit with 96",         to_exit_code(return_code::post_run_failures),      96);
    check(equality, "Every flag together exits with 111",     to_exit_code(static_cast<return_code>(31)),         111);
    check(equality,
          "Bits no status can carry exit as an incomplete run",
          to_exit_code(static_cast<return_code>(64)),
          88);

    // Every combination of the five flags survives the round trip.
    const auto codes{std::views::iota(0, 32) | std::views::transform([](int i){ return static_cast<return_code>(i); })};
    auto roundTrip{[](return_code code){ return child_return_code(to_exit_code(code), "A child"); }};
    check(equality,
          "Each exit status decodes to the code it encodes",
          codes | std::views::transform(roundTrip) | std::ranges::to<std::vector>(),
          codes | std::ranges::to<std::vector>());

    check(equality,
          "A child exiting 88 reports an incomplete run",
          child_return_code(88, "A child"),
          return_code::incomplete_run);

    // The first seven statuses are those a process gives when it fails for reasons of its own: generic,
    // LeakSanitizer's, the ends of sysexits' range, ThreadSanitizer's and MemorySanitizer's. The last two lie
    // either side of the runner's range. Each status is worded alike on every platform, so the whole message is checked.
    for(const int status : {1, 2, 23, 64, 66, 77, 78, 80, 112})
    {
      check_exception_thrown<std::runtime_error>(
        std::format("Exit status {} is not a runner's", status),
        [status](){ return child_return_code(status, "A child"); });
    }

    // The wording for these statuses differs by platform, so only the refusal is checked.
    auto refused{
      [](int status) {
        try
        {
          (void)child_return_code(status, "A child");
        }
        catch(const std::runtime_error&)
        {
          return true;
        }

        return false;
      }
    };

    check("A status of -1 is refused",                refused(-1));
    check("A status near INT_MIN is refused",         refused(std::numeric_limits<int>::min() + 3));
    check("A shell's 'not executable' is refused",    refused(126));
    check("A shell's 'not found' is refused",         refused(127));
    check("A status above 128 is refused",            refused(139));
  }
}
