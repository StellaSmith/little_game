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

dataclass(repr=True)
class BaseResource:
    path: str

    def __repr__(self):
        return f"<{self.__class__.__name__} {self.path}>"

dataclass(repr=True)
class DirectoryResource(BaseResource):
    entries: list[str]

dataclass(repr=True)
class FileResource(BaseResource):
    data: bytes

def add_directory(path):
    current = DirectoryResource()
    current.path = pathlib.Path(path)
    current.entries = []

    for entry in os.scandir(os.path.join(".", path)):
        if entry.is_dir():
            current.entries.append(pathlib.Path(entry.path))
            yield from add_directory(entry.path)
        elif entry.is_file():
            current.entries.append(pathlib.Path(entry.path))
            yield add_file(entry.path)
    yield current

def add_file(path):
    current = FileResource()
    current.path = pathlib.Path(path)
    with open(os.path.join(".", current.path), "rb") as f:
        current.data = f.read()
    return current

def main():
    print("#include <stddef.h>\n")
    print("enum ResourceType {\n    DIRECTORY_RESOURCE,\n    FILE_RESOURCE\n};\n")
    print("struct BaseResource {\n    ResourceType type;\n    char const *path;\n    char const *basename;\n    size_t size;\n};\n")
    print("struct DirectoryResource : BaseResource {\n    BaseResource const *const *entries;\n};\n")
    print("struct FileResource : BaseResource {\n    unsigned char const *data;\n};\n")
    indices = {}
    for i, entry in enumerate(add_directory(".")):
        indices[entry.path] = i
        path = entry.path.as_posix()
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
            if str(entry.path) == ".":
                print(f"    u8\"/\",")
                print(f"    u8\"/\",")
            else:
                print(f"    &entry_{i}_path[0],")
                print(f"    &entry_{i}_path[{name_index}],")
            if entry.entries:
                print(f"    sizeof(entry_{i}_data),")
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
            if str(entry.path) == ".":
                print(f"    u8\"/\",")
                print(f"    u8\"/\",")
            else:
                print(f"    &entry_{i}_path[0],")
                print(f"    &entry_{i}_path[{name_index}],")
            if entry.data:
                print(f"    sizeof(entry_{i}_data),")
                print(f"    &entry_{i}_data[0],")
            else:
                print(f"    0,")
                print(f"    nullptr,")
            print(f"}};\n")
    print(f"static BaseResource const *const root = &entry_{i};")
    print("namespace resources {")
    print("    BaseResource const *get_root() noexcept")
    print("    {")
    print("        return root;")
    print("    }\n}\n")

def usage():
    print("Usage:\n\t" + sys.executable + " " + sys.argv[0] + " <dir_path>\n", file=sys.stderr)
    print("dir_path\tDirectory containing the resources to compile", file=sys.stderr)
    print(file=sys.stderr)

if __name__ == "__main__":
    if len(sys.argv) < 2:
        usage()
    else:
        os.chdir(sys.argv[1])
        main()