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
        "with_lua": ["lua", "luajit"],
        "with_opengl": [True, False],
        "with_vulkan": [True, False]
    }

    default_options = {
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

    def layout(self):
        cmake_layout(self)

    def requirements(self):
        self.requires("glm/cci.20230113")
        self.requires("entt/3.11.1")
        self.requires("stb/cci.20220909")
        self.requires("ctre/3.7.2")
        self.requires("fmt/10.0.0")
        self.requires("spdlog/1.11.0")
        self.requires("rapidjson/cci.20220822")
        self.requires("imgui/1.89.4")
        self.requires("boost/1.82.0")
        self.requires("sol2/3.3.0")
        self.requires("sdl/2.26.5")
        self.requires("rmlui/4.4")

        if self.options.with_lua == "lua":
            self.requires("lua/5.4.6", override=True)
        elif self.options.with_lua == "luajit":
            self.requires("luajit/2.1.0-beta3")

        if self.options.with_opengl:
            self.requires("glad/0.1.36")

        if self.options.with_vulkan:
            sdk_version = "1.3.243.0"
            self.requires(f"vulkan-headers/{sdk_version}")
            self.requires(f"volk/{sdk_version}")
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
        tc.variables["IMGUI_RES_DIR"] = os.path.join(self.dependencies["imgui"].package_folder, "res")
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
