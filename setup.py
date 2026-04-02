import sys
from setuptools import Extension, setup
from Cython.Build import cythonize

_libraries = ["m"] if sys.platform != "win32" else []
# Suppress MSVC C4244 (double→float conversion in Cython-generated code)
_c_extra   = ["/wd4244"] if sys.platform == "win32" else []
# C++ extension also needs C++17 for nested namespace syntax
_cxx_extra = ["/wd4244", "/std:c++17"] if sys.platform == "win32" else ["-std=c++17"]

extensions = [
    # ----------------------------------------------------------------
    # bios_firmware: BIOS RTC + SpinPair + MosaicMemory bindings
    # Sources: bios_firmware.pyx + firmware/bios_interface.c
    # ----------------------------------------------------------------
    Extension(
        name="mmuko_os.bios_firmware",
        sources=[
            "python/mmuko_os/bios_firmware.pyx",
            "firmware/bios_interface.c",
        ],
        include_dirs=[".", "firmware"],
        libraries=_libraries,
        extra_compile_args=_c_extra,
    ),
    Extension(
        name="mmuko_os.firmware",
        sources=[
            "python/mmuko_os/firmware.pyx",
            "heartfull_membrane.c",
            "bzy_mpda.c",
            "tripartite_discriminant.c",
        ],
        include_dirs=["."],
        libraries=_libraries,
        extra_compile_args=_c_extra,
    ),
    Extension(
        name="mmuko_os._firmware",
        sources=[
            "python/mmuko_os/_firmware.pyx",
            "heartfull_membrane.c",
            "bzy_mpda.c",
            "tripartite_discriminant.c",
            "nsigii_cpp_wrapper.cpp",
        ],
        include_dirs=["."],
        libraries=_libraries,
        extra_compile_args=_cxx_extra,
        language="c++",
    ),
]

setup(
    ext_modules=cythonize(extensions, compiler_directives={"language_level": 3}),
)
