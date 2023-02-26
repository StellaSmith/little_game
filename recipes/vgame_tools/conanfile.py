from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.microsoft import msvc_runtime_flag, is_msvc
from conan.tools.build import check_min_cppstd
from conan.tools.env import VirtualBuildEnv
from conan.tools.files import copy, rmdir, rm
import pathlib
import os


class VGameToolsConan(ConanFile):
    name = "vgame_tools"
    description = "Build tools for vgame"
    package_type = "application"
    license = "GPL-2.0-only"
    url = homepage = "https://github.com/StellaSmith/little_game"
    author = "Stella Smith"

    settings = "os", "arch", "compiler", "build_type"

    @property
    def _project_root(self):
        return str(pathlib.Path(self.recipe_folder).parent.parent)

    def layout(self):
        cmake_layout(self, "src")

    def export_sources(self):
        copy(self, "CMakeLists.txt", src=os.path.join(self._project_root, "tools"), dst=self.export_sources_folder)
        copy(self, "*", src=os.path.join(self._project_root, "tools", "src"), dst=os.path.join(self.export_sources_folder, "src"))

    def requirements(self):
        self.requires("argparse/2.4")
        self.requires("fmt/9.1.0")
        self.requires("rapidjson/cci.20220822")
        self.requires("boost/1.80.0")
        self.requires("ctre/3.7.1")

    def validate(self):
        if self.settings.get_safe("compiler.cppstd"):
            check_min_cppstd(self, 20)

    def build_requirements(self):
        self.tool_requires("cmake/[>=3.16]")
        self.tool_requires("ninja/[*]")

    def generate(self):
        env = VirtualBuildEnv(self)
        env.generate()

        tc = CMakeToolchain(self)
        tc.generate()

        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        copy(self, "LICENSE", src=self._project_root, dst=os.path.join(self.package_folder, "licenses"))
        cmake = CMake(self)
        cmake.install()

        rmdir(self, os.path.join(self.package_folder, "lib", "pkgconfig"))
        rmdir(self, os.path.join(self.package_folder, "lib", "cmake"))
        rmdir(self, os.path.join(self.package_folder, "share"))
        rm(self, pattern="*.pdb", folder=os.path.join(self.package_folder, "lib"))
        rm(self,  pattern="*.pdb", folder=os.path.join(self.package_folder, "bin"))
