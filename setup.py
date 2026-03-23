from __future__ import annotations

from pathlib import Path

from setuptools import Extension, setup
from Cython.Build import cythonize

ROOT = Path(__file__).parent.resolve()
BUILD_LIB = ROOT / "build" / "lib"

extensions = [
    Extension(
        name="mmuko_os._firmware",
        sources=["python/mmuko_os/_firmware.pyx"],
        include_dirs=[str(ROOT), str(ROOT / "python")],
        libraries=["nsigii_firmware", "nsigii_firmware_cpp"],
        library_dirs=[str(BUILD_LIB)],
        runtime_library_dirs=[str(BUILD_LIB)],
        language="c++",
        extra_compile_args=["-O2", "-std=c++17"],
    )
]

setup(
    name="mmuko-os",
    version="0.1.0",
    description="Python/Cython compositor for MMUKO-OS NSIGII firmware",
    package_dir={"": "python"},
    packages=["mmuko_os"],
    ext_modules=cythonize(extensions, compiler_directives={"language_level": 3}),
    zip_safe=False,
)
