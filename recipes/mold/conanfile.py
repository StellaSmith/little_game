from conans import CMake, ConanFile, tools
from conans.errors import ConanInvalidConfiguration
import functools
import os
import textwrap

required_conan_version = ">=1.0"

class MoldConan(ConanFile):
    name = "mold"
    description = "A Modern Linker"
    license = "AGPL-3.0"
    homepage = "https://github.com/StellaSmith/little_game"
    url = "https://github.com/rui314/mold/"
    settings = "os", "arch", "compiler"
    generators = "cmake", "cmake_find_package"

    options = {
        "with_mimalloc": [True, False],
        "with_lto": [True, False]
    }

    default_options = {
        "with_mimalloc": True,
        "with_lto": True
    }

    @property
    def _source_subfolder(self):
        return "source_subfolder"

    @property
    def _build_subfolder(self):
        return "build_subfolder"

    @property
    def _settings_build(self):
        return getattr(self, "settings_build", self.settings)

    @property
    def _user_info_build(self):
        return getattr(self, "user_info_build", self.deps_user_info)

    def requirements(self):
        self.requires("onetbb/2021.3.0")
        self.requires("zlib/1.2.12")
        if self.options.with_mimalloc:
            self.requires("mimalloc/2.0.3")
        if self.settings.os in ("Macos", "iOS", "tvOS", "watchOS"):
            self.requires("openssl/1.1.1n")
        

    def validate(self):
        tools.check_min_cppstd(self, 20)

    def build_requirements(self):
        self.tool_requires("pkgconf/1.7.4")

    def export_sources(self):
        self.copy("CMakeLists.txt")

    def source(self):
        tools.get(**self.conan_data["sources"][self.version],
                  destination=self._source_subfolder, strip_root=True)

    def _patch_sources(self):
        pass

    @functools.lru_cache(1)
    def _configure_cmake(self):
        cmake = CMake(self, build_type="Release")
        cmake.definitions["MOLD_VERSION"] = self.version
        cmake.definitions["WITH_MIMALLOC"] = self.options.with_mimalloc
        cmake.definitions["WITH_LTO"] = self.options.with_lto
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