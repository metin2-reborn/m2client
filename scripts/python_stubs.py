#!/usr/bin/env python3
"""Generates .pyi stubs for the client's embedded Python modules (the `metin2` package) from the
C++ sources, so an editor's language server knows the exports the game scripts call.

    python_stubs.py [--src src/metin2] [--out ../m2game/typings]

Every file that calls init_python_module("metin2.x", table) contributes one module: the entries of
its PyMethodDef table become functions whose parameters are read from the PyArg_ParseTuple format
(and their names from the C variables it fills), the return type from Py_BuildValue / Py_RETURN_*,
and the PyModule_AddIntConstant calls that follow the registration become module constants. A
comment right above a function becomes its docstring. Window handles (O& with a converter) are
ints, as the scripts pass them. Anything the heuristics cannot read falls back to *args: Any.
"""

import argparse
import keyword
import os
import re
import sys
from collections import defaultdict

INIT_RE = re.compile(r'init_python_module\("([a-z0-9_.]+)",\s*([A-Za-z_][A-Za-z0-9_]*)\)')
TABLE_RE = re.compile(r'PyMethodDef\s+([A-Za-z_][A-Za-z0-9_]*)\s*\[\]\s*=\s*\{')
ENTRY_RE = re.compile(r'\{\s*"([A-Za-z0-9_]+)"\s*,\s*([A-Za-z_][A-Za-z0-9_:<>]*)\s*,\s*(METH_[A-Z_|\s]+)')
CONST_RE = re.compile(r'PyModule_AddIntConstant\(\s*module\s*,\s*"([A-Za-z0-9_]+)"')
PARSE_RE = re.compile(r'PyArg_ParseTuple\(\s*args\s*,\s*"([^"]*)"((?:\s*,\s*[^,)]+)*)\)')
BUILD_RE = re.compile(r'Py_BuildValue\(\s*"([^"]*)"')

# PyArg_ParseTuple format units -> Python types.
ARG_TYPES = {
    "i": "int", "l": "int", "h": "int", "b": "int", "B": "int", "H": "int", "I": "int", "k": "int",
    "K": "int", "n": "int", "L": "int", "c": "str", "f": "float", "d": "float", "s": "str",
    "z": "str | None", "u": "str", "O": "object", "O&": "int", "S": "str", "U": "str",
}

# Py_BuildValue units -> Python types.
BUILD_TYPES = {
    "i": "int", "l": "int", "h": "int", "b": "int", "B": "int", "H": "int", "I": "int", "k": "int",
    "K": "int", "n": "int", "L": "int", "c": "str", "f": "float", "d": "float", "s": "str",
    "z": "str | None", "u": "str", "O": "object", "N": "object", "S": "object",
}


def read(path):
    with open(path, encoding="utf-8", errors="replace") as f:
        return f.read()


def block_after(text, start):
    """The text from the first '{' at or after start to its matching '}' (exclusive)."""
    begin = text.find("{", start)
    if begin < 0:
        return ""
    depth = 0
    i = begin
    while i < len(text):
        c = text[i]
        if c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
            if depth == 0:
                return text[begin + 1:i]
        elif c == '"':
            i += 1
            while i < len(text) and text[i] != '"':
                if text[i] == "\\":
                    i += 1
                i += 1
        elif text.startswith("//", i):
            i = text.find("\n", i)
            if i < 0:
                break
        elif text.startswith("/*", i):
            i = text.find("*/", i) + 1
        i += 1
    return text[begin + 1:]


def function_body(text, name):
    """Body of `auto name(PyObject *self, PyObject *args) -> PyObject *` and the comment above it."""
    m = re.search(r'(?:^|\n)((?:[ \t]*//[^\n]*\n)*)[ \t]*(?:static\s+)?auto\s+' + re.escape(name) + r'\s*\(\s*PyObject\s*\*\s*\w*\s*,\s*PyObject\s*\*\s*\w*\s*\)\s*->\s*PyObject\s*\*\s*\{', text)
    if not m:
        return None, ""
    comment = " ".join(line.strip()[2:].strip() for line in m.group(1).strip().splitlines() if line.strip().startswith("//"))
    return block_after(text, m.end() - 1), comment


def split_format(fmt):
    """The units of a PyArg_ParseTuple format, with '|' marking where optionals start."""
    fmt = fmt.split(":")[0].split(";")[0]
    units = []
    i = 0
    while i < len(fmt):
        c = fmt[i]
        if c in "|$":
            units.append(c)
            i += 1
        elif c == "O" and i + 1 < len(fmt) and fmt[i + 1] in "&!":
            units.append("O&" if fmt[i + 1] == "&" else "O!")
            i += 2
        elif c in "sz" and i + 1 < len(fmt) and fmt[i + 1] in "#*":
            units.append(c + fmt[i + 1])
            i += 2
        elif c in "() ":
            units.append(c)
            i += 1
        else:
            units.append(c)
            i += 1
    return units


def parameters(body):
    """[(name, type, optional)] from the first PyArg_ParseTuple in the body, or None."""
    m = PARSE_RE.search(body)
    if not m:
        if "PyArg_ParseTuple" in body:
            return None
        return []
    units = split_format(m.group(1))
    targets = [t.strip() for t in m.group(2).split(",") if t.strip()]
    params = []
    optional = False
    depth = 0
    ti = 0

    def take():
        nonlocal ti
        if ti >= len(targets):
            raise ValueError("format longer than its arguments")
        t = targets[ti]
        ti += 1
        return t

    for u in units:
        if u == "|" or u == "$":
            optional = True
            continue
        if u == "(":
            depth += 1
            continue
        if u == ")":
            depth -= 1
            continue
        if u == " ":
            continue
        if u in ("O&", "O!"):
            take()  # the converter or type object
            target = take()
            typ = "int" if u == "O&" else "object"
        elif u in ("s#", "z#", "s*", "z*"):
            target = take()
            take()  # the length
            typ = "str"
        else:
            typ = ARG_TYPES.get(u)
            if typ is None:
                return None
            target = take()
        name = re.sub(r"^&", "", target).strip()
        name = re.sub(r"\[.*\]$", "", name)
        # A group "(ffff)" fills the members of one struct: the tuple is named after the struct.
        name = name.split(".")[0] if depth > 0 else name.split(".")[-1].split("->")[-1].strip()
        if not re.match(r"^[A-Za-z_][A-Za-z0-9_]*$", name):
            name = "arg%d" % (len(params) + 1)
        if keyword.iskeyword(name):
            name += "_"
        if depth > 0:
            typ = "tuple"  # a parenthesised group reads one tuple argument
        params.append((name, typ, optional))
    # A parenthesised group maps several C variables to one Python argument: merge them.
    merged = []
    for name, typ, opt in params:
        if typ == "tuple" and merged and merged[-1][1] == "tuple":
            continue
        merged.append((name, typ, opt))
    return merged


def build_type(fmt):
    units = split_format(fmt)
    types = []
    for u in units:
        if u in "() |$":
            continue
        t = BUILD_TYPES.get(u)
        if t is None:
            return "object"
        types.append(t)
    if fmt.startswith("(") or len(types) != 1:
        return "tuple[%s]" % ", ".join(types) if types else "tuple[()]"
    return types[0]


def return_type(body):
    found = []
    if re.search(r"\bPy_RETURN_NONE\b", body) or re.search(r"\bPy_RETURN_NONE\b", body):
        found.append("None")
    if re.search(r"\bPy_RETURN_(TRUE|FALSE)\b", body) or "PyBool_FromLong" in body:
        found.append("bool")
    for m in BUILD_RE.finditer(body):
        found.append(build_type(m.group(1)))
    if re.search(r"\bPy(Int|Long)_From", body):
        found.append("int")
    if re.search(r"\bPyFloat_From", body):
        found.append("float")
    if re.search(r"\bPy(String|Unicode)_From", body):
        found.append("str")
    if re.search(r"\bPyList_New\b", body):
        found.append("list")
    if re.search(r"\bPyTuple_New\b", body):
        found.append("tuple")
    if re.search(r"\bPyDict_New\b", body):
        found.append("dict")
    unique = []
    for t in found:
        if t not in unique:
            unique.append(t)
    if not unique:
        return "Any"
    if len(unique) > 3:
        return "Any"
    return " | ".join(unique)


def collect(src_dir):
    """{module name: {"functions": [(py name, params, return, doc)], "constants": [names]}}"""
    modules = {}
    for root, _dirs, files in os.walk(src_dir):
        if os.sep + "dev" in root:
            continue
        for file_name in sorted(files):
            if not file_name.endswith(".cpp"):
                continue
            text = read(os.path.join(root, file_name))
            inits = list(INIT_RE.finditer(text))
            if not inits:
                continue
            tables = {}
            for tm in TABLE_RE.finditer(text):
                tables[tm.group(1)] = block_after(text, tm.end() - 1)
            for index, im in enumerate(inits):
                name, table = im.group(1), im.group(2)
                functions = []
                for em in ENTRY_RE.finditer(tables.get(table, "")):
                    py_name, c_name, flags = em.group(1), em.group(2), em.group(3)
                    # A template instantiation (register_window<widgets::button>) reads like its template.
                    body, doc = function_body(text, c_name.split("<")[0])
                    if body is None:
                        params, ret = None, "Any"
                    else:
                        params, ret = parameters(body), return_type(body)
                        if "METH_NOARGS" in flags:
                            params = []
                    functions.append((py_name, params, ret, doc))
                end = inits[index + 1].start() if index + 1 < len(inits) else len(text)
                constants = []
                for cm in CONST_RE.finditer(text, im.end(), end):
                    if cm.group(1) not in constants:
                        constants.append(cm.group(1))
                entry = modules.setdefault(name, {"functions": [], "constants": [], "source": set()})
                entry["functions"].extend(functions)
                entry["constants"].extend(c for c in constants if c not in entry["constants"])
                entry["source"].add(os.path.relpath(os.path.join(root, file_name), src_dir))
    return modules


def render(name, module, children):
    lines = ['"""%s: generated by m2client/scripts/python_stubs.py from %s. Do not edit."""' % (name, ", ".join(sorted(module["source"])) if module else "the module tree"),
             "", "from typing import Any", ""]
    if children:
        lines.append("from . import " + ", ".join("%s as %s" % (c, c) for c in sorted(children)))
        lines.append("")
    if module:
        for const in module["constants"]:
            lines.append("%s: int" % const)
        if module["constants"]:
            lines.append("")
        for py_name, params, ret, doc in module["functions"]:
            if params is None:
                signature = "*args: Any"
            else:
                parts = []
                for p_name, p_type, optional in params:
                    parts.append("%s: %s%s" % (p_name, p_type, " = ..." if optional else ""))
                signature = ", ".join(parts)
            lines.append("def %s(%s) -> %s:" % (py_name, signature, ret))
            if doc:
                lines.append('    """%s"""' % doc.replace('"""', "'''"))
            lines.append("    ...")
            lines.append("")
    return "\n".join(lines).rstrip() + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    here = os.path.dirname(os.path.abspath(__file__))
    parser.add_argument("--src", default=os.path.join(here, "..", "src", "metin2"))
    parser.add_argument("--out", default=os.path.join(here, "..", "..", "m2game", "typings"))
    args = parser.parse_args()

    modules = collect(args.src)
    if not modules:
        print("no init_python_module calls found under %s" % args.src, file=sys.stderr)
        return 1

    # Every dotted prefix is a package; a registered module that has children is its __init__.pyi.
    names = set(modules)
    for name in list(names):
        parts = name.split(".")
        for i in range(1, len(parts)):
            names.add(".".join(parts[:i]))
    children = defaultdict(set)
    for name in names:
        if "." in name:
            parent, leaf = name.rsplit(".", 1)
            children[parent].add(leaf)

    written = 0
    unknown = 0
    for name in sorted(names):
        parts = name.split(".")
        if children[name]:
            path = os.path.join(args.out, *parts, "__init__.pyi")
        else:
            path = os.path.join(args.out, *parts[:-1], parts[-1] + ".pyi")
        os.makedirs(os.path.dirname(path), exist_ok=True)
        module = modules.get(name)
        with open(path, "w", encoding="utf-8") as f:
            f.write(render(name, module, children[name]))
        written += 1
        if module:
            unknown += sum(1 for _n, params, _r, _d in module["functions"] if params is None)
    total = sum(len(m["functions"]) for m in modules.values())
    constants = sum(len(m["constants"]) for m in modules.values())
    print("%d stub files, %d modules, %d functions (%d without a readable signature), %d constants -> %s" % (written, len(modules), total, unknown, constants, os.path.abspath(args.out)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
