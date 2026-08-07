from conan import ConanFile
from conan.tools.cmake import cmake_layout


class ExampleRecipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    def requirements(self):
        self.requires("gtest/1.17.0")
        self.requires("glaze/7.8.4")
        self.requires("sqlitecpp/3.3.3")
        self.requires("libpqxx/8.0.2")

    def layout(self):
        cmake_layout(self)
