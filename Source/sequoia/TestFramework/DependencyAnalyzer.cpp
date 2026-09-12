////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/TestFramework/DependencyAnalyzer.hpp"
#include "sequoia/TestFramework/FileSystemUtilities.hpp"

#include "sequoia/Maths/Arithmetic/ArithmeticCasts.hpp"
#include "sequoia/Maths/Graph/DynamicGraph.hpp"
#include "sequoia/Maths/Graph/GraphTraversalFunctions.hpp"
#include "sequoia/Streaming/Streaming.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <concepts>
#include <cstdint>
#include <format>
#include <fstream>
#include <map>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  using maths::checked_conversion_to;

  namespace
  {
    using duration_t   = prune_record::stamp_type::duration;
    using stream_rep_t = std::int64_t;
  }

  std::ostream& operator<<(std::ostream& s, const prune_record& record)
  {
    return s << "path: "      << record.test_path.generic_string() << '\n'
             << "timestamp: " << std::format("{}", checked_conversion_to<stream_rep_t>(record.time_stamp.time_since_epoch().count()));
  }

  std::istream& operator>>(std::istream& s, prune_record& record)
  {
    const auto extractField{
      [&s](std::string_view key) -> std::optional<std::string> {
        if(std::string line{}; std::getline(s, line) && line.starts_with(key))
          return line.substr(key.size());

        return std::nullopt;
      }
    };

    const auto toStamp{
      [](const std::string& text) -> std::optional<prune_record::stamp_type> {
        const auto last{text.data() + text.size()};
        if(stream_rep_t count{}; std::from_chars(text.data(), last, count) == std::from_chars_result{last, std::errc{}})
          return prune_record::stamp_type{duration_t{checked_conversion_to<duration_t::rep>(count)}};

        return std::nullopt;
      }
    };

    const auto parsed{
      extractField("path: ")
        .and_then([&](std::string path) {
          return extractField("timestamp: ")
                   .and_then(toStamp)
                   .transform([&path](prune_record::stamp_type stamp) { return prune_record{std::move(path), stamp}; });
        })
    };

    if(parsed) record = *parsed;
    else       s.setstate(std::ios::failbit);

    return s;
  }

  namespace
  {
    struct path_projector
    {
      const fs::path& operator()(const prune_record& record) const { return record.test_path; }
    };
    
    [[nodiscard]]
    bool is_stale(const fs::path& file, const fs::file_time_type& lastImplicitModTime, const fs::file_time_type& stalenessThreshold, const std::optional<fs::file_time_type>& exeTimeStamp)
    {
      if(exeTimeStamp.has_value() && (lastImplicitModTime >= exeTimeStamp.value()))
        throw std::runtime_error{
                std::format(
                  "Executable is out of date; please build it!\nExecutable time stamp: {}\n{} time stamp: {}\n",
                  exeTimeStamp.value(),
                  file.generic_string(),
                  lastImplicitModTime
                )
              };

      return lastImplicitModTime > stalenessThreshold;
    }

    struct file_info
    {
      file_info(fs::path f, const fs::file_time_type& stalenessThreshold, const std::optional<fs::file_time_type>& exeTimeStamp)
        : file{std::move(f)}
        , implicit_modification_time{fs::last_write_time(file)}
        , stale{is_stale(file, implicit_modification_time, stalenessThreshold, exeTimeStamp)}
      {}

      file_info(fs::path f)
        : file{std::move(f)}
      {}

      fs::path file;
      fs::file_time_type implicit_modification_time;
      bool stale{true};
    };

    [[nodiscard]]
    bool in_repo(const fs::path& file, const fs::path& repo)
    {
      auto zipped{std::views::zip(file, repo)};
      return std::ranges::find_if(zipped, [](const auto& e) { return std::get<0>(e) != std::get<1>(e); }) == zipped.end();
    }

    [[nodiscard]]
    bool is_cpp(const fs::path& file)
    {
      const auto ext{file.extension()};
      return (ext == ".cpp") || (ext == ".cc") || (ext == ".cxx");
    }

    [[nodiscard]]
    bool is_header(const fs::path& file)
    {
      const auto ext{file.extension()};
      return (ext == ".hpp") || (ext == ".h") || (ext == ".hxx");
    }

    /// Accumulates characters while `pred` holds, leaving the first which fails it unconsumed.
    template<std::predicate<char> Pred>
    std::string read_while(std::istream& istr, Pred pred)
    {
      constexpr auto eof{std::ifstream::traits_type::eof()};
      using int_type = std::ifstream::int_type;

      std::string str{};

      int_type c{};
      while((c = istr.get()) != eof)
      {
        if(!pred(static_cast<char>(c)))
        {
          istr.unget();
          break;
        }

        str.push_back(static_cast<char>(c));
      }

      return str;
    }

    /// Accumulates characters up to the first delimiter, which is consumed.
    [[nodiscard]]
    std::string read_until(std::istream& istr, std::string_view delimiters)
    {
      const std::string str{read_while(istr, [delimiters](char c){ return !std::ranges::contains(delimiters, c); })};
      istr.get();

      return str;
    }

    /// Whitespace which does not end a line: everything [cpp.pre] admits between a directive's tokens.
    [[nodiscard]]
    bool is_horizontal_space(char c) noexcept
    {
      return (c == ' ') || (c == '\t') || (c == '\v') || (c == '\f');
    }

    /// The characters which may appear in an identifier; ASCII, and so independent of the locale.
    [[nodiscard]]
    bool is_identifier_char(char c) noexcept
    {
      return    ((c >= 'a') && (c <= 'z'))
             || ((c >= 'A') && (c <= 'Z'))
             || ((c >= '0') && (c <= '9'))
             || (c == '_');
    }

    void skip_horizontal_space(std::istream& istr)
    {
      read_while(istr, [](char c){ return is_horizontal_space(c); });
    }

    /// Reads an identifier, leaving the first character which cannot extend one unconsumed.
    [[nodiscard]]
    std::string read_identifier(std::istream& istr)
    {
      return read_while(istr, [](char c){ return is_identifier_char(c); });
    }

    /** \brief Reads a delimited header name, or nothing if there is no opening delimiter or the
               line ends before the closing one does.

        A header name cannot cross a newline [lex.header], so a `"` or `<` left unclosed on its
        line opens none. The scan has no notion of a string literal, so without that bound
        `std::string_view include{"#include"};` opens a header name at its closing quote,
        consuming every `#include` up to the next quotation mark.
     */
    [[nodiscard]]
    std::string read_header_name(std::istream& istr)
    {
      const auto opening{istr.peek()};
      const char closing{(opening == '\"') ? '\"' : (opening == '<') ? '>' : '\0'};
      if(!closing) return {};

      istr.get();
      std::string name{read_while(istr, [closing](char c){ return (c != closing) && (c != '\n'); })};
      if(istr.peek() != closing) return {};

      istr.get();

      return name;
    }

    /** \brief Reads to the end of a declaration, returning its subject with all whitespace removed.

        A module name may be written `M : P` as readily as `M:P`, so the spaces cannot be kept if
        two spellings of one name are to compare equal.
     */
    [[nodiscard]]
    std::string declaration_subject(std::istream& istr)
    {
      auto subject{read_until(istr, ";\n")};
      std::erase_if(subject, [](char c){ return is_horizontal_space(c) || (c == '\r'); });

      return subject;
    }

    /** \brief Whether a string is spelled as a module name.

        Neither `module` nor `import` is a reserved word, so a line beginning with one may be
        ordinary code. Requiring what follows to look like a module name - identifier characters and
        dots, with at most one colon introducing a partition, and a leading colon permitted for a
        partition of the unit's own module - is what keeps `module = 3;` from declaring a module.
     */
    [[nodiscard]]
    bool is_module_name(std::string_view name)
    {
      const auto isIdentifier{
        [](std::string_view part) {
          return    !part.empty()
                 && !((part.front() >= '0') && (part.front() <= '9'))
                 && std::ranges::all_of(part, [](char c){ return is_identifier_char(c); });
        }
      };

      const auto isDottedName{
        [isIdentifier](std::string_view dotted) {
          return    !dotted.empty()
                 && std::ranges::all_of(std::views::split(dotted, '.'), [isIdentifier](const auto& part){ return isIdentifier(std::string_view{part}); });
        }
      };

      const auto colon{name.find(':')};
      if(colon == std::string_view::npos) return isDottedName(name);

      if(name.find(':', colon + 1) != std::string_view::npos) return false;

      // A leading colon abbreviates a partition of the importing unit's own module
      return    ((colon == 0) || isDottedName(name.substr(0, colon)))
             && isDottedName(name.substr(colon + 1));
    }

    /** \brief Consumes a module or import declaration, reporting whether the line was one.

        Neither `module` nor `import` is reserved, so a line which begins with one need not be a
        declaration. Returning false leaves the stream wherever the attempt reached and asks the
        caller to rewind, which is what allows the attempt to be made at all.
     */
    [[nodiscard]]
    bool consume_module_declaration(std::istream& istr, std::string_view keyword, module_role role, source_dependencies& dependencies)
    {
      if(keyword == "import")
      {
        skip_horizontal_space(istr);

        // A header unit is a dependency on a file, so it belongs with the includes
        if(const auto delimiter{istr.peek()}; (delimiter == '\"') || (delimiter == '<'))
        {
          fs::path header{read_header_name(istr)};
          if(header.empty()) return false;

          dependencies.includes.push_back(std::move(header));
          return true;
        }

        auto name{declaration_subject(istr)};
        if(!is_module_name(name)) return false;

        dependencies.imports.push_back(std::move(name));
        return true;
      }

      if(keyword == "module")
      {
        auto name{declaration_subject(istr)};

        // `module;` introduces the global module fragment and declares nothing; `export module;` is not a thing
        if(name.empty()) return role == module_role::implementation_unit;

        // A partition names the module it belongs to when it declares itself; only an import may abbreviate
        if(name.starts_with(':') || !is_module_name(name)) return false;

        dependencies.declaration = module_declaration{std::move(name), role};
        return true;
      }

      return false;
    }

    /// The extensions under which a module unit is conventionally written; what it declares is the lexer's to say.
    [[nodiscard]]
    bool has_module_extension(const fs::path& file)
    {
      const auto ext{file.extension()};
      return (ext == ".cppm") || (ext == ".ixx") || (ext == ".cxxm") || (ext == ".ccm") || (ext == ".c++m") || (ext == ".mpp");
    }

    /** \brief Whether other units can name what this one declares.

        An interface unit provides its module or partition; a partition provides its qualified name
        whether exported or not, since `import :P;` from within the module reaches either.
        `module M;` provides nothing: no unit can name it.
     */
    [[nodiscard]]
    bool provides(const module_declaration& declaration) noexcept
    {
      return (declaration.role == module_role::interface_unit) || declaration.name.contains(':');
    }

    /// Header names as written and module names as declared; nothing here is a path in the tree.
    void write_external_dependencies(const fs::path& file, std::span<const std::string> names)
    {
      if(std::ofstream ostream{file})
      {
        for(const auto& name : names) ostream << name << '\n';
      }
    }

    /// scan_dependencies, with the include names resolved against the file which wrote them.
    [[nodiscard]]
    source_dependencies get_dependencies(const fs::path& file, std::string_view cutoff)
    {
      source_dependencies dependencies{};

      // Binary, since the scan rewinds through `tellg`, which MSVC's text mode does not round-trip on LF files
      if(std::ifstream ifile{file, std::ios_base::binary})
      {
        auto scanned{scan_dependencies(ifile, cutoff)};

        for(auto& includedFile : scanned.includes)
        {
          // An extensionless name is a standard library header, which is never in the tree
          if(!includedFile.has_extension()) continue;

          if(includedFile.parent_path().empty())
          {
            // Maybe check if this file actually exists... if path is absolute
            includedFile = file.parent_path() / includedFile;
          }

          dependencies.includes.push_back(std::move(includedFile));
        }

        dependencies.imports     = std::move(scanned.imports);
        dependencies.declaration = std::move(scanned.declaration);
      }

      return dependencies;
    }

    using tests_dependency_graph = maths::directed_graph<maths::null_weight, file_info>;
    using node_iterator = tests_dependency_graph::iterator;

    void add_files(std::vector<file_info>& info, const fs::path& repo, const fs::file_time_type& stalenessThreshold, const std::optional<fs::file_time_type>& exeTimeStamp)
    {
      for(const auto& entry : fs::recursive_directory_iterator(repo))
      {
        const auto file{entry.path()};
        if(is_cpp(file) || is_header(file) || has_module_extension(file))
        {
          info.emplace_back(file, stalenessThreshold, exeTimeStamp);
        }
      }
    }

    /// pre-condition: the nodes of g have been sorted by file path
    void build_dependencies(tests_dependency_graph& g, const project_paths& projPaths, std::string_view cutoff)
    {
      using size_type = tests_dependency_graph::size_type;
      std::vector<std::string> externalDependencies{};

      std::vector<source_dependencies> scanned{};
      for(const auto& weight : g.cnode_weights())
      {
        scanned.push_back(get_dependencies(weight.file, cutoff));
      }

      // A partition is keyed on its qualified name, `M:P`, which is what `import :P;` from within M abbreviates
      std::map<std::string, size_type> providers{};
      for(size_type pos{}; pos != scanned.size(); ++pos)
      {
        if(const auto& declaration{scanned[pos].declaration}; declaration && provides(*declaration))
        {
          providers.emplace(declaration->name, pos);
        }
      }

      for(auto i{g.begin_node_weights()}; i != g.end_node_weights(); ++i)
      {
        const auto nodePos{static_cast<size_type>(std::ranges::distance(g.begin_node_weights(), i))};
        const auto& file{i->file};
        const auto& dependencies{scanned[nodePos]};

        for(const auto& includedFile : dependencies.includes)
        {
          if(auto eqrange{std::ranges::equal_range(g.node_weights(), includedFile.filename(), std::ranges::less{}, [](const file_info& weight){ return weight.file.filename(); })}; !eqrange.empty())
          {
            auto found{
              std::ranges::find_if(eqrange, [&includedFile,&projPaths,&file](const file_info& wt){
                  if(includedFile.is_absolute())
                  {
                    if(wt.file == includedFile) return true;
                  }
                  else
                  {
                    if(    (wt.file == (projPaths.source().repo() / includedFile))
                        || (wt.file == (projPaths.tests().repo() / includedFile))
                        || std::ranges::contains(projPaths.additional_dependency_analysis_paths(), wt.file, [&includedFile](const fs::path& p) {  return  p / includedFile; })
                      )
                      return true;

                    if(const auto trial{file.parent_path() / includedFile}; fs::exists(trial) && (wt.file == fs::canonical(trial)))
                      return true;
                  }

                  return false;
                }
              )
            };

            if(found != eqrange.end())
            {
              const auto includeNodePos{static_cast<size_type>(std::ranges::distance(g.begin_node_weights(), found))};
              g.join(nodePos, includeNodePos);

              if(is_cpp(file))
              {
                if(file.stem() == includedFile.stem())
                {
                  // Ensure that if cpp is stale, then its associated hpp is
                  // also rendered stale
                  if(i->stale) found->stale = true;

                  found->implicit_modification_time = std::ranges::max(i->implicit_modification_time, found->implicit_modification_time);
                }
                else
                {
                  /* Furnish the associated header with the same dependencies, as these are what
                     ultimately determine whether the test cpp is considered stale. Sorting of g
                     puts the header among the nodes following the source with the same stem; a
                     module unit sharing the stem may sit between them, so each is inspected.
                  */
                  for(auto next{std::ranges::next(i)}; (next != g.end_node_weights()) && (next->file.stem() == file.stem()); ++next)
                  {
                    if(is_header(next->file))
                    {
                      const auto nextPos{static_cast<size_type>(std::ranges::distance(g.begin_node_weights(), next))};
                      g.join(nextPos, includeNodePos);
                    }
                  }
                }
              }
            }
          }
          else
          {
            externalDependencies.push_back(includedFile.generic_string());
          }
        }

        auto joinToProvider{
          [&g, &providers, &externalDependencies, nodePos](const std::string& moduleName) {
            if(const auto found{providers.find(moduleName)}; found != providers.end())
              g.join(nodePos, found->second);
            else
              externalDependencies.push_back(moduleName);
          }
        };

        for(const auto& imported : dependencies.imports)
        {
          if(!imported.starts_with(':'))
          {
            joinToProvider(imported);
          }
          else if(dependencies.declaration)
          {
            // `import :P;` is only meaningful from within the module which owns P
            joinToProvider(std::string{dependencies.declaration->primary_name()} + imported);
          }
        }

        if(const auto& declaration{dependencies.declaration}; declaration)
        {
          if(const auto found{providers.find(std::string{declaration->primary_name()})};
             (found != providers.end()) && (found->second != nodePos))
          {
            /* Every unit of a module is linked into whatever imports it, yet importers name only
               the primary interface, so a change to any other unit must render the interface stale
               - the same trade the header path makes when it renders a header stale for its
               same-stem source. The unit which is `module M;` also depends on the interface, which
               it imports implicitly; the resulting two-cycle is why the staleness fold iterates to
               a fixed point.
            */
            g.join(found->second, nodePos);

            if(!declaration->name.contains(':')) g.join(nodePos, found->second);
          }
        }
      }

      std::ranges::sort(externalDependencies);
      auto iters{std::ranges::unique(externalDependencies)};
      externalDependencies.erase(iters.begin(), iters.end());

      write_external_dependencies(projPaths.prune().external_dependencies(), externalDependencies);
    }

    [[nodiscard]]
    bool materials_modified(const fs::path& relFilePath,
                            const fs::path& materialsRepo,
                            const fs::file_time_type stalenessThreshold)
    {
      const auto materials{materialsRepo / fs::path{relFilePath}.replace_extension("")};
      if(fs::exists(materials))
      {
        for(const auto& entry : fs::recursive_directory_iterator(materials))
        {
          if(fs::last_write_time(entry) > stalenessThreshold) return true;
        }
      }

      return false;
    }

    [[nodiscard]]
    std::optional<fs::file_time_type> materials_max_write_time(const fs::path& relFilePath, const fs::path& materialsRepo)
    {
      const auto materials{materialsRepo / fs::path{relFilePath}.replace_extension("")};
      if(fs::exists(materials))
      {
        fs::file_time_type maxTime{fs::last_write_time(materials)};

        for(const auto& entry : fs::recursive_directory_iterator(materials))
        {
          maxTime = std::ranges::max(maxTime, fs::last_write_time(entry));
        }

        return maxTime;
      }

      return std::nullopt;
    }

    void consider_passing_tests(node_iterator i,
                                const fs::path& relFilePath,
                                std::span<const prune_record> passingTests,
                                fs::file_time_type maxModificationTime)
    {
      auto iter{std::ranges::lower_bound(passingTests, relFilePath, {}, path_projector{})};
      if((iter != passingTests.end()) && (iter->test_path == relFilePath) && (iter->time_stamp > maxModificationTime))
      {
        i->stale = false;
      }
    }

    [[nodiscard]]
    std::optional<fs::file_time_type> get_stamp(const fs::path& file)
    {
      if(fs::exists(file)) return fs::last_write_time(file);

      return std::nullopt;
    }

    [[nodiscard]]
    std::vector<fs::path> find_stale_tests(fs::file_time_type stalenessThreshold, const project_paths& projPaths, std::string_view cutoff)
    {
      using namespace maths;

      tests_dependency_graph g{};

      const auto exeTimeStamp{get_stamp(projPaths.executable())};
      std::vector<file_info> files{};

      add_files(files, projPaths.source().repo(), stalenessThreshold, exeTimeStamp);
      add_files(files, projPaths.tests().repo(), stalenessThreshold, exeTimeStamp);
      for(const auto& p : projPaths.additional_dependency_analysis_paths())
      {
        add_files(files, p, stalenessThreshold, exeTimeStamp);
      }

      std::ranges::sort(
        files,
        [](const auto& lhs, const auto& rhs) {
          const fs::path& lfile{lhs.file}, rfile{rhs.file};

          const fs::path
            lname{lfile.filename()},
            rname{rfile.filename()};

          return lname != rname ? lname < rname : lfile < rfile;
        }
      );

      for(const auto& info : files)
      {
        g.add_node(info);
      }

      build_dependencies(g, projPaths, cutoff);

      bool changed{};

      auto nodesLate{
        [&g, &changed](const std::size_t node) {
          auto& wt{g.begin_node_weights()[node]};

          for(const auto& edge : g.cedges(node))
          {
            const auto& targetWt{g.cbegin_node_weights()[edge.target_node()]};

            if(targetWt.implicit_modification_time > wt.implicit_modification_time)
            {
              wt.implicit_modification_time = targetWt.implicit_modification_time;
              changed = true;
            }

            if(targetWt.stale && !wt.stale)
            {
              wt.stale = true;
              changed = true;
            }
          }
        }
      };

      /* A single post-order pass is exact only on a DAG, and headers may include one another.
         Rejecting a cycle is not an option, since mutual inclusion is legal, so the fold is
         repeated until it changes nothing: it only ever sets a flag or advances a time, and so
         reaches its fixed point - stale if anything reachable is stale, and the newest time
         among them. On a DAG that is one pass to do the work and one to confirm it.
       */
      do
      {
        changed = false;
        traverse(depth_first, g, find_disconnected_t{0}, null_func_obj{}, nodesLate);
      } while(changed);

      const auto passesFile{projPaths.prune().selected_passes(std::nullopt)};
      const auto passingTestsFromFile{read_tests(passesFile)};
      const auto passesStamp{get_stamp(passesFile)};

      std::vector<fs::path> staleTests{};

      for(auto i{g.begin_node_weights()}; i != g.end_node_weights(); ++i)
      {
        if(const auto& weight{*i}; is_cpp(weight.file) && in_repo(weight.file, projPaths.tests().repo()))
        {
          const auto relPath{fs::relative(weight.file, projPaths.tests().repo())};

          if(passesStamp && std::ranges::binary_search(passingTestsFromFile, relPath, {}, path_projector{}))
          {
            const auto materialsWriteTime{materials_max_write_time(relPath, projPaths.test_materials().repo())};
            if(!weight.stale && (materialsWriteTime > stalenessThreshold))
              i->stale = true;

            const auto maxModificationTime{materialsWriteTime ? std::ranges::max(materialsWriteTime.value(), weight.implicit_modification_time) : weight.implicit_modification_time};

            /* `>=`, not `>`: the question is whether the record of this test passing is older
               than the newest modification. Both sides come from `last_write_time`, and the record
               is written after the modification it supersedes, so wherever the filesystem can
               represent that difference the two spellings agree. Where it cannot - libstdc++
               truncates to whole seconds on macOS - "after" collapses onto "equal", and `>` then
               rejects a record which does post-date the change, so the test is selected again by
               every later run and never settles.
            */
            if(weight.stale && (passesStamp.value() >= maxModificationTime))
              consider_passing_tests(i, relPath, passingTestsFromFile, maxModificationTime);
          }
          else if(!weight.stale)
          {
            if(materials_modified(relPath, projPaths.test_materials().repo(), stalenessThreshold))
            {
              i->stale = true;
            }
          }

          if(weight.stale) staleTests.push_back(relPath);
        }
      }

      std::ranges::sort(staleTests);

      return staleTests;
    }

    void update_prune_stamp_on_disk(const prune_paths& prunePaths, fs::file_time_type time)
    {
      const auto stamp{prunePaths.stamp()};
      if(!fs::exists(stamp))
      {
        std::ofstream{stamp};
      }
      fs::last_write_time(stamp, time);
    }

    std::vector<prune_record>& read_tests_to(const fs::path& file, std::vector<prune_record>& tests)
    {
      
      if(std::ifstream ifile{file})
      {
        using iter_t = std::istream_iterator<prune_record>;
        // A call rather than a pipe, to stay identical to `modules-native`. The pipe is
        // fine here and is rejected there: under `import std`, g++ 15.2 reports
        // "use of operator| ... before deduction of 'auto'" whenever the adaptor carries a
        // lambda - a named predicate pipes fine. See gcc-bugs/E in the sequoia-LLM
        // repository, and PR 120318; fixed in gcc 16.1.
        tests.append_range(
            std::views::filter(std::ranges::subrange{iter_t{ifile}, iter_t{}},
                               [](const prune_record& record) {return !record.test_path.empty();})
        );
      }

      return tests;
    }

    struct least_path_most_recent{
      [[nodiscard]]
      bool operator()(const prune_record& lhs, const prune_record& rhs) const {
        auto comp{lhs.test_path <=> rhs.test_path};
        return comp == 0 ? lhs.time_stamp > rhs.time_stamp : comp < 0;
      }
    };

    std::vector<prune_record>& to_unique_range(std::vector<prune_record>& r) {
      auto erased{std::ranges::unique(r, {}, path_projector{})};
      r.erase(erased.begin(), erased.end());
      return r;
    }
    
    [[nodiscard]]
    std::vector<prune_record> aggregate_failures(const prune_paths& prunePaths, const std::size_t numReps)
    {
      std::vector<prune_record> allTests{};
      for(auto i : std::views::iota(0uz, numReps))
      {
        read_tests_to(prunePaths.failures(i), allTests);
      }

      return to_unique_range(allTests);
    }

    [[nodiscard]]
    std::optional<std::vector<prune_record>> aggregate_passes(const prune_paths& prunePaths, const std::size_t numReps)
    {
      std::vector<prune_record> intersection{};
      for(auto i : std::views::iota(0uz, numReps))
      {
        const auto file{prunePaths.selected_passes(i)};
        if(!fs::exists(file)) return std::nullopt;

        std::vector<prune_record> tests{testing::read_tests(file)};
        if(i)
        {
          std::vector<prune_record> currentIntersection{};
          std::ranges::set_intersection(tests, intersection, std::back_inserter(currentIntersection));
          intersection = std::move(currentIntersection);
        }
        else
        {
          intersection = std::move(tests);
        }
      }

      return intersection;
    }
  }

  [[nodiscard]]
  std::string_view module_declaration::primary_name() const noexcept
  {
    return std::string_view{name}.substr(0, name.find(':'));
  }

  [[nodiscard]]
  source_dependencies scan_dependencies(std::istream& source, std::string_view cutoff)
  {
    constexpr auto eof{std::ifstream::traits_type::eof()};
    using int_type = std::ifstream::int_type;

    source_dependencies dependencies{};

    /* A module or import declaration is recognized only at the start of a line, which is what keeps
       the words - neither of them reserved - from being found in the middle of ordinary code. Only
       horizontal whitespace may precede one, so the flag survives that and nothing else.
    */
    bool atLineStart{true};

    int_type c{};
    while((c = source.get()) != eof)
    {
      const bool lineStart{std::exchange(atLineStart, false)};

      if(lineStart && ((c == 'e') || (c == 'm') || (c == 'i')))
      {
        /* Only these three characters can begin `export`, `module` or `import`, and a line which
           begins with one of them and proves to be something else is rewound so that the scan sees
           it exactly as it would have done - the attempt is made ahead of the other branches so
           that the character then falls through to them, as a cutoff beginning `int` must.

           The order here is load-bearing: `tellg` on a `std::filebuf` discards the putback area, so
           an `unget` after it fails - silently, since nothing checks - where the same sequence on a
           `std::stringbuf` succeeds. Ungetting first and asking where we are second works on both.
        */
        source.unget();
        const auto rewind{source.tellg()};

        auto keyword{read_identifier(source)};
        auto role{module_role::implementation_unit};

        if(keyword == "export")
        {
          role = module_role::interface_unit;
          skip_horizontal_space(source);
          keyword = read_identifier(source);
        }

        if(consume_module_declaration(source, keyword, role, dependencies)) continue;

        source.clear();
        source.seekg(rewind);
        source.get();
      }

      if(is_horizontal_space(static_cast<char>(c)))
      {
        atLineStart = lineStart;
      }
      else if(c == '\n')
      {
        atLineStart = true;
      }
      else if(c == '/')
      {
        if(source.peek() == '/')
        {
          source.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
          atLineStart = true;
        }
        else if(source.peek() == '*')
        {
          source.get();
          while(source)
          {
            source.ignore(std::numeric_limits<std::streamsize>::max(), '*');
            if(source.peek() == '/')
            {
              source.get();
              break;
            }
          }
        }
      }
      else if(c == '#')
      {
        skip_horizontal_space(source);

        if(read_identifier(source) == "include")
        {
          skip_horizontal_space(source);

          if(fs::path includedFile{read_header_name(source)}; !includedFile.empty())
            dependencies.includes.push_back(std::move(includedFile));
        }
      }
      else if(!cutoff.empty() && (c == cutoff.front()))
      {
        source.unget();
        if(const std::string pattern{read_until(source, "\n")}; pattern.find(cutoff) != std::string::npos)
        {
          break;
        }

        atLineStart = true;
      }
    }

    return dependencies;
  }

  [[nodiscard]]
  fs::file_time_type staleness_threshold(const fs::file_time_type stamp)
  {
    using namespace std::chrono;
    const auto subSecond{stamp.time_since_epoch() - floor<seconds>(stamp.time_since_epoch())};

    return (subSecond == fs::file_time_type::duration::zero())
      ? stamp - duration_cast<fs::file_time_type::duration>(seconds{1})
      : stamp;
  }

  [[nodiscard]]
  std::vector<prune_record> read_tests(const fs::path& file)
  {
    std::vector<prune_record> tests{};
    return read_tests_to(file, tests);
  }

  void write_tests(const project_paths& projPaths, const fs::path& file, std::span<const prune_record> tests)
  {
    if(std::ofstream ostream{file})
    {
      for(const auto& test : tests)
      {
        ostream << prune_record{rebase_from(test.test_path, projPaths.tests().repo()), test.time_stamp} << "\n";
      }
    }
  }

  namespace
  {
    void do_update_prune_files(const project_paths& projPaths,
                          std::vector<prune_record> failedTests,
                          fs::file_time_type updateTime,
                          std::optional<std::size_t> id)
    {
      std::ranges::sort(failedTests, least_path_most_recent{});
      
      const auto prunePaths{projPaths.prune()};
      write_tests(projPaths, prunePaths.failures(id), failedTests);
      fs::remove(prunePaths.selected_passes(id));
      update_prune_stamp_on_disk(prunePaths, updateTime);
    }

    void do_update_prune_files(const project_paths& projPaths,
                               std::vector<prune_record> executedTests,
                               std::vector<prune_record> failedTests,
                               std::optional<std::size_t> id)
    {
      std::ranges::sort(executedTests, least_path_most_recent{});
      std::ranges::sort(failedTests, least_path_most_recent{});
      to_unique_range(executedTests);

      auto unionize{
        [](std::span<const prune_record> a, std::span<const prune_record> b){
          std::vector<prune_record> tests{};
          std::ranges::set_union(a, b, std::back_inserter(tests), least_path_most_recent{});
          to_unique_range(tests);
          return tests;
        }
      };

      auto difference{
        [](std::span<const prune_record> a, std::span<const prune_record> b){
          std::vector<prune_record> tests{};
          std::ranges::set_difference(a, b, std::back_inserter(tests), {}, path_projector{}, path_projector{});
          return tests;
        }
      };

      const auto prunePaths{projPaths.prune()};
      const auto passesFile{prunePaths.selected_passes(id)},
                 failuresFile{prunePaths.failures(id)};
      
      const std::vector<prune_record> trialPasses{unionize(executedTests, read_tests(passesFile))};      
      const std::vector<prune_record> passingTests{difference(trialPasses, failedTests)};
      const std::vector<prune_record> remainingPreviousFailures{difference(read_tests(failuresFile), passingTests)};
      const std::vector<prune_record> allFailures{unionize(remainingPreviousFailures, failedTests)};

      write_tests(projPaths, failuresFile, allFailures);
      write_tests(projPaths, passesFile, passingTests);
    }

    [[nodiscard]]
    std::vector<prune_record> build_prune_records(std::span<const fs::path> tests, fs::file_time_type updateTime) {
      return   std::views::transform(tests, [updateTime](const fs::path& p){ return prune_record{p, updateTime}; })
             | std::ranges::to<std::vector>();
    }
  }

  [[nodiscard]]
  std::optional<std::vector<fs::path>>
  tests_to_run(const project_paths& projPaths, std::string_view cutoff)
  {
    const auto prunePaths{projPaths.prune()};
    const auto pruneTimeStamp{get_stamp(prunePaths.stamp())};

    if(!pruneTimeStamp) return std::nullopt;

    const auto staleTests{find_stale_tests(staleness_threshold(pruneTimeStamp.value()), projPaths, cutoff)};

    const std::vector<fs::path> failingTests{
      std::views::transform(read_tests(prunePaths.failures(std::nullopt)), path_projector{}) | std::ranges::to<std::vector>()
    };

    std::vector<fs::path> testsToRun{};
    std::ranges::set_union(staleTests, failingTests, std::back_inserter(testsToRun));

    return testsToRun;
  }

  void update_prune_files(const project_paths& projPaths,
                          std::span<const fs::path> failedTests,
                          fs::file_time_type updateTime,
                          std::optional<std::size_t> id)
  {
    do_update_prune_files(
      projPaths,
      build_prune_records(failedTests, updateTime),
      updateTime,
      id
    );
  }

  void update_prune_files(const project_paths& projPaths,
                          std::span<const fs::path> executedTests,
                          std::span<const fs::path> failedTests,
                          fs::file_time_type updateTime,
                          std::optional<std::size_t> id)
  {    
    do_update_prune_files(
      projPaths,
      build_prune_records(executedTests, updateTime),
      build_prune_records(failedTests, updateTime),
      id
    );
  }

  void setup_instability_analysis_prune_folder(const project_paths& projPaths)
  {
    const auto dir{projPaths.prune().instability_analysis()};
    fs::remove_all(dir);
    fs::create_directories(dir);
  }

  void aggregate_instability_analysis_prune_files(const project_paths& projPaths, prune_mode mode, std::filesystem::file_time_type timeStamp, std::size_t numReps)
  {
    const auto prunePaths{projPaths.prune()};
    auto failingCases{aggregate_failures(prunePaths, numReps)};

    switch(mode)
    {
    case prune_mode::passive:
    {
      if(auto optPasses{aggregate_passes(prunePaths, numReps)})
      {
        auto& executedCases{optPasses.value()};
        executedCases.insert(executedCases.end(), failingCases.begin(), failingCases.end());

        do_update_prune_files(projPaths, std::move(executedCases), std::move(failingCases), std::nullopt);
      }
      else
      {
        do_update_prune_files(projPaths, std::move(failingCases), timeStamp, std::nullopt);
      }

      break;
    }
    case prune_mode::active:
    {
      do_update_prune_files(projPaths, std::move(failingCases), timeStamp, std::nullopt);
      break;
    }
    }

    fs::remove_all(prunePaths.instability_analysis());
  }
}
