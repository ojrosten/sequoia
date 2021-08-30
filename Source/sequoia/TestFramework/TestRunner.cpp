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

#include "sequoia/TestFramework/ProjectCreator.hpp"

#include "sequoia/Parsing/CommandLineArguments.hpp"
#include "sequoia/TestFramework/Summary.hpp"
#include "sequoia/TestFramework/FileEditors.hpp"
#include "sequoia/TestFramework/DependencyAnalyzer.hpp"
#include "sequoia/TextProcessing/Substitutions.hpp"

#include <fstream>

// TO DO: remove once libc++ supports <format>
#ifdef _MSC_VER
  #include <format>
#endif 

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    void set_proj_name(std::string& text, const std::filesystem::path& projRoot)
    {
      if(projRoot.empty()) return;

      const auto name{(--projRoot.end())->generic_string()};
      const std::string myProj{"MyProject"}, projName{replace_all(name, " ", "_")};
      replace_all(text, myProj, projName);
    }

    void check_indent(const indentation& ind)
    {
      if(std::string_view{ind}.find_first_not_of("\t ") != std::string::npos)
        throw std::runtime_error{"Code indent must comprise only spaces or tabs"};
    }

    [[nodiscard]]
    bool is_appropriate_root(const fs::path& root)
    {
      if(fs::exists(root))
      {
        for(auto& entry : fs::directory_iterator(root))
        {
          const auto& f{entry.path().filename()};
          if((f != ".DS_Store") && (f != ".keep")) return false;
        }
      }

      return true;
    }
  }

  [[nodiscard]]
  std::string report_time(const test_family::summary& s)
  {
    return report_time(s.log, s.execution_time);
  }

  //=========================================== test_runner ===========================================//

  void test_runner::nascent_test_data::operator()(const parsing::commandline::arg_list& args)
  {
    auto& nascentTests{runner.m_NascentTests};

    creation_factory factory{{"semantic", "allocation", "behavioural"}, runner.m_Paths, runner.m_Copyright, runner.m_CodeIndent, runner.stream()};
    auto nascent{factory.create(genus)};

    std::visit(
        variant_visitor{
          [&args,&species=species](nascent_semantics_test& nascent){
            nascent.test_type(species);
            nascent.qualified_name(args[0]);
            nascent.add_equivalent_type(args[1]);
          },
          [&args,&species=species](nascent_allocation_test& nascent){
            nascent.test_type(species);
            nascent.forename(args[0]);
          },
          [&args,&species=species](nascent_behavioural_test& nascent){
            nascent.test_type(species);
            nascent.header(args[0]);
          }
        },
        nascent);

    nascentTests.emplace_back(std::move(nascent));
  }

  auto test_runner::time_stamps::from_file(const std::filesystem::path& stampFile) -> stamp
  {
    if(fs::exists(stampFile))
    {
      return fs::last_write_time(stampFile);
    }

    return std::nullopt;
  }

  test_runner::test_runner(int argc, char** argv, std::string copyright, project_paths paths, std::string codeIndent, std::ostream& stream)
    : m_Copyright{std::move(copyright)}
    , m_Paths{std::move(paths)}
    , m_CodeIndent{std::move(codeIndent)}
    , m_Stream{&stream}
  {
    check_indent(m_CodeIndent);

    process_args(argc, argv);

    fs::create_directory(m_Paths.output());
    fs::create_directory(diagnostics_output_path(m_Paths.output()));
    fs::create_directory(test_summaries_path(m_Paths.output()));
  }

  void test_runner::process_args(int argc, char** argv)
  {

    using namespace parsing::commandline;

    const option familyOption{"--family", {"-f"}, {"family"},
      [this](const arg_list& args){
        if(m_NascentTests.empty())
          throw std::logic_error{"Unable to find nascent test"};

        std::visit(variant_visitor{[&args](auto& nascent){ nascent.family(args[0]);}}, m_NascentTests.back());
      }
    };

    const option equivOption{"--equivalent-type", {"-e"}, {"equivalent_type"},
      [this](const arg_list& args){
        if(m_NascentTests.empty())
          throw std::logic_error{"Unable to find nascent test"};

        auto visitor{
          variant_visitor{
            [&args](nascent_semantics_test& nascent){ nascent.add_equivalent_type(args[0]); },
            [](auto&){}
          }
        };

        std::visit(visitor, m_NascentTests.back());
      }
    };

    const option headerOption{"--class-header", {"-ch"}, {"header of class to test"},
      [this](const arg_list& args){
        if(m_NascentTests.empty())
          throw std::logic_error{"Unable to find nascent test"};

        std::visit(variant_visitor{[&args](auto& nascent){ nascent.header(args[0]); }}, m_NascentTests.back());
      }
    };

    const option nameOption{"--forename", {"-name"}, {"forename"},
      [this](const arg_list& args){
        if(m_NascentTests.empty())
          throw std::logic_error{"Unable to find nascent test"};

        std::visit(variant_visitor{[&args](auto& nascent){ nascent.forename(args[0]); }}, m_NascentTests.back());
      }
    };

    const option genFreeSourceOption{"gen-source", {"g"}, {"namespace"},
      [this](const arg_list& args) {
        if(m_NascentTests.empty())
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

        std::visit(visitor, m_NascentTests.back());
      }
    };

    const option genSemanticsSourceOption{"gen-source", {"g"}, {"source dir"},
      [this](const arg_list& args) {
        if(m_NascentTests.empty())
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

        std::visit(visitor, m_NascentTests.back());
      }
    };

    const std::vector<option> semanticsOptions{equivOption, familyOption, headerOption, genSemanticsSourceOption};

    const std::vector<option> allocationOptions{familyOption, headerOption};

    const auto help{
      parse_invoke_depth_first(argc, argv,
                { {"test", {"t"}, {"test family name"},
                    [this](const arg_list& args) { m_SelectedFamilies.emplace(args.front(), false); }
                  },
                  {"select", {"s"}, {"source file name"},
                    [this](const arg_list& args) {
                      m_SelectedSources.emplace_back(fs::path{args.front()}.lexically_normal(), false);
                    }
                  },
                  {"prune", {"p"}, {},
                    [this](const arg_list&) {
                      m_PruneInfo.stamps.ondisk = time_stamps::from_file(prune_path(m_Paths.output(), m_Paths.main_cpp_dir()));
                    },
                    {{"--cutoff", {"-c"}, {"Cutoff for #include search e.g. 'namespace'"},
                      [this](const arg_list& args) {
                        m_PruneInfo.include_cutoff = args[0];
                      }}
                    }
                  },
                  {"create", {"c"}, {}, [](const arg_list&) {},
                   { {"regular_test", {"regular"}, {"qualified::class_name<class T>", "equivalent type"},
                      nascent_test_data{"semantic", "regular", *this}, semanticsOptions
                     },
                     {"move_only_test", {"move_only"}, {"qualified::class_name<class T>", "equivalent type"},
                      nascent_test_data{"semantic", "move_only", *this}, semanticsOptions
                     },
                     {"regular_allocation_test", {"regular_allocation", "allocation_test"}, {"raw class name"},
                      nascent_test_data{"allocation", "regular_allocation", *this}, allocationOptions
                     },
                     {"move_only_allocation_test", {"move_only_allocation"}, {"raw class name"},
                      nascent_test_data{"allocation", "move_only_allocation", *this}, allocationOptions
                     },
                     {"free_test", {"free"}, {"header"},
                      nascent_test_data{"behavioural", "free", *this}, {familyOption, nameOption, genFreeSourceOption}
                     },
                     {"performance_test", {"performance"}, {"header"},
                       nascent_test_data{"behavioural", "performance", *this}, {familyOption}
                     }
                   },
                   [this](const arg_list&) {
                      if(!m_NascentTests.empty())
                      {
                        variant_visitor visitor{ [](auto& nascent) { nascent.finalize(); } };
                        std::visit(visitor, m_NascentTests.back());
                      }
                    }
                  },
                  {"init", {"i"}, {"copyright owner", "path ending with project name", "code indent"},
                    [this](const arg_list& args) {
                      const auto ind{
                        [](std::string arg) {
                          
                          replace_all(arg, "\\t", "\t");
                          return indentation{arg};
                        }
                      };

                      m_NascentProjects.push_back(project_data{args[0], args[1], ind(args[2])});
                    },
                    { {"--no-build", {}, {}, [this](const arg_list&) { m_NascentProjects.back().do_build = build_invocation::no; }},
                      {"--to-files",  {}, {"filename (A file of this name will appear in multiple directories)"}, 
                        [this](const arg_list& args) { m_NascentProjects.back().output = args[0]; }},
                      {"--no-ide", {}, {}, [this](const arg_list&) {
                          auto& build{m_NascentProjects.back().do_build};
                          if(build == build_invocation::launch_ide) build = build_invocation::yes;
                        }
                      }
                    }
                  },
                  {"update-materials", {"u"}, {},
                    [this](const arg_list&) { m_UpdateMode = update_mode::soft; }
                  },
                  {"dump", {}, {},
                    [this, recoveryDir{recovery_path(m_Paths.output())}](const arg_list&) {
                      std::filesystem::create_directory(recoveryDir);
                      m_Recovery.dump_file = recoveryDir / "Dump.txt";
                      std::filesystem::remove(m_Recovery.dump_file);
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
                  {"--verbose",  {"-v"}, {}, [this](const arg_list&) { m_OutputMode |= output_mode::verbose; }},
                  {"--recovery", {"-r"}, {},
                    [this,recoveryDir{recovery_path(m_Paths.output())}] (const arg_list&) {
                      std::filesystem::create_directory(recoveryDir);
                      m_Recovery.recovery_file = recoveryDir / "Recovery.txt";
                      std::filesystem::remove(m_Recovery.recovery_file);
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

                  if(fs::exists(exePath))
                    m_PruneInfo.stamps.executable = fs::last_write_time(exePath);
                })
        };

    if(!help.empty())
    {
      m_OutputMode |= output_mode::help;
      stream() << help;
    }
    else
    {
      check_argument_consistency();
    }
  }

  void test_runner::check_argument_consistency()
  {
    using parsing::commandline::error;
    using parsing::commandline::warning;

    if(concurrent_execution() && (!m_Recovery.recovery_file.empty() || !m_Recovery.dump_file.empty()))
      throw std::runtime_error{error("Can't run asynchronously in recovery/dump mode\n")};

    if(pruned())
    {
      if((!m_SelectedFamilies.empty() || !m_SelectedSources.empty()))
      {
        stream() << warning("'prune' ignored if either test families or test source files are specified");
        m_PruneInfo.stamps.ondisk = std::nullopt;
      }
      else
      {
        stream() << "\nAnalyzing dependencies...\n";
        const auto start{std::chrono::steady_clock::now()};

        if(auto maybeToRun{tests_to_run(m_Paths.source_root(), m_Paths.tests(), m_Paths.test_materials(), m_PruneInfo.stamps.ondisk, m_PruneInfo.stamps.executable, m_PruneInfo.include_cutoff)})
        {
          auto& toRun{maybeToRun.value()};

          if(std::ifstream ifile{prune_path(m_Paths.output(), m_Paths.main_cpp_dir())})
          {
            while(ifile)
            {
              fs::path source{};
              ifile >> source;
              toRun.push_back(source);
            }
          }

          std::sort(toRun.begin(), toRun.end());
          auto last{std::unique(toRun.begin(), toRun.end())};
          toRun.erase(last, toRun.end());

          std::transform(toRun.begin(), toRun.end(), std::back_inserter(m_SelectedSources),
            [](const fs::path& file) -> std::pair<fs::path, bool> { return {file, false}; });
        }

        const auto end{std::chrono::steady_clock::now()};

        const auto [dur, unit]{testing::stringify(end - start)};
        stream() << '[' << dur << unit << "]\n\n";
      }
    }
  }

  [[nodiscard]]
  std::string test_runner::stringify(concurrency_mode mode)
  {
    switch(mode)
    {
    case concurrency_mode::serial:
      return "Serial";
    case concurrency_mode::dynamic:
      return "Dynamic";
    case concurrency_mode::family:
      return "Family";
    case concurrency_mode::test:
      return "Test";
    }

    throw std::logic_error{"Unknown option for concurrency_mode"};
  }

  [[nodiscard]]
  bool test_runner::pruned() const noexcept
  {
    return m_PruneInfo.stamps.ondisk.has_value();
  }

  bool test_runner::mark_family(std::string_view name)
  {
    if(m_SelectedFamilies.empty()) return true;

    auto i{m_SelectedFamilies.find(name)};
    if(i != m_SelectedFamilies.end())
    {
      i->second = true;
      return true;
    }

    return false;
  }

  [[nodiscard]]
  test_family::summary test_runner::process_family(const test_family::results& results)
  {
    test_family::summary familySummary{results.execution_time};
    std::string output{};
    const auto detail{summary_detail::failure_messages | summary_detail::timings};
    for(const auto& s : results.logs)
    {
      if(mode(output_mode::verbose)) output += summarize(s, detail, tab, tab);
      familySummary.log += s;
    }

    if(mode(output_mode::verbose))
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

  namespace
  {
    const std::string& convert(const std::string& s) { return s; }
    std::string convert(const std::filesystem::path& p) { return p.generic_string(); }
  }

  void test_runner::check_for_missing_tests()
  {
    if(pruned()) return;

    auto check{
      [&stream=stream()](const auto& tests, std::string_view type, auto fn) {
        for(const auto& test : tests)
        {
          if(!test.second)
          {
            using namespace parsing::commandline;
            stream << warning(std::string{"Test "}.append(type)
                              .append(" '")
                              .append(convert(test.first))
                              .append("' not found\n")
                              .append(fn(test.first)));
          }
        }
      }
    };

    check(m_SelectedFamilies, "Family", [](const std::string& name) -> std::string {
        if(auto pos{name.rfind('.')}; pos < std::string::npos)
        {
          return "--If trying to select a source file use 'select' rather than 'test'\n";
        }

        return "";
      }
    );

    check(m_SelectedSources, "File", [](const std::filesystem::path& p) -> std::string {
        if(!p.has_extension())
        {
          return "--If trying to test a family use 'test' rather than 'select'\n";
        }
  
        return "";
      }
    );
  }

  [[nodiscard]]
  auto test_runner::find_filename(const std::filesystem::path& filename) -> source_list::iterator
  {
    return std::find_if(m_SelectedSources.begin(), m_SelectedSources.end(),
                 [&filename, &repo=m_Paths.tests(), &root=m_Paths.project_root()](const auto& element){
                   const auto& source{element.first};

                   if(filename.empty() || source.empty() || ((*--source.end()) != (*--filename.end())))
                     return false;

                   if(filename == source) return true;

                   if(filename.is_absolute())
                   {
                     if(rebase_from(filename, root) == rebase_from(source, working_path()))
                       return true;
                   }

                   // filename is relative to where compilation was performed which
                   // cannot be known here. Therefore fallback to assuming the 'selected sources'
                   // live in the test repository

                   if(!repo.empty())
                   {
                     if(rebase_from(source, repo) == rebase_from(filename, repo))
                       return true;

                     if(const auto path{find_in_tree(repo, source)}; !path.empty())
                     {
                       if(rebase_from(path, repo) == rebase_from(filename, repo))
                         return true;
                     }
                   }

                   return false;
                 });
  }

  void test_runner::execute([[maybe_unused]] timer_resolution r)
  {
    if(!mode(output_mode::help))
    {
      init_projects();
      create_tests();
      run_tests();
    }
  }

  void test_runner::create_tests()
  {
    using namespace runtime;
    if(!m_NascentTests.empty())
    {
      if(fs::exists(m_Paths.main_cpp_dir()) && fs::exists(m_Paths.cmade_build_dir()))
      {
        stream() << "\n";
        invoke(cd_cmd(m_Paths.main_cpp_dir()) && cmake_cmd(m_Paths.cmade_build_dir(), {}));
      }
    }
  }

  void test_runner::report(std::string_view prefix, std::string_view message)
  {
    if(!message.empty())
    {
      stream() << prefix << '\n';
      stream() << message << "\n\n";
    }
  }

  void test_runner::finalize_concurrency_mode()
  {
    if(m_ConcurrencyMode == concurrency_mode::dynamic)
    {
      m_ConcurrencyMode = (m_Families.size() < 4) ? concurrency_mode::test : concurrency_mode::family;
    }
  }

  void test_runner::run_tests()
  {
    using namespace std::chrono;
    const auto time{steady_clock::now()};

    finalize_concurrency_mode();

    const bool selected{!m_SelectedFamilies.empty() || !m_SelectedSources.empty()};
    if((m_NascentTests.empty() && m_NascentProjects.empty()) || selected)
    {
      log_summary summary{};
      if(!m_Families.empty())
      {
        stream() << "\nRunning tests...\n\n";
        if(!concurrent_execution())
        {
          for(auto& family : m_Families)
          {
            stream() << family.name() << ":\n";
            summary += process_family(family.execute(m_UpdateMode, m_ConcurrencyMode)).log;
          }
        }
        else
        {
          stream() << "\n\t--Using asynchronous execution, level: " << stringify(m_ConcurrencyMode) << "\n\n";
          std::vector<std::pair<std::string, std::future<test_family::results>>> results{};
          results.reserve(m_Families.size());

          for(auto& family : m_Families)
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
      else if(pruned())
      {
        stream() << "Nothing to do: no changes since the last run, therefore 'prune' has pruned all tests\n";
      }
      else if(!selected)
      {
        stream() << "Nothing to do; try creating some tests!\nRun with --help to see options\n";
      }


      if(!selected || pruned())
      {
        const auto stampFile{prune_path(m_Paths.output(), m_Paths.main_cpp_dir())};
        if(std::ofstream ostream{stampFile})
        {
          for(const auto& source : m_FailedTestSourceFiles)
          {
            ostream << source.generic_string() << "\n";
          }
        }

        fs::last_write_time(stampFile, m_PruneInfo.stamps.current);
      }
    }

    check_for_missing_tests();
  }

  void test_runner::init_projects()
  {
    if(m_NascentProjects.empty()) return;

    stream() << "Initializing Project(s)....\n\n";

    for(const auto& data : m_NascentProjects)
    {
      if(data.project_root.empty())
        throw std::runtime_error{"Project path should not be empty\n"};

      if(!data.project_root.is_absolute())
        throw std::runtime_error{std::string{"Project path '"}.append(data.project_root.generic_string()).append("' should be absolute\n")};

      if(!is_appropriate_root(data.project_root))
        throw std::runtime_error{std::string{"Project location '"}.append(data.project_root.generic_string()).append("' is in use\n")};

      const auto name{(--data.project_root.end())->generic_string()};
      if(name.empty())
        throw std::runtime_error{"Project name, deduced as the last token of path, is empty\n"};

      if(std::find_if(name.cbegin(), name.cend(), [](char c) { return !std::isalnum(c) || (c == '_') || (c == '-'); }) != name.cend())
      {
        throw std::runtime_error{std::string{"Please ensure the project name '"}
        .append(name)
        .append("' consists of just alpha-numeric characters, underscores and dashes\n")};
      }

      check_indent(data.code_indent);

      report("Creating new project at location:", data.project_root.generic_string());

      fs::create_directories(data.project_root);
      fs::copy(project_template_path(m_Paths.project_root()), data.project_root, fs::copy_options::recursive | fs::copy_options::skip_existing);
      fs::create_directory(project_paths::source_path(data.project_root));
      fs::copy(aux_files_path(m_Paths.project_root()), aux_files_path(data.project_root), fs::copy_options::recursive | fs::copy_options::skip_existing);

      generate_test_main(data.copyright, data.project_root, data.code_indent);
      generate_build_system_files(data.project_root);

      if(data.do_build != build_invocation::no)
      {
        const auto mainDir{data.project_root / "TestAll"};
        const auto buildDir{project_paths::cmade_build_dir(data.project_root, mainDir)};

        using namespace runtime;
        invoke(cd_cmd(mainDir)
            && cmake_cmd(buildDir, data.output)
            && build_cmd(buildDir, data.output)
            && git_first_cmd(data.project_root, data.output)
            && (data.do_build == build_invocation::launch_ide ? launch_cmd(data.project_root, buildDir) : shell_command{})
        );
      }
    }
  }

  void test_runner::generate_test_main(std::string_view copyright, const std::filesystem::path& projRoot, indentation codeIndent) const
  {
    auto modifier{
      [copyright, codeIndent](std::string& text) {

        set_top_copyright(text, copyright);

        tabs_to_spacing(text, codeIndent);
        replace(text, "Oliver J. Rosten", copyright);

        const auto indentReplacement{
          [&codeIndent]() {
            std::string replacement;
            for(auto c : std::string_view{codeIndent})
            {
              if(c == '\t') replacement.append("\\t");
              else          replacement.push_back(c);
            }

            return indentation{replacement};
          }
        };

        replace(text, "\\t", indentReplacement());
      }
    };

    read_modify_write(projRoot / "TestAll" / "TestAllMain.cpp", modifier);
  }

  void test_runner::generate_build_system_files(const std::filesystem::path& projRoot) const
  {
    if(projRoot.empty())
      throw std::logic_error{"Pre-condition violated: path should not be empty"};

    const std::filesystem::path relCmakeLocation{"TestAll/CMakeLists.txt"};

    auto setBuildSysPath{
      [&parentProjRoot=m_Paths.project_root(),&projRoot](std::string& text) {
        constexpr auto npos{std::string::npos};
        std::string_view token{"/build_system"};
        if(const auto pos{text.find(token)}; pos != npos)
        {
          std::string_view pattern{"BuildSystem "};
          if(auto left{text.rfind(pattern)}; left != npos)
          {
            left += pattern.size();
            if(pos >= left)
            {
              const auto count{pos - left + token.size()};
              const auto relPath{fs::relative(parentProjRoot / "TestAll" / text.substr(left, count), projRoot / "TestAll")};
              text.replace(left, count, relPath.lexically_normal().generic_string());
            }
          }
        }
      }
    };

    read_modify_write(projRoot / relCmakeLocation, [setBuildSysPath, &projRoot](std::string& text) {
        setBuildSysPath(text);
        set_proj_name(text, projRoot);
      }
    );

    read_modify_write(projRoot / "Source" / "CMakeLists.txt", [&projRoot](std::string& text) {
        set_proj_name(text, projRoot);
      }
    );

    read_modify_write(project_template_path(projRoot) / relCmakeLocation, [setBuildSysPath](std::string& text) {
        setBuildSysPath(text);
      }
    );
  }
}
