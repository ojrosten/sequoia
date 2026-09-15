////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "BuildArtefactsFreeTest.hpp"
#include "sequoia/TestFramework/BuildArtefacts.hpp"

#include "sequoia/Streaming/Streaming.hpp"

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
    test_dyndep();
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

      // Ninja reads only what it can, so a record cut short by an interruption is dropped and those before it kept
      const auto whole{read_to_string(log, std::ios_base::binary).value()};
      write_to_file(scratch / "truncated.ninja_deps", std::string_view{whole}.substr(0, whole.size() - 6), std::ios_base::binary);
      check(equality,
            "A truncated final record is dropped",
            read_ninja_deps(scratch / "truncated.ninja_deps"),
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

    check_exception_thrown<std::runtime_error>("A log which does not exist", [&](){ return read_ninja_deps(scratch / "absent.ninja_deps"); });
  }

  void build_artefacts_free_test::test_dyndep()
  {
    const auto scratch{working_materials()};

    const std::vector<compilation_record> records{
      {"CMakeFiles/x.dir/M.cppm.o", {}, fs::path{"CMakeFiles/x.dir/M.pcm"}, {"CMakeFiles/x.dir/std.pcm", "CMakeFiles/x.dir/M-P.pcm"}},
      {"CMakeFiles/x.dir/P.cppm.o", {}, fs::path{"CMakeFiles/x.dir/M-P.pcm"}, {}},
      {"CMakeFiles/x.dir/use.cpp.o", {}, std::nullopt, {"CMakeFiles/x.dir/M.pcm"}},
      {"CMakeFiles/x.dir/plain.cpp.o", {}, std::nullopt, {}},
      {"CMakeFiles/x.dir/odd name.cpp.o", {}, std::nullopt, {"C:/with colon/M.pcm"}}
    };

    write_dyndep(scratch / "CXX.dd", records);
    check(equality, "Round trip, with a space and a colon escaped", read_dyndep(scratch / "CXX.dd"), records);
    check(weak_equivalence, "The file as written", scratch / "CXX.dd", predictive_materials() / "CXX.dd");

    write_to_file(scratch / "cmake.dd",
                  "ninja_dyndep_version = 1.0\n"
                  "build CMakeFiles/x.dir/a.cpp.o | CMakeFiles/x.dir/A.pcm: dyndep | CMakeFiles/x.dir/std.pcm || CMakeFiles/x.dir/order.only\n"
                  "  restat = 1\n"
                  "build CMakeFiles/x.dir/b.cpp.o: dyndep\n",
                  std::ios_base::out);
    check(equality,
          "As CMake writes it: a restat variable and an order-only input, both ignored",
          read_dyndep(scratch / "cmake.dd"),
          std::vector<compilation_record>{
            {"CMakeFiles/x.dir/a.cpp.o", {}, fs::path{"CMakeFiles/x.dir/A.pcm"}, {"CMakeFiles/x.dir/std.pcm"}},
            {"CMakeFiles/x.dir/b.cpp.o", {}, std::nullopt, {}}
          });

    write_to_file(scratch / "bad.dd", "ninja_dyndep_version = 1.0\nbuild foo.o: cc foo.cpp\n", std::ios_base::out);
    check_exception_thrown<std::runtime_error>("A build statement which is not a dyndep", [&](){ return read_dyndep(scratch / "bad.dd"); });

    check_exception_thrown<std::runtime_error>("A file which does not exist", [&](){ return read_dyndep(scratch / "absent.dd"); });
  }

  void build_artefacts_free_test::test_tlogs()
  {
    const auto scratch{working_materials()};

    // The files a tracker log names must exist, since it spells them in upper case and their case is recovered from the filesystem
    const auto project{scratch / "Proj"};
    fs::create_directories(project / "Sub Dir");
    for(const auto name : {"a.cpp", "b.cpp", "c.cpp", "a.h", "Sub Dir/b.h", "M.ifc", "a.obj", "b.obj", "c.obj"})
    {
      write_to_file(project / name, "", std::ios_base::out);
    }

    {
      const std::vector<compilation_record> records{
        {project / "a.obj", {project / "a.cpp", project / "a.h", project / "Sub Dir" / "b.h"}, std::nullopt, {project / "M.ifc"}},
        {project / "b.obj", {project / "b.cpp"}, project / "M.ifc", {}},
        {project / "c.obj", {project / "c.cpp", project / "a.h"}, std::nullopt, {}}
      };

      write_tlogs(scratch / "round.tlog", records);
      auto read{read_tlogs(scratch / "round.tlog")};
      std::ranges::sort(read, {}, &compilation_record::output);
      check(equality, "Round trip: the source first, an .ifc read is required and one written is provided", read, records);
    }

    {
      // One invocation compiling two sources lists them together; each object goes to the source sharing its stem
      const auto dir{scratch / "joint.tlog"};
      fs::create_directories(dir);
      write_utf16(dir / "CL.read.1.tlog", u"^" + upper(project / "a.cpp") + u"|" + upper(project / "c.cpp") + u"\r\n" + upper(project / "a.h") + u"\r\n");
      write_utf16(dir / "CL.write.1.tlog", u"^" + upper(project / "a.cpp") + u"|" + upper(project / "c.cpp") + u"\r\n" + upper(project / "a.obj") + u"\r\n" + upper(project / "c.obj") + u"\r\n");

      auto read{read_tlogs(dir)};
      std::ranges::sort(read, {}, &compilation_record::output);
      check(equality,
            "Sources compiled together, in upper case",
            read,
            std::vector<compilation_record>{
              {project / "a.obj", {project / "a.cpp", project / "a.h"}, std::nullopt, {}},
              {project / "c.obj", {project / "c.cpp", project / "a.h"}, std::nullopt, {}}
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
            std::vector<compilation_record>{{project / "a.obj", {project / "a.cpp", project / "a.h"}, std::nullopt, {}}});
    }

    {
      // CMake's Visual Studio generator names objects by the whole source name where stems collide
      const auto dir{scratch / "collision.tlog"};
      fs::create_directories(dir);
      for(const auto name : {"Gadget.cpp", "Gadget.cppm", "Gadget.cpp.obj", "Gadget.cppm.obj", "Gadget.ifc"}) write_to_file(project / name, "", std::ios_base::out);

      const auto roots{u"^" + upper(project / "Gadget.cpp") + u"|" + upper(project / "Gadget.cppm") + u"\r\n"};
      write_utf16(dir / "CL.read.1.tlog", roots + upper(project / "a.h") + u"\r\n");
      write_utf16(dir / "CL.write.1.tlog", roots + upper(project / "Gadget.cpp.obj") + u"\r\n" + upper(project / "Gadget.cppm.obj") + u"\r\n");

      auto read{read_tlogs(dir)};
      std::ranges::sort(read, {}, &compilation_record::output);
      check(equality,
            "Objects named by the whole source name go to the right source",
            read,
            std::vector<compilation_record>{
              {project / "Gadget.cpp.obj",  {project / "Gadget.cpp",  project / "a.h"}, std::nullopt, {}},
              {project / "Gadget.cppm.obj", {project / "Gadget.cppm", project / "a.h"}, std::nullopt, {}}
            });

      write_utf16(dir / "CL.write.1.tlog", roots + upper(project / "Gadget.cpp.obj") + u"\r\n" + upper(project / "Gadget.cppm.obj") + u"\r\n" + upper(project / "M.ifc") + u"\r\n" + upper(project / "Gadget.ifc") + u"\r\n");
      check_exception_thrown<std::runtime_error>("Sources compiled together which wrote two interfaces cannot be told apart", [&](){ return read_tlogs(dir); });

      write_utf16(dir / "CL.write.1.tlog", roots + upper(project / "a.obj") + u"\r\n" + upper(project / "b.obj") + u"\r\n");
      check_exception_thrown<std::runtime_error>("An object which bears neither source's name", [&](){ return read_tlogs(dir); });
    }

    {
      // The tracker's UTF-16 reaches the path as code units, so a name outside ASCII survives on every platform
      const auto accented{project / "Jos\u00e9"};
      fs::create_directories(accented);
      for(const auto name : {"d.cpp", "d.obj"}) write_to_file(accented / name, "", std::ios_base::out);

      const auto dir{scratch / "accented.tlog"};
      fs::create_directories(dir);
      write_utf16(dir / "CL.read.1.tlog", u"^" + upper(accented / "d.cpp") + u"\r\n");
      write_utf16(dir / "CL.write.1.tlog", u"^" + upper(accented / "d.cpp") + u"\r\n" + upper(accented / "d.obj") + u"\r\n");
      check(equality,
            "A directory named outside ASCII",
            read_tlogs(dir),
            std::vector<compilation_record>{{accented / "d.obj", {accented / "d.cpp"}, std::nullopt, {}}});
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
    check(equality, "Root", tree.root, root);
    check(equality, "Generator", tree.generator, std::string{"Ninja"});
    check(equality, "Implicit include directories, from every compiler information file", tree.implicitIncludeDirs, std::vector<fs::path>{"/usr/include/c++/16", "/usr/include", "/usr/include"});

    check_exception_thrown<std::runtime_error>("A cache which does not exist", [&](){ return read_build_tree(scratch / "elsewhere" / "CMakeCache.txt"); });
    check_exception_thrown<std::runtime_error>("A Ninja tree which has not been built has no log", [&](){ return read_compilations(tree, executable); });

    {
      const std::vector<compilation_record> logged{
        {"CMakeFiles/x.dir/a.cpp.o", {"/proj/a.cpp", "/proj/a.h"}},
        {"CMakeFiles/x.dir/b.cpp.o", {"/proj/b.h"}},
        {"CMakeFiles/x.dir/retired.cpp.o", {"/proj/retired.cpp"}}
      };
      write_ninja_deps(root / ".ninja_deps", logged);
      fs::create_directories(root / "CMakeFiles" / "x.dir");
      write_dyndep(root / "CMakeFiles" / "x.dir" / "CXX.dd", std::vector<compilation_record>{{"CMakeFiles/x.dir/a.cpp.o", {}, std::nullopt, {"CMakeFiles/x.dir/M.pcm"}}});

      write_to_file(root / "build.ninja",
                    "build CMakeFiles/x.dir/a.cpp.o: CXX_COMPILER /proj/a.cpp || cmake_object_order_depends\n"
                    "build CMakeFiles/x.dir/b.cpp.o: CXX_COMPILER /proj/b.cpp\n",
                    std::ios_base::out);
      check(equality,
            "The log's record of an object build.ninja no longer names is not read; a source the log omits is supplied; a dyndep file build.ninja does not name is not read",
            read_compilations(tree, executable),
            std::vector<compilation_record>{
              {"CMakeFiles/x.dir/a.cpp.o", {"/proj/a.cpp", "/proj/a.h"}, std::nullopt, {}},
              {"CMakeFiles/x.dir/b.cpp.o", {"/proj/b.cpp", "/proj/b.h"}, std::nullopt, {}}
            });

      write_to_file(root / "build.ninja",
                    "build CMakeFiles/x.dir/a.cpp.o: CXX_COMPILER /proj/a.cpp\r\n"
                    "  dyndep = CMakeFiles/x$ dir/CXX.dd\r\n"
                    "build CMakeFiles/x.dir/b.cpp.o | CMakeFiles/x.dir/b.pcm: CXX_COMPILER /proj/b.cpp | CMakeFiles/x.dir/M.pcm\n"
                    "  dyndep = CMakeFiles/x.dir/CXX.dd\n",
                    std::ios_base::out);
      check(equality,
            "A dyndep file build.ninja names is joined on the output, the statement's escapes and CRLF notwithstanding",
            read_compilations(tree, executable),
            std::vector<compilation_record>{
              {"CMakeFiles/x.dir/a.cpp.o", {"/proj/a.cpp", "/proj/a.h"}, std::nullopt, {"CMakeFiles/x.dir/M.pcm"}},
              {"CMakeFiles/x.dir/b.cpp.o", {"/proj/b.cpp", "/proj/b.h"}, std::nullopt, {}}
            });

      write_to_file(root / "build.ninja", "build CMakeFiles/y.dir/c.cpp.o: CXX_COMPILER /proj/c.cpp\n", std::ios_base::out);
      check_exception_thrown<std::runtime_error>("A log none of whose objects build.ninja names is two spellings of one tree, not an empty build", [&](){ return read_compilations(tree, executable); });
    }

    for(const auto generator : {"Ninja Multi-Config", "Xcode"})
    {
      write_to_file(cache, std::format("# CMake cache\nCMAKE_GENERATOR:INTERNAL={}\n", generator), std::ios_base::out);
      check_exception_thrown<std::runtime_error>(std::format("{}: a generator whose record of dependencies is not understood", generator), [&](){ return read_compilations(read_build_tree(cache), executable); });
    }
  }
}
