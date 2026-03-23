from setuptools import Extension, setup
from Cython.Build import cythonize

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
        libraries=["m"],
    )
]

setup(
    ext_modules=cythonize(extensions, compiler_directives={"language_level": 3}),
)
