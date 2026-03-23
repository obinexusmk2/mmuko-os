import sys
from setuptools import Extension, setup
from Cython.Build import cythonize

_libraries = ["m"] if sys.platform != "win32" else []

extensions = [
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
        language="c++",
    ),
]

setup(
    ext_modules=cythonize(extensions, compiler_directives={"language_level": 3}),
)
