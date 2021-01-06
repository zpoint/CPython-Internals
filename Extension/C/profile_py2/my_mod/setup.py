from distutils.core import setup, Extension

my_module = Extension('my_module', sources=['my_module.c'], extra_compile_args=["-std=c99"])

setup(name='my_module',
      version='1.0',
      description='my module to compute my task in a faster way',
      ext_modules=[my_module])

