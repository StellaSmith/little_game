from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.build import check_min_cppstd
from conan.tools.scm import Version
from conan.tools.apple import is_apple_os

import functools
import shutil
import io
import re
import os


class VGameRecipe(ConanFile):
    name = "vgame"
    # version = "develop"
    package_type = "application"

    name = "vgame"
    package_type = "application"
    description = "VGame"
    license = "GPL-2.0-only"
    url = homepage = "https://github.com/StellaSmith/little_game"
    author = "Stella Smith"

    settings = "os", "compiler", "build_type", "arch"

    exports_sources = "CMakeLists.txt", "src/*", "include/*", "external/*", "res/*"

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
        "with_vulkan": True,


        "glad/*:gl_profile": "core",
        "glad/*:gl_version": "3.3",
        "glad/*:no_loader": True,
        "glad/*:extensions": "GL_KHR_debug,GL_ARB_get_program_binary",

        "libalsa/*:shared": True,
        "openssl/*:shared": True,

        "sdl/*:pulse": False,
        "sdl/*:shared": True,
    }

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.use_mold

    def layout(self):
        cmake_layout(self)

    @property
    def _min_cmake_version(self):
        return Version("1.18")

    @property
    @functools.lru_cache(1)
    def _requires_cmake(self):
        if cmake := shutil.which("cmake"):
            stdout = io.StringIO()
            self.run([cmake, "--version"], stdout, env="conanbuild", shell=False)
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

    def requirements(self):
        self.requires("glm/cci.20230113")
        self.requires("entt/3.11.1")
        self.requires("stb/cci.20220909")
        self.requires("ctre/3.7.2")
        self.requires("fmt/9.1.0")
        self.requires("spdlog/1.11.0")
        self.requires("rapidjson/cci.20220822")
        self.requires("imgui/1.89.4")
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
            self.requires(f"vulkan-headers/{sdk_version}", override=True)
            self.requires(f"volk/{sdk_version}")
            self.requires("vulkan-memory-allocator/3.0.1")
            if self.settings.build_type != "Release":
                # self.requires(f"vulkan-validationlayers/{sdk_version}", run=True)
                pass
            if is_apple_os(self):
                self.requires("moltenvk/1.2.2")

    def validate(self):
        if self.settings.compiler.get_safe("cppstd"):
            check_min_cppstd(self, 20)

        if self.options.with_opengl:
            if Version(self.dependencies["glad"].options.gl_version) < "3.3":
                raise ConanInvalidConfiguration("OpenGL 3.3 or greater is required")
            if not self.dependencies["sdl"].options.opengl:
                raise ConanInvalidConfiguration(f"OpenGL is enabled but {self.dependencies['sdl'].ref} OpenGL support is disabled")
        if self.options.with_vulkan:
            if not self.dependencies["sdl"].options.vulkan:
                raise ConanInvalidConfiguration(f"Vulkan is enabled but {self.dependencies['sdl'].ref} Vulkan support is disabled")
        

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.variables["WITH_LUA"] = self.options.with_lua
        tc.variables["WITH_OPENGL"] = self.options.with_opengl
        tc.variables["WITH_VULKAN"] = self.options.with_vulkan
        tc.variables["imgui_RES_DIRS"] = os.path.join(self.dependencies["imgui"].package_folder, "res")
        if not self.options.use_mold:
            tc.variables["MOLD_PROGRAM"] = ""
        if not self.options.use_ccache:
            tc.variables["CCACHE_PROGRAM"] = ""
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
