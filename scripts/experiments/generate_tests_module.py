#!/usr/bin/env python3
"""EXPERIMENT: emit all of Tests + TestCommon as one module over `import sequoia`.

    python3 scripts/experiments/generate_tests_module.py <out.cppm>

A single module for 220 test headers is deliberately the wrong granularity, and
saying so is half the result: it puts every test helper namespace in scope at
once, and 14 of 172 test units then fail on ambiguities such as `sets` and
`wait`. Production wants a module per test, where helpers are simply not
exported and take module linkage. See memory/project_compile_time_experiments.
"""

ROOTS=["Tests","TestCommon"]
INC_Q=re.compile(r'^\s*#include\s+"[^"]+"\s*$', re.M)
INC_A=re.compile(r'^\s*#include\s+<[^>]+>\s*$', re.M)
PRAGMA=re.compile(r'^\s*#pragma once\s*$', re.M)
LIC=re.compile(r'^/{60,}\n(?:.*\n)*?^/{60,}\n', re.M)

hdrs=[]
for r in ROOTS:
    for d,_,fs in os.walk(r):
        hdrs += [os.path.join(d,f) for f in fs if f.endswith(".hpp")]
byname={}
for h in hdrs:
    byname.setdefault(os.path.basename(h), []).append(h)

dep=collections.defaultdict(set)
for h in hdrs:
    hd=os.path.dirname(h)
    for inc in re.findall(r'#include "([^"]+)"', open(h).read()):
        cand=os.path.normpath(os.path.join(hd, inc))
        if cand in hdrs: dep[h].add(cand); continue
        for r in ROOTS:
            c2=os.path.normpath(os.path.join(r, inc))
            if c2 in hdrs: dep[h].add(c2); break
        else:
            base=os.path.basename(inc)
            if base in byname and len(byname[base])==1 and byname[base][0]!=h:
                dep[h].add(byname[base][0])

order=[]; done=set(); active=set()
def visit(n):
    if n in done or n in active: return
    active.add(n)
    for d in sorted(dep[n]): visit(d)
    active.discard(n); done.add(n); order.append(n)
for n in sorted(hdrs): visit(n)

parts=[]
for n in order:
    s=open(n).read()
    s=LIC.sub("",s,count=1); s=PRAGMA.sub("",s); s=INC_Q.sub("",s); s=INC_A.sub("",s)
    # A forward declaration of something the sequoia module owns would attach a
    # *different* entity to sequoia.tests; the import already declares it.
    s=re.sub(r'^\s*class test_runner;\s*$', '', s, flags=re.M)
    parts.append(f"// ==== {n} ====\n{s.strip()}\n")
open(sys.argv[1],"w").write(
  "// Generated - do not commit.\nmodule;\n\nexport module sequoia.tests;\n\n"
  "import std;\nimport std.compat;\nexport import sequoia;\n\nexport {\n"+"\n".join(parts)+"\n}\n")
print(f"  {len(order)} test headers -> {sys.argv[1]}", file=sys.stderr)
