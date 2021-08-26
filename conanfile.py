from conans import ConanFile, CMake, tools

class VGameConan(ConanFile):
    name = "vgame"
    version = "testing"
    description = "A voxel game"
    license = "GPL-2.0"
    url = homepage = "https://github/StellaSmith/little_game"
    
    settings = "os", "arch", "compiler", "build_type"
    generators = "cmake_find_package", "cmake_paths"

    options = {
        "use_ninja": [True, False]
    }

    default_options = {
        "use_ninja": False,
        "sdl2:shared": True,
        "glad:gl_profile": "core",
        "glad:gl_version": "3.3",
        "glad:extensions": "GL_ARB_get_program_binary,GL_KHR_debug"
    }

    _cmake = None

    def requirements(self):
        if self.settings.os == "Windows":
            # Conan Center only supports SDL2 on windows for the moment 
            self.requires("sdl/2.0.14")
        else:
            self.requires("sdl2/2.0.14@bincrafters/stable")
        self.requires("glm/0.9.9.8")
        self.requires("fmt/8.0.1")
        self.requires("spdlog/1.9.1")
        self.requires("rapidjson/cci.20200410")
        self.requires("entt/3.8.0")
        self.requires("luajit/2.0.5")
        self.requires("sol2/3.2.3")
        self.requires("openssl/1.1.1k")
        self.requires("imgui/1.83")
        self.requires("stb/cci.20210713")
        self.requires("glad/0.1.34")



    def build_requirements(self):
        self.build_requires("cmake/[>=3.14]")
        if self.options.use_ninja:
            self.build_requires("ninja/[>=1.9]")

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()
    
    def package(self):
        self.copy("LICENSE", dst="licenses", src=self.source_folder)
        cmake = self._configure_cmake()
        cmake.install()

    def _configure_cmake(self):
        if self._cmake:
            return self._cmake
        if self.options.use_ninja:
            self._cmake = CMake(self, generator="Ninja")
        else:
            self._cmake = CMake(self)
        if self.version == "testing":
            self._cmake.definitions["CMAKE_EXPORT_COMPILE_COMMANDS"] = "ON"
        self._cmake.configure() # Default
        return self._cmake
