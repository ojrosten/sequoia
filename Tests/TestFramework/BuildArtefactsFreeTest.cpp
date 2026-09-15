////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "BuildArtefactsFreeTest.hpp"
#include "BuildArtefactsTestingUtilities.hpp"

#include "sequoia/Streaming/Streaming.hpp"

#include <cstring>
#include <format>
#include <fstream>
#include <stdexcept>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    /// The tracker's encoding: UTF-16, little-endian, with a byte order mark
    void write_utf16(const fs::path& file, std::u16string_view text)
    {
      std::ofstream out{file, std::ios_base::binary};
      out.write("\xFF\xFE", 2);
      for(const char16_t unit : text)
      {
        out.put(static_cast<char>(unit & 0xFF));
        out.put(static_cast<char>(unit >> 8));
      }
    }

    /// As the tracker spells a path, for the ASCII the fixtures use; a character beyond it is left as it is
    [[nodiscard]]
    std::u16string upper(const fs::path& p)
    {
      auto s{p.u16string()};
      std::ranges::transform(s, s.begin(), [](char16_t c){ return (c < 0x80) ? static_cast<char16_t>(std::toupper(static_cast<unsigned char>(c))) : c; });
      return s;
    }
  }

  [[nodiscard]]
  std::filesystem::path build_artefacts_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void build_artefacts_free_test::run_tests()
  {
    test_ninja_deps();
    test_tlogs();
    test_build_tree();
  }

  void build_artefacts_free_test::test_ninja_deps()
  {
    const auto scratch{working_materials()};

    {
      // Written by ninja 1.13 for a two-object build: a.o was compiled twice, the second time with one include fewer
      const auto records{read_ninja_deps(auxiliary_materials() / "superseded.ninja_deps")};
      check(equality,
            "A log ninja wrote: the later record for a.o supersedes the earlier, and c.o has one record",
            records,
            std::vector<compilation_record>{
              {"a.o", {"/Library/Developer/CommandLineTools/SDKs/MacOSX26.sdk/SDKSettings.json", "a.cpp", "a.h"}},
              {"c.o", {"/Library/Developer/CommandLineTools/SDKs/MacOSX26.sdk/SDKSettings.json", "c.cpp"}}
            });
    }

    {
      const std::vector<compilation_record> records{
        {"CMakeFiles/x.dir/a.cpp.o", {"/proj/a.cpp", "/proj/a.h", "/proj/sub dir/b.h"}},
        {"CMakeFiles/x.dir/b.cpp.o", {"/proj/b.cpp", "/proj/a.h"}},
        {"CMakeFiles/x.dir/c.cpp.o", {}}
      };

      const auto log{scratch / "round_trip.ninja_deps"};
      write_ninja_deps(log, records);
      check(equality, "Round trip, with a path shared between records and one with a space", read_ninja_deps(log), records);
      check(weak_equivalence, "The log as written, byte for byte", log, predictive_materials() / "round_trip.ninja_deps");

      // Cutting the last six bytes leaves a final record whose size word promises more than the file holds
      const auto whole{read_to_string(log, std::ios_base::binary).value()};
      write_to_file(scratch / "interrupted.ninja_deps", std::string_view{whole}.substr(0, whole.size() - 6), std::ios_base::binary);
      check(equality,
            "An incomplete final record, as an interrupted ninja leaves, is ignored and those before it returned",
            read_ninja_deps(scratch / "interrupted.ninja_deps"),
            std::vector<compilation_record>{records.begin(), records.end() - 1});

      auto corrupted{whole};
      corrupted[whole.find("/proj/a.h") - 8] ^= 0x01; // a path record ends in its checksum, and the next begins with a size word
      write_to_file(scratch / "corrupted.ninja_deps", corrupted, std::ios_base::binary);
      check_exception_thrown<std::runtime_error>("A checksum which does not match its record's position", [&](){ return read_ninja_deps(scratch / "corrupted.ninja_deps"); });
    }

    write_to_file(scratch / "not_a_log", "# something else\n", std::ios_base::binary);
    check_exception_thrown<std::runtime_error>("Not a ninja log", [&](){ return read_ninja_deps(scratch / "not_a_log"); });

    write_to_file(scratch / "future_version", std::string{"# ninjadeps\n"} + std::string{"\x07\x00\x00\x00", 4}, std::ios_base::binary);
    check_exception_thrown<std::runtime_error>("A version which is not understood", [&](){ return read_ninja_deps(scratch / "future_version"); });

    write_to_file(scratch / "signature_only", "# ninjadeps\n", std::ios_base::binary);
    check_exception_thrown<std::runtime_error>("A log cut before its version word", [&](){ return read_ninja_deps(scratch / "signature_only"); });

    {
      // Records spelled byte by byte, each malformed in one way the reader refuses
      auto word{
        [](std::uint32_t w) {
          std::string bytes(sizeof(w), '\0');
          std::memcpy(bytes.data(), &w, sizeof(w));
          return bytes;
        }
      };
      constexpr std::uint32_t depsFlag{0x80000000u};
      const auto stamp{std::string(8, '\0')};
      const auto pathA{word(8) + std::string{"a\0\0\0", 4} + word(~0u)};

      const std::vector<std::pair<std::string_view, std::string>> malformed{
        {"A record larger than ninja ever writes",       word(1u << 19) + std::string(1u << 19, '\0')},
        {"A deps record too small to name an output",    word(depsFlag | 4) + word(0)},
        {"A deps record naming an output not yet seen",  word(depsFlag | 12) + word(0) + stamp},
        {"A deps record naming an input not yet seen",   pathA + word(depsFlag | 16) + word(0) + stamp + word(5)},
        {"A path record too small to hold its checksum", word(0)}
      };

      for(const auto& [description, body] : malformed)
      {
        const auto log{scratch / "malformed.ninja_deps"};
        write_to_file(log, std::string{"# ninjadeps\n"} + word(4) + body, std::ios_base::binary);
        check_exception_thrown<std::runtime_error>(description, [&](){ return read_ninja_deps(log); });
      }
    }

    check_exception_thrown<std::runtime_error>("A log which does not exist", [&](){ return read_ninja_deps(scratch / "absent.ninja_deps"); });
  }

  void build_artefacts_free_test::test_tlogs()
  {
    const auto scratch{working_materials()};

    // The files a tracker log names must exist, since it spells them in upper case and their case is recovered from the filesystem
    const auto project{scratch / "Proj"};
    fs::create_directories(project / "Sub Dir");
    for(const auto name : {"a.cpp", "b.cpp", "c.cpp", "a.h", "Sub Dir/b.h", "a.obj", "b.obj", "c.obj"})
    {
      write_to_file(project / name, "", std::ios_base::out);
    }

    {
      // The tracker lists files in the order the compiler opened them, a file more than once where it was opened more than once
      const std::vector<compilation_record> written{
        {project / "a.obj", {project / "a.cpp", project / "a.h", project / "Sub Dir" / "b.h", project / "a.h"}},
        {project / "b.obj", {project / "b.cpp"}},
        {project / "c.obj", {project / "c.cpp", project / "a.h"}}
      };

      write_tlogs(scratch / "round.tlog", written);
      const auto read{read_tlogs(scratch / "round.tlog")};
      check(equality,
            "Round trip: the source first, then what else was read, sorted and each once",
            read,
            std::vector<compilation_record>{
              {project / "a.obj", {project / "a.cpp", project / "Sub Dir" / "b.h", project / "a.h"}},
              {project / "b.obj", {project / "b.cpp"}},
              {project / "c.obj", {project / "c.cpp", project / "a.h"}}
            });
    }

    {
      // One invocation compiling two sources lists them together; each object goes to the source sharing its stem
      const auto dir{scratch / "joint.tlog"};
      fs::create_directories(dir);
      write_utf16(dir / "CL.read.1.tlog", u"^" + upper(project / "a.cpp") + u"|" + upper(project / "c.cpp") + u"\r\n" + upper(project / "a.h") + u"\r\n");
      write_utf16(dir / "CL.write.1.tlog", u"^" + upper(project / "a.cpp") + u"|" + upper(project / "c.cpp") + u"\r\n" + upper(project / "a.obj") + u"\r\n" + upper(project / "c.obj") + u"\r\n");

      const auto read{read_tlogs(dir)};
      check(equality,
            "Sources compiled together, in upper case",
            read,
            std::vector<compilation_record>{
              {project / "a.obj", {project / "a.cpp", project / "a.h"}},
              {project / "c.obj", {project / "c.cpp", project / "a.h"}}
            });
    }

    {
      // A target's tracker logs may carry a number, as the TestAll tree's write log did on Windows
      const auto dir{scratch / "numbered.tlog"};
      fs::create_directories(dir);
      write_utf16(dir / "CL.read.1.tlog",        u"^" + upper(project / "a.cpp") + u"\r\n" + upper(project / "a.h") + u"\r\n");
      write_utf16(dir / "CL.11932.write.1.tlog", u"^" + upper(project / "a.cpp") + u"\r\n" + upper(project / "a.obj") + u"\r\n");
      write_utf16(dir / "CL.command.1.tlog",     u"^" + upper(project / "a.cpp") + u"\r\n" + u"/c /Zi\r\n");

      check(equality,
            "A numbered write log is read; the command log is not",
            read_tlogs(dir),
            std::vector<compilation_record>{{project / "a.obj", {project / "a.cpp", project / "a.h"}}});
    }

    {
      // CMake's Visual Studio generator names objects by the whole source name where stems collide
      const auto dir{scratch / "collision.tlog"};
      fs::create_directories(dir);
      for(const auto name : {"Gadget.cpp", "Gadget.cxx", "Gadget.cpp.obj", "Gadget.cxx.obj"})
      {
        write_to_file(project / name, "", std::ios_base::out);
      }

      const auto roots{u"^" + upper(project / "Gadget.cpp") + u"|" + upper(project / "Gadget.cxx") + u"\r\n"};
      write_utf16(dir / "CL.read.1.tlog", roots + upper(project / "a.h") + u"\r\n");
      write_utf16(dir / "CL.write.1.tlog", roots + upper(project / "Gadget.cpp.obj") + u"\r\n" + upper(project / "Gadget.cxx.obj") + u"\r\n");

      const auto read{read_tlogs(dir)};
      check(equality,
            "Objects named by the whole source name go to the right source",
            read,
            std::vector<compilation_record>{
              {project / "Gadget.cpp.obj", {project / "Gadget.cpp", project / "a.h"}},
              {project / "Gadget.cxx.obj", {project / "Gadget.cxx", project / "a.h"}}
            });

      write_utf16(dir / "CL.write.1.tlog", roots + upper(project / "a.obj") + u"\r\n" + upper(project / "b.obj") + u"\r\n");
      check_exception_thrown<std::runtime_error>("An object which bears neither source's name", [&](){ return read_tlogs(dir); });
    }

    {
      // MSBuild's ObjectFileName may name an object anything; alone under its source, the object is the source's whatever the name
      const auto dir{scratch / "renamed.tlog"};
      fs::create_directories(dir);
      for(const auto name : {"main.cpp", "main_x64.obj"})
      {
        write_to_file(project / name, "", std::ios_base::out);
      }

      write_utf16(dir / "CL.read.1.tlog", u"^" + upper(project / "main.cpp") + u"\r\n" + upper(project / "a.h") + u"\r\n");
      write_utf16(dir / "CL.write.1.tlog", u"^" + upper(project / "main.cpp") + u"\r\n" + upper(project / "main_x64.obj") + u"\r\n");
      check(equality,
            "A lone object bearing neither the source's stem nor its name is the source's",
            read_tlogs(dir),
            std::vector<compilation_record>{{project / "main_x64.obj", {project / "main.cpp", project / "a.h"}}});
    }

    {
      // Two sources compiled together, writing one object which bears neither name: each would be given it
      const auto dir{scratch / "claimed_twice.tlog"};
      fs::create_directories(dir);
      const auto roots{u"^" + upper(project / "a.cpp") + u"|" + upper(project / "c.cpp") + u"\r\n"};
      write_utf16(dir / "CL.read.1.tlog", roots + upper(project / "a.h") + u"\r\n");
      write_utf16(dir / "CL.write.1.tlog", roots + upper(project / "main_x64.obj") + u"\r\n");
      check_exception_thrown<std::runtime_error>("An object which two sources would each be given", [&](){ return read_tlogs(dir); });
    }

    {
      // The tracker's UTF-16 reaches the path as code units, so a name outside ASCII survives on every platform
      const auto accented{project / "Jos\u00e9"};
      fs::create_directories(accented);
      for(const auto name : {"d.cpp", "d.obj"})
      {
        write_to_file(accented / name, "", std::ios_base::out);
      }

      const auto dir{scratch / "accented.tlog"};
      fs::create_directories(dir);
      write_utf16(dir / "CL.read.1.tlog", u"^" + upper(accented / "d.cpp") + u"\r\n");
      write_utf16(dir / "CL.write.1.tlog", u"^" + upper(accented / "d.cpp") + u"\r\n" + upper(accented / "d.obj") + u"\r\n");
      check(equality,
            "A directory named outside ASCII",
            read_tlogs(dir),
            std::vector<compilation_record>{{accented / "d.obj", {accented / "d.cpp"}}});
    }

    {
      const auto dir{scratch / "narrow.tlog"};
      fs::create_directories(dir);
      write_to_file(dir / "CL.read.1.tlog", "^a.cpp\n", std::ios_base::binary);
      check_exception_thrown<std::runtime_error>("A log which is not the tracker's UTF-16", [&](){ return read_tlogs(dir); });
    }

    {
      const auto dir{scratch / "unwritten.tlog"};
      fs::create_directories(dir);
      write_utf16(dir / "CL.read.1.tlog", u"^" + upper(project / "a.cpp") + u"\r\n" + upper(project / "a.h") + u"\r\n");
      write_utf16(dir / "CL.write.1.tlog", u"");
      check(equality, "A source which wrote no object is not a compilation", read_tlogs(dir), std::vector<compilation_record>{});
    }
  }

  void build_artefacts_free_test::test_build_tree()
  {
    const auto scratch{working_materials()};
    const auto root{scratch / "build"};
    fs::create_directories(root / "CMakeFiles" / "4.1.2");
    fs::create_directories(root / "CMakeFiles" / "4.2.0");
    fs::create_directories(root / "Debug");
    const auto cache{root / "CMakeCache.txt"};
    const auto executable{root / "Debug" / "TestAll"};

    // Two compiler information directories, as a tree configured by two versions of CMake has, and CRLF, as Windows writes
    write_to_file(cache, "# CMake cache\r\nCMAKE_GENERATOR:INTERNAL=Ninja\r\nCMAKE_HOME_DIRECTORY:INTERNAL=/proj\r\n", std::ios_base::binary);
    write_to_file(root / "CMakeFiles" / "4.1.2" / "CMakeCXXCompiler.cmake",
                  "set(CMAKE_CXX_COMPILER \"/usr/bin/c++\")\nset(CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES \"/usr/include/c++/16;/usr/include\")\n",
                  std::ios_base::out);
    write_to_file(root / "CMakeFiles" / "4.2.0" / "CMakeCXXCompiler.cmake",
                  "set(CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES \"/usr/include\")\n",
                  std::ios_base::out);

    const auto tree{read_build_tree(cache)};
    check(equality, "Build directory", tree.build_directory, root);
    check(equality, "Generator", tree.generator, std::string{"Ninja"});
    check(equality, "Implicit include directories, from every compiler information file", tree.implicit_include_directories, std::vector<fs::path>{"/usr/include/c++/16", "/usr/include", "/usr/include"});

    check_exception_thrown<std::runtime_error>("A cache which does not exist", [&](){ return read_build_tree(scratch / "elsewhere" / "CMakeCache.txt"); });
    check_exception_thrown<std::runtime_error>("A Ninja tree which has not been built has no log", [&](){ return read_compilations(tree, executable); });

    {
      const std::vector<compilation_record> logged{
        {"CMakeFiles/x.dir/a.cpp.o", {"/proj/a.cpp", "/proj/a.h"}},
        {"CMakeFiles/x.dir/b.cpp.o", {"/proj/b.h"}},
        {"CMakeFiles/x.dir/c.cpp.o", {"/proj/c.h", "/proj/c.cpp"}},
        {"CMakeFiles/x.dir/retired.cpp.o", {"/proj/retired.cpp"}}
      };
      write_ninja_deps(root / ".ninja_deps", logged);

      write_to_file(root / "build.ninja",
                    "build CMakeFiles/x.dir/a.cpp.o: CXX_COMPILER /proj/a.cpp || cmake_object_order_depends\n"
                    "build CMakeFiles/x.dir/b.cpp.o: CXX_COMPILER /proj/b.cpp\n"
                    "build CMakeFiles/x.dir/c.cpp.o: CXX_COMPILER /proj/c.cpp\n",
                    std::ios_base::out);
      check(equality,
            "The log's record of an object build.ninja no longer names is not read; the source comes first, supplied where the log omits it",
            read_compilations(tree, executable),
            std::vector<compilation_record>{
              {"CMakeFiles/x.dir/a.cpp.o", {"/proj/a.cpp", "/proj/a.h"}},
              {"CMakeFiles/x.dir/b.cpp.o", {"/proj/b.cpp", "/proj/b.h"}},
              {"CMakeFiles/x.dir/c.cpp.o", {"/proj/c.cpp", "/proj/c.h"}}
            });

      write_to_file(root / "build.ninja", "build CMakeFiles/x.dir/a.cpp.o CXX_COMPILER /proj/a.cpp\n", std::ios_base::out);
      check_exception_thrown<std::runtime_error>("A build statement with no rule", [&](){ return read_compilations(tree, executable); });

      write_to_file(root / "build.ninja",
                    "build CMakeFiles/x.dir/a.cpp.o: CXX_COMPILER /proj/a.cpp\r\n"
                    "  DEP_FILE = CMakeFiles/x$ dir/a.cpp.o.d\r\n"
                    "build CMakeFiles/x.dir/b.cpp.o | CMakeFiles/x.dir/b.cpp.o.extra: CXX_COMPILER /proj/b.cpp | /proj/implicit.h\n",
                    std::ios_base::out);
      check(equality,
            "Statements with implicit outputs and inputs, variable lines and CRLF",
            read_compilations(tree, executable),
            std::vector<compilation_record>{
              {"CMakeFiles/x.dir/a.cpp.o", {"/proj/a.cpp", "/proj/a.h"}},
              {"CMakeFiles/x.dir/b.cpp.o", {"/proj/b.cpp", "/proj/b.h"}}
            });

      // The generator escapes a space, a colon and a dollar; ninja stores what they stand for
      write_ninja_deps(root / ".ninja_deps", std::vector<compilation_record>{{"CMakeFiles/x.dir/odd name.cpp.o", {"C:/proj/a$b.h"}}});
      write_to_file(root / "build.ninja",
                    "build CMakeFiles/x.dir/odd$ name.cpp.o: CXX_COMPILER C$:/proj/odd$ name$$.cpp\n",
                    std::ios_base::out);
      check(equality,
            "A statement's tokens are read in their plain spelling, so its object matches the log and its source is as on disk",
            read_compilations(tree, executable),
            std::vector<compilation_record>{{"CMakeFiles/x.dir/odd name.cpp.o", {"C:/proj/odd name$.cpp", "C:/proj/a$b.h"}}});

      // An escaped dollar escapes nothing after it: the colon immediately after `$$` still ends the outputs, and the space still separates
      write_ninja_deps(root / ".ninja_deps", std::vector<compilation_record>{{"CMakeFiles/x.dir/a$", {"/proj/b$"}}});
      write_to_file(root / "build.ninja", "build CMakeFiles/x.dir/a$$: CXX_COMPILER /proj/b$$ || order\n", std::ios_base::out);
      check(equality,
            "A dollar escaped by another is not itself an escape",
            read_compilations(tree, executable),
            std::vector<compilation_record>{{"CMakeFiles/x.dir/a$", {"/proj/b$"}}});

      write_to_file(root / "build.ninja", "build CMakeFiles/y.dir/c.cpp.o: CXX_COMPILER /proj/c.cpp\n", std::ios_base::out);
      check_exception_thrown<std::runtime_error>("A log none of whose objects build.ninja names is two spellings of one tree, not an empty build", [&](){ return read_compilations(tree, executable); });
    }

    for(const auto generator : {"Ninja Multi-Config", "Xcode"})
    {
      write_to_file(cache, std::format("# CMake cache\nCMAKE_GENERATOR:INTERNAL={}\n", generator), std::ios_base::out);
      check_exception_thrown<std::runtime_error>(std::format("{}: a generator whose record of dependencies is not understood", generator), [&](){ return read_compilations(read_build_tree(cache), executable); });
    }

    write_to_file(cache, "# CMake cache\n", std::ios_base::out);
    check_exception_thrown<std::runtime_error>("A cache which names no generator was not written by CMake", [&](){ return read_build_tree(cache); });
  }
}
