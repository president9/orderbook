from conan import ConanFile
from conan.tools.cmake import cmake_layout

class OrderbookConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    def requirements(self):
        self.requires("boost/1.86.0")

    def layout(self):
        cmake_layout(self)
