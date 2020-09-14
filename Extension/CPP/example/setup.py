from distutils.core import setup, Extension

my_module = Extension('m_example', sources=['./example.cpp'], extra_compile_args=['-std=c++11'])

setup(name='m_example',
      version='1.0',
      description='my module to use C++ STL',
      ext_modules=[my_module])
