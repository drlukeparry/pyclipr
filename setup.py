import os
import re
import sys
import platform
import subprocess

from setuptools import setup, Extension, find_packages
from setuptools.command.build_ext import build_ext
from distutils.version import LooseVersion

from setuptools.command.develop import develop
from setuptools.command.install import install
from setuptools.command.sdist import sdist
from setuptools.command.build_py import build_py
from setuptools import setup, find_packages
from subprocess import check_call, check_output

with open('README.rst') as f:
    readme = f.read()

""" Specific versions of submodules to use. """

submoduleVersions = {
    'clipper2': 'Clipper2_1.3.0',
    'pybind11': 'v2.11',
    'eigen': '3.4'
}

class CMakeExtension(Extension):
    def __init__(self, name, modName, sourcedir=''):

        Extension.__init__(self, name, sources=[], language='c++')

        self.modName = modName
        self.sourcedir = os.path.abspath(sourcedir)


def gitcmd_update_submodules():
    """
    Check if the package is being deployed as a git repository. If so, recursively update all dependencies.
    """

    if os.path.exists('.git'):

        check_call(['git', 'submodule', 'update', '--init', '--recursive'])

        # set all versions as required
        curPath = os.path.abspath('.')

        for k, v in submoduleVersions.items():
            modPath =  os.path.abspath('{:s}/external/{:s}'.format(curPath, k))

            import subprocess

            process = subprocess.Popen(['git', 'checkout', v], cwd=modPath)


        check_call(['git', 'submodule', 'status'])

        return True

    return False


class gitcmd_develop(develop):
    """
    Specialized packaging class that runs `git submodule update --init --recursive`
    as part of the update/install procedure.
    """
    def run(self):
        gitcmd_update_submodules()
        develop.run(self)


class gitcmd_install(install):
    """
    Specialized packaging class that runs `git submodule update --init --recursive`
    as part of the update/install procedure.
    """
    def run(self):
        gitcmd_update_submodules()
        install.run(self)

class gitcmd_sdist(sdist):
    """
    Specialized packaging class that runs git submodule update --init --recursive
    as part of the update/install procedure;.
    """
    def run(self):
        gitcmd_update_submodules()
        sdist.run(self)

class CMakeBuild(build_ext):
    def run(self):

        # Update the submodules to the correct version
        gitcmd_update_submodules()

        try:
            out = subprocess.check_output(['cmake', '--version'])
        except OSError:
            raise RuntimeError("CMake must be installed to build the following extensions: " +
                               ", ".join(e.name for e in self.extensions))

        if platform.system() == "Windows":
            cmake_version = LooseVersion(re.search(r'version\s*([\d.]+)', out.decode()).group(1))
            if cmake_version < '3.12.0':
                raise RuntimeError("CMake >= 3.12.0 is required on Windows")

        for ext in self.extensions:
            self.build_extension(ext)

    def build_extension(self, ext):
        """
        Builds the following CMake extension within the project tree

        :param ext: CMake Extension to build
        """
        print('ext_full_path', self.get_ext_fullpath(ext.name))
        extdir = os.path.abspath(os.path.dirname(self.get_ext_fullpath(ext.name)))
        # required for auto-detection of auxiliary "native" libs
        if not extdir.endswith(os.path.sep):
            extdir += os.path.sep

        cmake_args = ['-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=' + extdir,
                      '-DPYTHON_EXECUTABLE=' + sys.executable,
                      '-DBUILD_PYTHON=ON']

        cfg = 'Debug' if self.debug else 'Release'
        build_args = ['--config', cfg]

        if platform.system() == "Windows":
            cmake_args += ['-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{}={}'.format(cfg.upper(), extdir)]
            if sys.maxsize > 2**32:
                cmake_args += ['-A', 'x64']
            build_args += ['--', '/m']
        else:
            cmake_args += ['-DCMAKE_BUILD_TYPE=' + cfg]
            build_args += ['--', '-j4']

        cmake_args += ['-DCMAKE_INSTALL_PREFIX=' + '.']

        env = os.environ.copy()
        env['CXXFLAGS'] = '{} -DVERSION_INFO=\\"{}\\"'.format(env.get('CXXFLAGS', ''),
                                                              self.distribution.get_version())
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
        subprocess.check_call(['cmake', ext.sourcedir] + cmake_args, cwd=self.build_temp, env=env)
        subprocess.check_call(['cmake', '--build', '.', '--target', ext.modName] + build_args, cwd=self.build_temp)


setup(
    name='pyclipr',
    version='0.1.7',
    author='Luke Parry',
    author_email='dev@lukeparry.uk',
    url='https://github.com/drlukeparry/pyclipr',
    long_description=readme,
    long_description_content_type = 'text/x-rst',
    description='Python library for polygon clipping and offsetting based on Clipper2.',
    ext_modules=[CMakeExtension('pyclipr.pyclipr', 'pyclipr')],
    cmdclass= {
        'build_ext': CMakeBuild,
        'develop': gitcmd_develop,
        'sdist': gitcmd_sdist,
    },
    packages = ['pyclipr'],
    package_dir={'': 'python'},
    keywords=['polygon clipping', 'polygon offsetting', 'libClipper', 'Clipper2', 'polygon boolean', 'polygon', 'line clipping', 'clipper'],
    # tell setuptools that all packages will be under the 'src' directory
    # and nowhere else
    zip_safe=False,
        classifiers=[
        'Programming Language :: Python',
        'Programming Language :: Python :: 3.7',
        'Programming Language :: Python :: 3.8',
        'Programming Language :: Python :: 3.9',
        'Programming Language :: Python :: 3.10',
        'Programming Language :: Python :: 3.11',
        'Programming Language :: Python :: 3.12',
        'Natural Language :: English',
        'Topic :: Scientific/Engineering'],
    license="",
    project_urls = {
        'Documentation': 'https://github.com/drylukeparry/pyclipr',
        'Source': 'https://github.com/drylukeparry/pyclipr',
        'Tracker': 'https://github.com/drlukeparry/pyclipr/issues'
    }
)

