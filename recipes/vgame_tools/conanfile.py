from conans import CMake, ConanFile, tools
from conans.errors import ConanInvalidConfiguration
from conan.tools.microsoft import msvc_runtime_flag, is_msvc
import functools
import os
import textwrap

required_conan_version = ">=1.0"

class VGameToolsConan(ConanFile):
    name = "vgame_tools"
    description = "Build tools for vgame"
    license = "GPL-2.0-only"
    url = homepage = "https://github.com/StellaSmith/little_game"
    author = "Stella Smith"

    settings = "os", "arch", "compiler"
    generators = "cmake", "cmake_find_package"

    @property
    def _source_subfolder(self):
        return "source_subfolder"

    @property
    def _build_subfolder(self):
        return "build_subfolder"

    def export_sources(self):
        self.copy("CMakeLists.txt")
        for export in (("..", "LICENSE"), "CMakeLists.txt", ("src", "*")):
            if not isinstance(export, str):
                export = os.path.join(*export)
            self.copy(export, src=os.path.join("..", "..", "tools"), dst=self._source_subfolder)

    def requirements(self):
        self.requires("argparse/2.4")
        self.requires("fmt/8.1.1")
        self.requires("rapidjson/cci.20211112")
        self.requires("boost/1.79.0")
        self.requires("ctre/3.6")

    def validate(self):
        tools.check_min_cppstd(self, 20)

    def build_requirements(self):
        self.tool_requires("cmake/[>=3.16]")
        self.tool_requires("ninja/[*]")

    def _patch_sources(self):
        tools.save("CMakeLists.txt", textwrap.dedent(f"""\
            cmake_minimum_required(VERSION 3.0)
            project(cmake_wrapper)
            include(conanbuildinfo.cmake)
            conan_basic_setup(TARGETS KEEP_RPATHS)
            add_subdirectory(source_subfolder)
        """))

    @functools.lru_cache(1)
    def _configure_cmake(self):
        cmake = CMake(self, build_type="Release")
        cmake.configure(build_folder=self._build_subfolder)
        return cmake

    def build(self):
        self._patch_sources()
        cmake = self._configure_cmake()
        cmake.build()

    def package(self):
        self.copy(pattern="LICENSE", dst="licenses", src=self._source_subfolder)
        cmake = self._configure_cmake()
        cmake.install()

        tools.rmdir(os.path.join(self.package_folder, "lib", "pkgconfig"))
        tools.rmdir(os.path.join(self.package_folder, "lib", "cmake"))
        tools.rmdir(os.path.join(self.package_folder, "share"))
        tools.remove_files_by_mask(os.path.join(self.package_folder, "lib"), "*.pdb")
        tools.remove_files_by_mask(os.path.join(self.package_folder, "bin"), "*.pdb")

    def package_info(self):
        bindir = os.path.join(self.package_folder, "bin")
        self.output.info("Appending PATH environment variable: {}".format(bindir))
        self.env_info.PATH.append(bindir)
