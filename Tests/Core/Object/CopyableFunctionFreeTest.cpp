////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/** \file */

#include "CopyableFunctionFreeTest.hpp"
#include "sequoia/Core/Object/CopyableFunction.hpp"

#include <array>
#include <memory>
#include <string>

namespace sequoia::testing
{
  using namespace object;

  namespace
  {
    /** Counts its own copies, moves and destructions, so that a wrapper's lifetime behaviour can
        be observed from outside rather than inferred from the fact that nothing crashed.
     */
    struct counter
    {
      inline static int copies{}, moves{}, destructions{};

      static void reset() { copies = moves = destructions = 0; }

      counter() = default;
      counter(const counter&) { ++copies; }
      counter(counter&&) noexcept { ++moves; }
      counter& operator=(const counter&) { ++copies; return *this; }
      counter& operator=(counter&&) noexcept { ++moves; return *this; }
      ~counter() { ++destructions; }
    };

    /** Large enough to defeat the small buffer, which is three pointers, and counted, so that a
        copy of the wrapper which shared the payload rather than cloning it would be seen.
     */
    struct big_payload
    {
      std::array<double, 8> values{};
      counter count{};
    };

    [[nodiscard]]
    int free_function() { return 42; }
  }

  [[nodiscard]]
  std::filesystem::path copyable_function_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void copyable_function_free_test::run_tests()
  {
    test_small_target();
    test_large_target();
    test_arguments();
    test_conversion();
    test_constraints();
    test_swap();
    test_constant_evaluation();
  }

  template<class Fn>
  void copyable_function_free_test::test_lifetime(std::string_view description, Fn fn, int expected)
  {
    using function_t = copyable_function<int() const>;

    check(append_lines(description, "A default-constructed function holds nothing"), !static_cast<bool>(function_t{}));

    const function_t f{fn};
    check(append_lines(description, "A function holding a target is engaged"), static_cast<bool>(f));
    check(equality, append_lines(description, "Invocation"), f(), expected);

    function_t copy{f};
    check(append_lines(description, "The source of a copy is still engaged"), static_cast<bool>(f));
    check(equality, append_lines(description, "A copy invokes as the original does"), copy(), expected);
    check(equality, append_lines(description, "The original still invokes"), f(), expected);

    function_t moved{std::move(copy)};
    check(append_lines(description, "The source of a move is disengaged"), !static_cast<bool>(copy));
    check(equality, append_lines(description, "The target of a move invokes"), moved(), expected);

    function_t assigned{};
    assigned = moved;
    check(append_lines(description, "Copy assignment leaves the source engaged"), static_cast<bool>(moved));
    check(equality, append_lines(description, "Copy assignment"), assigned(), expected);

    function_t moveAssigned{};
    moveAssigned = std::move(moved);
    check(equality, append_lines(description, "Move assignment"), moveAssigned(), expected);

    // Assignment over an engaged function must destroy what it held; asan and ubsan are the
    // instruments for that, so the check here is that it still invokes correctly afterwards.
    assigned = function_t{fn};
    check(equality, append_lines(description, "Assignment over an engaged function"), assigned(), expected);

    // Self-assignment goes through the same copy-and-swap path and must not leave a moved-from
    // target behind. The indirection is what stops the compiler warning about the very case under
    // test.
    const function_t& alias{assigned};
    assigned = alias;
    check(append_lines(description, "Self-assignment leaves the function engaged"), static_cast<bool>(assigned));
    check(equality, append_lines(description, "Self-assignment"), assigned(), expected);
  }

  void copyable_function_free_test::test_small_target()
  {
    int captured{7};
    test_lifetime("Small target", [captured]() { return captured; }, 7);
    test_lifetime("Small target, by reference", [&captured]() { return captured; }, 7);
    test_lifetime("Function pointer", &free_function, 42);

    // The wrapper must copy and destroy its target exactly as the target expects, which is what a
    // counting payload can show and a lambda cannot.
    counter::reset();
    {
      counter c{};
      copyable_function<int() const> f{[c]() { return 1; }};
      check(equality, "Capturing by value copies the payload; the wrapper does not copy it again", counter::copies, 1);
      check(equality, "The wrapper takes the closure by value, so a temporary costs a move rather than a copy", counter::moves, 1);

      const auto g{f};
      check(equality, "Copying the wrapper copies the payload", counter::copies, 2);

      const auto h{std::move(f)};
      check(equality, "Moving the wrapper does not copy", counter::copies, 2);
      check(equality, "Moving the wrapper moves the payload", counter::moves, 2);
    }
    check(equality, "Every payload constructed is destroyed", counter::destructions, counter::copies + counter::moves + 1);
  }

  void copyable_function_free_test::test_large_target()
  {
    big_payload big{};
    big.values[3] = 2.5;

    test_lifetime("Large target", [big]() { return static_cast<int>(big.values[3] * 2); }, 5);

    // A payload too large for the small buffer is held by pointer, so copying must deep-copy it
    // rather than share it, which the payload's counter sees.
    const copyable_function<int() const> f{[big]() { return static_cast<int>(big.values[3] * 2); }};
    counter::reset();
    const auto g{f};
    check(equality, "Copying a wrapper holding a large target copies the payload", counter::copies, 1);
    check(equality, "A large target survives being copied", g(), 5);
    check(equality, "The original is unaffected", f(), 5);
  }

  void copyable_function_free_test::test_arguments()
  {
    const copyable_function<int(int) const> doubler{[](int i) { return 2 * i; }};
    check(equality, "One argument", doubler(21), 42);

    const copyable_function<int(int, int) const> adder{[](int i, int j) { return i + j; }};
    check(equality, "Two arguments", adder(20, 22), 42);

    // A reference parameter must reach the target as a reference rather than a copy.
    const copyable_function<void(int&) const> incrementer{[](int& i) { ++i; }};
    int value{41};
    incrementer(value);
    check(equality, "An argument taken by reference is not copied on the way through", value, 42);

    const copyable_function<std::string(const std::string&) const> exclaim{[](const std::string& s) { return s + "!"; }};
    check(equality, "An argument taken by const reference", exclaim("hello"), std::string{"hello!"});

    // A move-only argument reaches the target only if the thunk forwards rather than copies.
    const copyable_function<int(std::unique_ptr<int>) const> deref{[](std::unique_ptr<int> p) { return *p; }};
    check(equality, "A move-only argument is forwarded", deref(std::make_unique<int>(42)), 42);
  }

  void copyable_function_free_test::test_conversion()
  {
    // The whole reason the constraint is std::is_invocable_r_v rather than an exact match: a target
    // whose result is merely convertible to the signature's must be accepted. A generator returning
    // a reference for a signature returning a value is the case which arises in practice.
    const std::string held{"held"};
    const copyable_function<std::string() const> byReference{[&held]() -> const std::string& { return held; }};
    check(equality, "A target returning a reference satisfies a signature returning a value", byReference(), held);

    const copyable_function<double() const> widening{[]() { return 3; }};
    check(equality, "A target returning int satisfies a signature returning double", widening(), 3.0);

    int sideEffect{};
    const copyable_function<void() const> discarding{[&sideEffect]() { return sideEffect = 42; }};
    discarding();
    check(equality, "A target with a result satisfies a signature returning void", sideEffect, 42);
  }

  void copyable_function_free_test::test_constraints()
  {
    using function_t = copyable_function<int() const>;

    STATIC_CHECK(std::constructible_from<function_t, int(*)()>);
    STATIC_CHECK(std::copyable<function_t>);
    STATIC_CHECK(std::default_initializable<function_t>);

    // A copyable_function is itself an acceptable target; the converting constructor's
    // resolve_to_copy_v clause states that copying is not conversion, though the non-template
    // constructors would win regardless.
    STATIC_CHECK(std::is_invocable_r_v<int, const function_t&>);
    STATIC_CHECK(std::is_copy_constructible_v<function_t>);

    // Targets which cannot satisfy the signature are rejected rather than failing at the call.
    STATIC_CHECK(!std::constructible_from<function_t, int>);
    STATIC_CHECK(!std::constructible_from<function_t, void(*)()>);
    STATIC_CHECK(!std::constructible_from<copyable_function<int(int) const>, int(*)(int, int)>);

    // A target invocable only through a non-const reference cannot satisfy a const-qualified
    // signature. The capture is what makes this so: a captureless mutable lambda converts to a
    // function pointer, and that conversion operator is itself const, so the closure is
    // const-invocable after all.
    STATIC_CHECK(!std::constructible_from<function_t, decltype([i = 0]() mutable { return i; })>);
    STATIC_CHECK(std::constructible_from<function_t, decltype([]() mutable { return 1; })>);
  }

  void copyable_function_free_test::test_swap()
  {
    using function_t = copyable_function<int() const>;

    big_payload big{};
    big.values[0] = 1.0;

    function_t small{[]() { return 1; }},
               large{[big]() { return static_cast<int>(big.values[0]) + 1; }},
               empty{};

    swap(small, large);
    check(equality, "Swapping a small target with a large one, first", small(), 2);
    check(equality, "Swapping a small target with a large one, second", large(), 1);

    swap(small, empty);
    check("Swapping with an empty function disengages the source", !static_cast<bool>(small));
    check(equality, "Swapping with an empty function engages the target", empty(), 2);

    swap(small, empty);
    check(equality, "Swapping back", small(), 2);
    check("Swapping back disengages the other", !static_cast<bool>(empty));

    swap(small, small);
    check(equality, "Swapping a function with itself", small(), 2);

    // Two targets on the same path, for each path: the small ones are not trivially managed, so
    // that the general manager's move through the temporary is what is exercised.
    counter c{};
    function_t smallCounted{[c]() { return 5; }}, otherSmallCounted{[c]() { return 6; }};
    swap(smallCounted, otherSmallCounted);
    check(equality, "Swapping two small targets, first", smallCounted(), 6);
    check(equality, "Swapping two small targets, second", otherSmallCounted(), 5);

    // `small` holds the large target by now
    function_t otherLarge{[big]() { return static_cast<int>(big.values[0]) + 7; }};
    swap(small, otherLarge);
    check(equality, "Swapping two large targets, first", small(), 8);
    check(equality, "Swapping two large targets, second", otherLarge(), 2);

    // Value preservation is too weak a test of a self-swap: three moves through a temporary happen
    // to restore what they disturb, so a self-swap which ends the target's lifetime and then moves
    // from it - formally undefined, and invisible to asan since nothing is freed - still returns
    // the right answer. What distinguishes the two is that the correct implementation does no work
    // at all, which a counting payload can see.
    function_t counted{[c = counter{}]() { return 3; }};
    counter::reset();
    swap(counted, counted);
    check(equality, "Swapping a function with itself moves nothing", counter::moves, 0);
    check(equality, "Swapping a function with itself destroys nothing", counter::destructions, 0);
    check(equality, "Swapping a function with itself preserves its target", counted(), 3);

    function_t neither{}, alsoNeither{};
    swap(neither, alsoNeither);
    check("Swapping two empty functions leaves both empty", !static_cast<bool>(neither) && !static_cast<bool>(alsoNeither));
  }

  void copyable_function_free_test::test_constant_evaluation()
  {
    // Every operation, over a target whose own copy allocates and over one which is trivially
    // managed at run time, since in a constant evaluation both are held behind the pointer. A
    // copy which shared its target rather than cloning it would delete the target twice, and a
    // leaked target would outlive the evaluation; the evaluator refuses both, so it is the
    // instrument for the lifetime, and the results pin invocation, swap and disengagement.
    constexpr auto lifetime{
      []() {
        using function_t = copyable_function<int(int) const>;

        const std::string captured{"twelve chars"};
        function_t f{[captured](int x) { return x + static_cast<int>(captured.size()); }};
        function_t copy{f};
        function_t moved{std::move(f)};
        function_t negating{[](int x) { return -x; }};
        function_t assigned{};
        assigned = copy;
        swap(copy, negating);

        const function_t& alias{assigned};
        assigned = alias;

        int sideEffect{};
        const copyable_function<void(int) const> discarding{[&sideEffect](int x) { sideEffect = x; }};
        discarding(3);

        // A result which is a class type with a non-trivial destructor: the shape gcc rejects when the
        // thunk is a lambda called through a function pointer (GCC bug 125000)
        const copyable_function<std::string() const> generating{[captured]() { return captured; }};

        return std::array{assigned(1), copy(1), negating(1), moved(1), f ? 1 : 0, sideEffect, static_cast<int>(generating().size())};
      }
    };

    constexpr std::array expected{13, -1, 13, 13, 0, 3, 12};

    // The cast back from void* in a constant evaluation is C++26's (P2738); a build without it
    // runs the same lifetime at run time, so that the check count does not depend on the build
#if __cpp_constexpr >= 202306L
    STATIC_CHECK(lifetime() == expected);
#else
    check(equality, "The lifetime at run time; the constant evaluation is untried below C++26", lifetime(), expected);
#endif
  }
}
