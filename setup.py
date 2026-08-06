from setuptools import setup, Extension
import sys
import os

sources = [
    'python_api.c',
    'btmha.c',
    'eval.c',
    'optimize.c',
    'state.c',
    'interactive.c',
    'var_list.c',
    'process.c',
    'stream.c',
    'gr_headless.c',
]

include_dirs = ['/usr/local/include']
extra_objects = ['libbtmha.a', '../chapro/libchapro.a']

if sys.platform == 'darwin':
    extra_objects.extend(['/usr/local/lib/libsigpro.a', '/usr/local/lib/libarsc.a'])
    extra_link_args = ['-lz']
else:
    extra_link_args = ['-lsigpro', '-larsc', '-lz']

module = Extension('btmha',
                   sources=sources,
                   include_dirs=include_dirs,
                   extra_objects=extra_objects,
                   extra_link_args=extra_link_args,
                   define_macros=[('MAC', None), ('linux', None), ('ALSA', None), ('ANSI_C', None)] if sys.platform == 'darwin' else [])

setup(name='btmha',
      version='1.0',
      description='BTMHA Python API',
      ext_modules=[module])
