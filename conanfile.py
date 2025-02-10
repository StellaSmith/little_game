from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.build import check_min_cppstd
from conan.tools.scm import Version
from conan.tools.apple import is_apple_os

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
        "with_opengl": [True, False],
    }

    default_options = {
        "with_opengl": True,

        "glad/*:gl_profile": "core",
        "glad/*:gl_version": "3.3",
        "glad/*:no_loader": True,
        "glad/*:extensions": "GL_KHR_debug,GL_ARB_get_program_binary",

        "libalsa/*:shared": True,
        "openssl/*:shared": True,

        "sdl/*:pulse": False,
        "sdl/*:shared": False,
    }

    def layout(self):
        cmake_layout(self)

    def requirements(self):
        self.requires("glm/[~1.0]")
        self.requires("entt/[~3.13]")
        self.requires("fmt/[~11.0]")
        self.requires("spdlog/[~1.15]")
        self.requires("rapidjson/cci.20230929")
        self.requires("imgui/[~1.91]")
        self.requires("boost/[~1.86]")
        self.requires("sdl/[~2.30]", override=True)
        self.requires("sdl_image/[~2]")
        self.requires("range-v3/[~0.12]")
        self.requires("assimp/[~5.4]")

        if self.options.with_opengl:
            self.requires("glad/[~0.1]")

    def validate(self):
        if self.settings.compiler.get_safe("cppstd"):
            check_min_cppstd(self, 20)

        if self.options.with_opengl:
            if Version(self.dependencies["glad"].options.gl_version) < "3.3":
                raise ConanInvalidConfiguration("OpenGL 3.3 or greater is required")
            if not self.dependencies["sdl"].options.opengl:
                raise ConanInvalidConfiguration(f"OpenGL is enabled but {self.dependencies['sdl'].ref} OpenGL support is disabled")

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.variables["WITH_OPENGL"] = self.options.with_opengl
        tc.variables["IMGUI_RES_DIR"] = os.path.join(self.dependencies["imgui"].package_folder, "res")
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
