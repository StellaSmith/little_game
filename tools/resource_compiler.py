"""
Takes an input directory and shoves all of its contents to a C++ source file
This makes a simple file system on memory embeded into the executable itself
"""

import os
import pathlib
import sys
import json
from typing import *
from dataclasses import dataclass
import subprocess


@dataclass(repr=True)
class BaseResource:
    path: str

@dataclass()
class DirectoryResource(BaseResource):
    entries: List[str]

@dataclass()
class FileResource(BaseResource):
    data: bytes

def add_directory(path: Union[str, pathlib.Path]):
    current = DirectoryResource(pathlib.Path(path), [])

    print(f"scanning directory {os.path.relpath(current.path, project_dir)}", file=sys.stderr)
    for entry in os.scandir(os.path.join(".", path)):
        if entry.is_dir():
            current.entries.append(pathlib.Path(entry.path))
            yield from add_directory(entry.path)
        elif entry.is_file():
            current.entries.append(pathlib.Path(entry.path))
            yield add_file(entry.path)
    print(f"including directory {os.path.relpath(current.path, project_dir)} as is", file=sys.stderr)
    yield current

preprocessors = {}

def add_file(path: Union[str, pathlib.Path]) -> FileResource:
    current = FileResource(pathlib.Path(path), b"")

    extension = os.path.splitext(current.path.name)[-1][1:]

    with open(path, "rb") as f:
        if extension in preprocessors:
            print(f"preprocessing file {os.path.relpath(current.path, project_dir)} with {os.path.relpath(preprocessors[extension], project_dir)}", file=sys.stderr)
            p = subprocess.Popen([sys.executable, preprocessors[extension]], stdin=f, stdout=subprocess.PIPE, text=False)
            output, errors = p.communicate()
            if p.returncode != 0:
                exit(-1)
            current.data = output
        else:
            print(f"including file {os.path.relpath(current.path, project_dir)} as is", file=sys.stderr)
            current.data = f.read()

    return current

def load_preprocessors():
    for pp in os.scandir(os.path.abspath( os.path.join(__file__, "..", "rc"))):
        path = pathlib.Path(pp.path)
        name = os.path.splitext(path.name)[0]
        preprocessor = name.rsplit("_", 1)[0]
        preprocessors[preprocessor] = str(path)
        

def main():
    print("#include <stddef.h>\n")
    print("namespace resources {\n")
    print("enum ResourceType {\n    DIRECTORY_RESOURCE,\n    FILE_RESOURCE\n};\n")
    print("struct BaseResource {\n    ResourceType type;\n    char const *path;\n    char const *basename;\n    size_t size;\n};\n")
    print("struct DirectoryResource : BaseResource {\n    BaseResource const *const *entries;\n};\n")
    print("struct FileResource : BaseResource {\n    unsigned char const *data;\n};\n")
    print("BaseResource const *get_root() noexcept;\n")
    print("}\n")
    print("#if defined(COMPILE_RESOURCES)\n")
    indices = {}
    print("namespace resources {\n")
    for i, entry in enumerate(add_directory(".")):
        indices[entry.path] = i
        path = entry.path.as_posix()
        if path == ".":
            path = "/"

        print(f"static char const entry_{i}_path[] = {json.dumps(path)};")
        name_index = path.rindex(entry.path.name)
        if isinstance(entry, DirectoryResource):
            if entry.entries:
                print(f"static BaseResource const *const entry_{i}_data[] = {{")
                for e in sorted(entry.entries):
                    print(f"    &entry_{indices[e]},")
                print(f"}};")
            print(f"static DirectoryResource const entry_{i} = {{")
            print(f"    DIRECTORY_RESOURCE,")
            print(f"    &entry_{i}_path[0],")
            print(f"    &entry_{i}_path[{name_index}],")
            if entry.entries:
                print(f"    sizeof(entry_{i}_data) / sizeof(entry_{i}_data[0]),")
                print(f"    &entry_{i}_data[0],")
            else:
                print(f"    0,")
                print(f"    nullptr,")
            print(f"}};\n")
        elif isinstance(entry, FileResource):
            if entry.data:
                print(f"static unsigned char const entry_{i}_data[] = {{")
                for j in range(0, len(entry.data), 16):
                    l = (str(x) for x in entry.data[j:j+16])
                    print(f"    {', '.join(l)},")
                print(f"}};")
            print(f"static FileResource const entry_{i} = {{")
            print(f"    FILE_RESOURCE,")
            print(f"    &entry_{i}_path[0],")
            print(f"    &entry_{i}_path[{name_index}],")
            if entry.data:
                print(f"    sizeof(entry_{i}_data),")
                print(f"    &entry_{i}_data[0],")
            else:
                print(f"    0,")
                print(f"    nullptr,")
            print(f"}};\n")
    print(f"BaseResource const *get_root() noexcept")
    print(f"{{")
    print(f"    return &entry_{i};")
    print(f"}}\n")
    print("}\n")
    print("#endif")

def usage():
    print("Usage:\n\t" + sys.executable + " " + sys.argv[0] + " [dir_path]\n", file=sys.stderr)
    print("dir_path\tDirectory containing the resources to compile, defaults to cwd", file=sys.stderr)
    print(file=sys.stderr)

if __name__ == "__main__":
    project_dir = pathlib.Path(__file__) / ".." / ".."
    if len(sys.argv) == 2:
        if sys.argv[1] in ("-h", "--help"):
            usage()
            exit()
        else:
            os.chdir(sys.argv[1])
    load_preprocessors()
    main()