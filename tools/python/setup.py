#!/usr/bin/python

from distutils.core import setup, Extension

m = Extension('linphone',
	include_dirs = ['/home/ghislain/linphone-install/include'],
	sources = ['linphone.c']
)
setup(name = 'Linphone', version = '1.0', description = 'Linphone package', ext_modules = [m])
