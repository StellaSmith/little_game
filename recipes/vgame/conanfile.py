import os
import shutil
import sys
import io
import functools
import re
import shlex

from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps, CMake, cmake_layout
from conan.errors import ConanInvalidConfiguration
from conan.tools.files import copy, get, rmdir, rm
from conan.tools.build import check_min_cppstd
from conan.tools.env import VirtualBuildEnv
from conan.tools.apple import is_apple_os
from conan.tools.scm import Version
from conan.api.output import ConanOutput


required_conan_version = ">=1.0"


class VGameConan(ConanFile):
    name = "vgame"
    package_type = "application"
    description = "VGame"
    license = "GPL-2.0-only"
    url = homepage = "https://github.com/StellaSmith/little_game"
    author = "Stella Smith"

    settings = "os", "arch", "compiler", "build_type"

    options = {
        "use_ccache": [True, False],
        "use_mold": [True, False],
        "with_lua": ["lua", "luajit"],
        "with_opengl": [True, False],
        "with_vulkan": [True, False]
    }

    default_options = {
        "use_ccache": True,
        "use_mold": True,

        "with_lua": "lua",
        "with_opengl": True,
        "with_vulkan": False,


        "glad/*:gl_profile": "core",
        "glad/*:gl_version": "3.3",
        "glad/*:no_loader": True,
        "glad/*:extensions": "GL_KHR_debug,GL_ARB_get_program_binary",

        "libalsa/*:shared": True,
        "pulseaudio/*:shared": True,
        "openssl/*:shared": True,
        "sdl/*:shared": True,
    }

    def layout(self):
        cmake_layout(self, src_folder="src")

    def export_sources(self):
        for export in ("LICENSE", "CMakeLists.txt", ("include", "*"), ("src", "*"), ("external", "*"), ("res", "*"), ("assets", "*"), ("cfg", "*"), ("lua", "*")):
            if not isinstance(export, str):
                export = os.path.join(*export)
            copy(self, export, src=os.path.join("..", ".."), dst=self.export_folder)

    @property
    def _min_cmake_version(self):
        return Version("1.16")

    @property
    @functools.lru_cache(1)
    def _requires_cmake(self):
        if cmake := shutil.which("cmake"):
            stdout = io.StringIO()
            self.run(shlex.join([cmake, "--version"]), stdout=stdout, env="conanbuild")
            stdout.seek(0)
            stdout = stdout.read()
            cmake_version = Version(re.match(r"cmake version (\d+.\d+.\d+)", stdout)[1])
            if cmake_version >= self._min_cmake_version:
                return False
        return True

    @property
    @functools.lru_cache(1)
    def _requires_mold(self):
        if cmake := shutil.which("mold"):
            return False
        return True

    @property
    @functools.lru_cache(1)
    def _requires_ninja(self):
        if cmake := shutil.which("ninja"):
            return False
        return True

    @property
    @functools.lru_cache(1)
    def _requires_ccache(self):
        if cmake := shutil.which("ccache"):
            return False
        return True

    def build_requirements(self):
        if self._requires_cmake:
            self.tool_requires(f"cmake/[>={self._min_cmake_version}]")

        if self._requires_ninja:
            self.tool_requires("ninja/[*]")

        if self.options.get_safe("use_mold") and self._requires_mold:
            self.tool_requires("mold/[*]")

        if self.options.get_safe("use_ccache") and self._requires_ccache:
            self.tool_requires("ccache/[*]")

        self.tool_requires(f"vgame_tools/{self.version}")

    def requirements(self):
        self.requires("glm/0.9.9.8")
        self.requires("entt/3.10.3")
        self.requires("stb/cci.20220909")
        self.requires("ctre/3.7.1")
        self.requires("fmt/9.1.0")
        self.requires("spdlog/1.11.0")
        self.requires("rapidjson/cci.20220822")
        self.requires("imgui/1.89.2")
        self.requires("boost/1.81.0")
        self.requires("sol2/3.3.0")
        self.requires("assimp/5.2.2")
        self.requires("sdl/2.26.1")

        if self.options.with_lua == "lua":
            self.requires("lua/5.4.4")
        elif self.options.with_lua == "luajit":
            self.requires("luajit/2.1.0-beta3")

        if self.options.with_opengl:
            self.requires("glad/0.1.36")

        if self.options.with_vulkan:
            sdk_version = "1.3.236.0"
            self.requires(f"volk/{sdk_version}")
            self.requires(f"vulkan-headers/{sdk_version}")
            self.requires("vulkan-memory-allocator/3.0.1")
            if is_apple_os(self):
                self.requires("moltenvk/1.2.1")

    def validate(self):
        if self.settings.get("cppstd"):
            check_min_cppstd(self, 20)

        if self.options.with_opengl and self.options.with_vulkan:
            raise ConanInvalidConfiguration("Either OpenGL or Vulkan can be enabled at the same time for the time being")

        if self.options.with_opengl and tools.Version(self.options["glad"].gl_version) < "3.3":
            raise ConanInvalidConfiguration("OpenGL 3.3 or greater is required")

    def generate(self):
        env = VirtualBuildEnv(self)
        env.generate()

        tc = CMakeToolchain(self)
        tc.variables["WITH_LUA"] = self.options.with_lua
        tc.variables["WITH_OPENGL"] = self.options.with_opengl
        tc.variables["WITH_VULKAN"] = self.options.with_vulkan
        if not self.options.use_mold:
            tc.variables["MOLD_PROGRAM"] = ""
        if not self.options.use_ccache:
            tc.variables["CCACHE_PROGRAM"] = ""
        tc.generate()

        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        copy(self, "LICENSE", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))
        cmake = CMake(self)
        cmake.install()

        rmdir(self, os.path.join(self.package_folder, "lib", "pkgconfig"))
        rmdir(self, os.path.join(self.package_folder, "lib", "cmake"))
        rmdir(self, os.path.join(self.package_folder, "share"))
        rm(self, "*.pdb", os.path.join(self.package_folder, "lib"))
        rm(self, "*.pdb", os.path.join(self.package_folder, "bin"))
