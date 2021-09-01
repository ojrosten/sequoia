////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for TestRunner.hpp.
*/

#include "sequoia/TestFramework/TestRunner.hpp"

#include "sequoia/Parsing/CommandLineArguments.hpp"
#include "sequoia/TestFramework/FileEditors.hpp"
#include "sequoia/TestFramework/ProjectCreator.hpp"
#include "sequoia/TestFramework/Summary.hpp"
#include "sequoia/TestFramework/TestCreator.hpp"

#include "sequoia/TextProcessing/Substitutions.hpp"

#include <fstream>
#include <future>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    struct nascent_test_data
    {
      nascent_test_data(std::string type, std::string subType, test_runner& r, std::vector<nascent_test_vessel>& nascentTests);

      void operator()(const parsing::commandline::arg_list& args);

      std::string genus, species;
      test_runner& runner;
      std::vector<nascent_test_vessel>& nascent_tests;
    };


    nascent_test_data::nascent_test_data(std::string type, std::string subType, test_runner& r, std::vector<nascent_test_vessel>& nascentTests)
      : genus{std::move(type)}
      , species{std::move(subType)}
      , runner{r}
      , nascent_tests{nascentTests}
    {}

    void nascent_test_data::operator()(const parsing::commandline::arg_list& args)
    {
      nascent_test_factory factory{{"semantic", "allocation", "behavioural"}, runner.proj_paths(), runner.copyright(), runner.code_indent(), runner.stream()};
      auto nascent{factory.create(genus)};

      std::visit(
        variant_visitor{
          [&args,&species = species](nascent_semantics_test& nascent) {
            nascent.test_type(species);
            nascent.qualified_name(args[0]);
            nascent.add_equivalent_type(args[1]);
          },
          [&args,&species = species](nascent_allocation_test& nascent) {
            nascent.test_type(species);
            nascent.forename(args[0]);
          },
          [&args,&species = species](nascent_behavioural_test& nascent) {
            nascent.test_type(species);
            nascent.header(args[0]);
          }
        },
        nascent);

      nascent_tests.emplace_back(std::move(nascent));
    }
  }

  [[nodiscard]]
  std::string report_time(const test_family::summary& s)
  {
    return report_time(s.log, s.execution_time);
  }

  //=========================================== test_runner ===========================================//



  test_runner::test_runner(int argc, char** argv, std::string copyright, project_paths paths, std::string codeIndent, std::ostream& stream)
    : m_Copyright{std::move(copyright)}
    , m_Selector{std::move(paths)}
    , m_CodeIndent{std::move(codeIndent)}
    , m_Stream{&stream}
  {
    check_indent(m_CodeIndent);

    process_args(argc, argv);

    fs::create_directory(proj_paths().output());
    fs::create_directory(diagnostics_output_path(proj_paths().output()));
    fs::create_directory(test_summaries_path(proj_paths().output()));
  }

  void test_runner::process_args(int argc, char** argv)
  {
    using namespace parsing::commandline;

    std::vector<nascent_test_vessel> nascentTests{};
    std::vector<project_data> nascentProjects{};

    const option familyOption{"--family", {"-f"}, {"family"},
      [&nascentTests](const arg_list& args){
        if(nascentTests.empty())
          throw std::logic_error{"Unable to find nascent test"};

        std::visit(variant_visitor{[&args](auto& nascent){ nascent.family(args[0]);}}, nascentTests.back());
      }
    };

    const option equivOption{"--equivalent-type", {"-e"}, {"equivalent_type"},
      [&nascentTests](const arg_list& args){
        if(nascentTests.empty())
          throw std::logic_error{"Unable to find nascent test"};

        auto visitor{
          variant_visitor{
            [&args](nascent_semantics_test& nascent){ nascent.add_equivalent_type(args[0]); },
            [](auto&){}
          }
        };

        std::visit(visitor, nascentTests.back());
      }
    };

    const option headerOption{"--class-header", {"-ch"}, {"header of class to test"},
      [&nascentTests](const arg_list& args){
        if(nascentTests.empty())
          throw std::logic_error{"Unable to find nascent test"};

        std::visit(variant_visitor{[&args](auto& nascent){ nascent.header(args[0]); }}, nascentTests.back());
      }
    };

    const option nameOption{"--forename", {"-name"}, {"forename"},
      [&nascentTests](const arg_list& args){
        if(nascentTests.empty())
          throw std::logic_error{"Unable to find nascent test"};

        std::visit(variant_visitor{[&args](auto& nascent){ nascent.forename(args[0]); }}, nascentTests.back());
      }
    };

    const option genFreeSourceOption{"gen-source", {"g"}, {"namespace"},
      [&nascentTests](const arg_list& args) {
        if(nascentTests.empty())
          throw std::logic_error{"Unable to find nascent test"};

        using src_opt = nascent_test_base::gen_source_option;

        auto visitor{
          variant_visitor{
            [&args](nascent_behavioural_test& nascent) {
              nascent.generate_source_files(src_opt::yes);
              if(args[0] != "::") nascent.set_namespace(args[0]);
            },
            [](auto&) {}
          }
        };

        std::visit(visitor, nascentTests.back());
      }
    };

    const option genSemanticsSourceOption{"gen-source", {"g"}, {"source dir"},
      [&nascentTests](const arg_list& args) {
        if(nascentTests.empty())
          throw std::logic_error{"Unable to find nascent test"};

        using src_opt = nascent_test_base::gen_source_option;

        auto visitor{
          variant_visitor{
            [&args](nascent_semantics_test& nascent) {
              nascent.generate_source_files(src_opt::yes);
              nascent.source_dir(args[0]);
            },
            [](auto&) {}
          }
        };

        std::visit(visitor, nascentTests.back());
      }
    };

    const std::vector<option> semanticsOptions{equivOption, familyOption, headerOption, genSemanticsSourceOption};
    const std::vector<option> allocationOptions{familyOption, headerOption};

    const auto help{
      parse_invoke_depth_first(argc, argv,
                { {"test", {"t"}, {"test family name"},
                    [this](const arg_list& args) {
                      m_RunnerMode |= runner_mode::test;
                      m_Selector.select_family(args.front());
                    }
                  },
                  {"select", {"s"}, {"source file name"},
                    [this](const arg_list& args) {
                      m_RunnerMode |= runner_mode::test;
                      m_Selector.select_source_file(fs::path{args.front()});
                    }
                  },
                  {"prune", {"p"}, {},
                    [this](const arg_list&) {
                      m_RunnerMode |= runner_mode::test;
                      m_Selector.enable_prune();
                    },
                    {{"--cutoff", {"-c"}, {"Cutoff for #include search e.g. 'namespace'"},
                      [this](const arg_list& args) {
                        m_Selector.set_prune_cutoff(args[0]);
                      }}
                    }
                  },
                  {"create", {"c"}, {}, [](const arg_list&) {},
                   { {"regular_test", {"regular"}, {"qualified::class_name<class T>", "equivalent type"},
                      nascent_test_data{"semantic", "regular", *this, nascentTests}, semanticsOptions
                     },
                     {"move_only_test", {"move_only"}, {"qualified::class_name<class T>", "equivalent type"},
                      nascent_test_data{"semantic", "move_only", *this, nascentTests}, semanticsOptions
                     },
                     {"regular_allocation_test", {"regular_allocation", "allocation_test"}, {"raw class name"},
                      nascent_test_data{"allocation", "regular_allocation", *this, nascentTests}, allocationOptions
                     },
                     {"move_only_allocation_test", {"move_only_allocation"}, {"raw class name"},
                      nascent_test_data{"allocation", "move_only_allocation", *this, nascentTests}, allocationOptions
                     },
                     {"free_test", {"free"}, {"header"},
                      nascent_test_data{"behavioural", "free", *this, nascentTests}, {familyOption, nameOption, genFreeSourceOption}
                     },
                     {"performance_test", {"performance"}, {"header"},
                       nascent_test_data{"behavioural", "performance", *this, nascentTests}, {familyOption}
                     }
                   },
                   [this,&nascentTests](const arg_list&) {
                      if(!nascentTests.empty())
                      {
                        m_RunnerMode |= runner_mode::create;
                        variant_visitor visitor{ [](auto& nascent) { nascent.finalize(); } };
                        std::visit(visitor, nascentTests.back());
                      }
                    }
                  },
                  {"init", {"i"}, {"copyright owner", "path ending with project name", "code indent"},
                    [this,&nascentProjects](const arg_list& args) {
                      m_RunnerMode |= runner_mode::init;

                      const auto ind{
                        [](std::string arg) {
                          
                          replace_all(arg, "\\t", "\t");
                          return indentation{arg};
                        }
                      };

                      nascentProjects.push_back(project_data{args[0], args[1], ind(args[2])});
                    },
                    { {"--no-build", {}, {},
                        [&nascentProjects](const arg_list&) { nascentProjects.back().do_build = build_invocation::no; }},
                      {"--to-files",  {}, {"filename (A file of this name will appear in multiple directories)"},
                        [&nascentProjects](const arg_list& args) { nascentProjects.back().output = args[0]; }},
                      {"--no-ide", {}, {},
                        [&nascentProjects](const arg_list&) {
                          auto& build{nascentProjects.back().do_build};
                          if(build == build_invocation::launch_ide) build = build_invocation::yes;
                        }
                      }
                    }
                  },
                  {"update-materials", {"u"}, {},
                    [this](const arg_list&) {
                      m_RunnerMode |= runner_mode::test;
                      m_UpdateMode = update_mode::soft;
                    }
                  },
                  {"dump", {}, {},
                    [this, recoveryDir{recovery_path(proj_paths().output())}](const arg_list&) {
                      std::filesystem::create_directory(recoveryDir);
                      m_Selector.dump_file(recoveryDir / "Dump.txt");
                      std::filesystem::remove(m_Selector.dump_file());
                    }
                  },
                  {"--async", {"-a"}, {},
                    [this](const arg_list&) {
                      if(m_ConcurrencyMode == concurrency_mode::serial)
                        m_ConcurrencyMode = concurrency_mode::dynamic;
                    }
                  },
                  {"--async-depth", {"-ad"}, {"depth [0,1]"},
                    [this](const arg_list& args) {
                      const int i{std::clamp(std::stoi(args.front()), 0, 1)};
                      m_ConcurrencyMode = i ? concurrency_mode::test : concurrency_mode::family;
                    }
                  },
                  {"--verbose",  {"-v"}, {}, [this](const arg_list&) { m_OutputMode = output_mode::verbose; }},
                  {"--recovery", {"-r"}, {},
                    [this,recoveryDir{recovery_path(proj_paths().output())}] (const arg_list&) {
                      std::filesystem::create_directory(recoveryDir);
                      m_Selector.recovery_file(recoveryDir / "Recovery.txt");
                      std::filesystem::remove(m_Selector.recovery_file());
                    }
                  }
                },
                [this](std::string_view exe){
                  const auto exePath{
                    [exe]() -> fs::path {
                      const fs::path attempt{exe};
                      if(attempt.is_absolute())
                      {
                        return attempt;
                      }

                      return working_path_v / attempt;
                    }()
                  };

                  m_Selector.executable_time_stamp(exePath);
                })
        };

    if(!help.empty())
    {
      m_RunnerMode &= runner_mode::help;
      stream() << help;
    }
    else
    {
      if(m_RunnerMode == runner_mode::none)
        m_RunnerMode = runner_mode::test;

      check_argument_consistency();

      if(mode(runner_mode::create))
        cmake_nascent_tests(proj_paths().main_cpp_dir(), proj_paths().cmade_build_dir(), stream());
  
      if(mode(runner_mode::init))
        init_projects(proj_paths().project_root(), nascentProjects, stream());

      if(mode(runner_mode::test))
        m_Selector.prune(stream());
    }
  }

  void test_runner::check_argument_consistency()
  {
    stream() << m_Selector.check_argument_consistency(m_ConcurrencyMode);
  }

  [[nodiscard]]
  test_family::summary test_runner::process_family(const test_family::results& results)
  {
    test_family::summary familySummary{results.execution_time};
    std::string output{};
    const auto detail{summary_detail::failure_messages | summary_detail::timings};
    for(const auto& s : results.logs)
    {
      if(m_OutputMode == output_mode::verbose) output += summarize(s, detail, tab, tab);
      familySummary.log += s;
    }

    if(m_OutputMode == output_mode::verbose)
    {
      output.insert(0, report_time(familySummary));
    }
    else
    {
      output += summarize(familySummary, detail, tab, no_indent);
    }

    stream() << output;

    std::copy(results.failed_tests.begin(), results.failed_tests.end(), std::back_inserter(m_FailedTestSourceFiles));

    return familySummary;
  }

  void test_runner::execute([[maybe_unused]] timer_resolution r)
  {
    if(!mode(runner_mode::help))
    {
      run_tests();
    }
  }

  void test_runner::finalize_concurrency_mode()
  {
    if(m_ConcurrencyMode == concurrency_mode::dynamic)
    {
      m_ConcurrencyMode = (m_Selector.size() < 4) ? concurrency_mode::test : concurrency_mode::family;
    }
  }

  void test_runner::run_tests()
  {
    using namespace std::chrono;
    const auto time{steady_clock::now()};

    finalize_concurrency_mode();

    if(mode(runner_mode::test))
    {
      log_summary summary{};
      if(!m_Selector.empty())
      {
        stream() << "\nRunning tests...\n\n";
        if(!concurrent_execution())
        {
          for(auto& family : m_Selector)
          {
            stream() << family.name() << ":\n";
            summary += process_family(family.execute(m_UpdateMode, m_ConcurrencyMode)).log;
          }
        }
        else
        {
          stream() << "\n\t--Using asynchronous execution, level: " << to_string(m_ConcurrencyMode) << "\n\n";
          std::vector<std::pair<std::string, std::future<test_family::results>>> results{};
          results.reserve(m_Selector.size());

          for(auto& family : m_Selector)
          {
            results.emplace_back(family.name(),
              std::async([&family, umode{m_UpdateMode}, cmode{m_ConcurrencyMode}](){
              return family.execute(umode, cmode); }));
          }

          for(auto& res : results)
          {
            stream() << res.first << ":\n";
            summary += process_family(res.second.get()).log;
          }
        }
        stream() << "\n-----------Grand Totals-----------\n";
        stream() << summarize(summary, steady_clock::now() - time, summary_detail::absent_checks | summary_detail::timings, indentation{"\t"}, no_indent);
      }
      else if(m_Selector.pruned())
      {
        stream() << "Nothing to do: no changes since the last run, therefore 'prune' has pruned all tests\n";
      }
      else if(!m_Selector.bespoke_selection())
      {
        stream() << "Nothing to do; try creating some tests!\nRun with --help to see options\n";
      }

      m_Selector.update_prune_info(m_FailedTestSourceFiles);
    }

    stream() << m_Selector.check_for_missing_tests();
  }
}
