from conans import CMake, ConanFile, tools
from conans.errors import ConanInvalidConfiguration
import functools
import os
import textwrap

required_conan_version = ">=1.0"

class VGameConan(ConanFile):
    name = "vgame"
    description = "VGame"
    license = "GPL-2.0-only"
    url = homepage = "https://github.com/StellaSmith/little_game"
    author = "Stella Smith"

    settings = "os", "arch", "compiler", "build_type"
    generators = "cmake", "cmake_find_package"

    options = {
        "use_ccache": [True, False],
        "use_mold": [True, False],
        "with_luajit": [True, False],
        "with_opengl": [True, False],
        "with_vulkan": [True, False]
    }

    default_options = {
        "use_ccache": True,
        "use_mold": False,
        "with_luajit": False,
        "with_opengl": True,
        "with_vulkan": False,

        "libalsa:shared": True,
        "pulseaudio:shared": True,
        "glad:gl_profile": "core",
        "glad:gl_version": "3.3",
        "glad:no_loader": True,
        "glad:extensions": "GL_KHR_debug,GL_ARB_get_program_binary",
    }

    @property
    def _source_subfolder(self):
        if self.in_local_cache:
            return "source_subfolder"
        else:
            return os.path.join(self.recipe_folder, "..", "..")

    @property
    def _build_subfolder(self):
        if self.in_local_cache:
            return "build_subfolder"
        else:
            return self.build_folder

    @property
    def _settings_build(self):
        return getattr(self, "settings_build", self.settings)

    @property
    def _user_info_build(self):
        return getattr(self, "user_info_build", self.deps_user_info)

    def export_sources(self):
        if self.in_local_cache:
            for export in ("LICENSE", "CMakeLists.txt", ("include", "*"), ("src", "*"), ("external", "*"), ("res", "*"), ("assets", "*"), ("cfg", "*"), ("lua", "*")):
                if not isinstance(export, str):
                    export = os.path.join(*export)
                self.copy(export, src=os.path.join("..", ".."), dst=self._source_subfolder)

    def requirements(self):
        self.requires("glm/0.9.9.8")
        self.requires("entt/3.9.0")
        self.requires("stb/cci.20210910")
        self.requires("ctre/3.6")

        self.requires("fmt/8.1.1")
        self.requires("spdlog/1.9.2")
        self.requires("rapidjson/cci.20211112")
        if self.options.with_luajit:
            self.requires("luajit/2.0.5")
        else:
            self.requires("lua/5.4.4")
        self.requires("sol2/3.2.3")
        self.requires("imgui/1.86")
        self.requires("assimp/5.2.2")
        self.requires("sdl/2.0.20")
        self.requires("boost/1.78.0")

        if self.options.with_opengl:
            self.requires("glad/0.1.35")
        if self.options.with_vulkan:
            self.requires("vulkan-headers/1.3.211.0")
            if self.settings.os in ("Macos", "iOS", "tvOS", "watchOS"):
                self.requires("moltenvk/1.1.9")

    def validate(self):
        tools.check_min_cppstd(self, 20)

        if self.options.with_opengl and self.options.with_vulkan:
            raise ConanInvalidConfiguration("Either OpenGL or Vulkan can be enabled at the same time for the time being")
        
        if self.options.with_opengl and tools.Version(self.options["glad"].gl_version) < "3.3":
            raise ConanInvalidConfiguration("OpenGL 3.3 or greater is required")

    def config_options(self):
        if self.options.use_mold:
            # mold is not in CCI
            # it requires onetbb 2021.5 which is not in CCI either
            raise ConanInvalidConfiguration("mold is not supperted with this recipe yet")

    def build_requirements(self):
        self.tool_requires(f"vgame_tools/{self.version}")
        if self.options.get_safe("use_ccache"):
            self.tool_requires("ccache/[*]")
        if self.options.get_safe("use_mold"):
            self.tool_requires("mold/[*]")
        self.tool_requires("cmake/[>=3.16]")
        self.tool_requires("ninja/[*]")

    def _patch_sources(self):
        if self.in_local_cache:
            tools.replace_in_file(os.path.join(self._source_subfolder, "CMakeLists.txt"), 
                "include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)",
                "include(${CMAKE_CURRENT_LIST_DIR}/../conanbuildinfo.cmake)")

    @functools.lru_cache(1)
    def _configure_cmake(self):
        cmake = CMake(self)
        cmake.definitions["USE_CONAN"] = True
        cmake.definitions["WITH_LUAJIT"] = self.options.with_luajit
        cmake.definitions["WITH_OPENGL"] = self.options.with_opengl
        cmake.definitions["WITH_VULKAN"] = self.options.with_vulkan
        if not self.options.use_mold:
            cmake.definitions["MOLD_PROGRAM"] = ""
        if not self.options.use_ccache:
            cmake.definitions["CCACHE_PROGRAM"] = ""
    
        
        cmake.configure(source_folder=self._source_subfolder, build_folder=self._build_subfolder)
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
