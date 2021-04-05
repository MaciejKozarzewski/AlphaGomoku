import os
import sys

from utils import get_platform, run_commands, find_sources, remove_ext, setup_msvc


class LibBuilder:
    def __init__(self, build_target: str, main_compiler: str, use_cuda: bool):
        assert (main_compiler == 'gcc' or main_compiler == 'msvc')
        self._build_target = build_target
        self._platform = get_platform()
        if self._platform == 'linux':
            assert (main_compiler == 'gcc')
        self._main_compiler = main_compiler
        self._sm_arch = sm_arch
        self._objects = []
        self._path_to_cuda = 'C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v9.0\\'
        self._path_to_zlib = 'C:\\Program Files (x86)\\GnuWin32\\'

    def _math_flags(self) -> str:
        if self._platform == 'windows':
            tmp = ' -DUSE_OPENBLAS'
        else:
            tmp = ' -DUSE_BLIS'
        if self._use_cuda:
            tmp += ' -DUSE_CUDA'
        return tmp

    def _lib_name(self) -> str:
        if not self._use_cuda:
            tmp = 'cpu_'
        else:
            tmp = ''
        if self._build_target == 'release':
            tmp += 'alfa'
        else:
            tmp += 'alfa_d'
        if self._main_compiler == 'gcc':
            return tmp + '.a'
        else:
            return tmp + '.lib'

    def _compiler(self) -> str:
        if self._main_compiler == 'gcc':
            return 'g++ -m64 -std=c++17 -fopenmp -c'
        else:
            return 'cl /std:c++17 /openmp /c /EHsc'

    def _optimizations(self) -> str:
        if self._main_compiler == 'gcc':
            if self._build_target == 'release':
                return ' -O3 -DNDEBUG'
            else:
                return ' -O0 -g3'
        else:
            if self._build_target == 'release':
                return ' /MD /O2 -DNDEBUG'
            else:
                return ' /MDd /Zi'

    def _includes(self) -> str:
        tmp = ''
        if self._main_compiler == 'gcc':
            tmp = ' -I\"../include/\" -I\"../contrib/\"'
        if self._main_compiler == 'msvc':
            tmp = ' /I\"../include/\" /I\"../contrib/\"'

        if self._platform == 'windows' and self._use_cuda:
            if self._main_compiler == 'gcc':
                tmp += ' -I\"' + self._path_to_cuda + 'include/\"'
            if self._main_compiler == 'msvc':
                tmp += ' /I\"' + self._path_to_cuda + 'include/\"'
        return tmp

    def _misc_options(self) -> str:
        tmp = ''
        if self._platform == 'windows' and self._main_compiler == 'gcc':
            tmp += ' -mxsave'
        return tmp

    def _compile_single_file(self, path: str, filename: str) -> list:
        command = self._compiler() + self._optimizations() + self._includes() + self._misc_options() + self._math_flags()
        result = None
        if self._main_compiler == 'gcc':
            result = [command + ' -o \"../bin/objects/' + remove_ext(filename) + '.o\" \"' + path + filename + '\"']
        if self._main_compiler == 'msvc':
            result = [command + ' /Fo\"../bin/objects/' + remove_ext(filename) + '.o\" \"' + path + filename + '\"']
        assert (result is not None)
        self._objects += ['../bin/objects/' + remove_ext(filename) + '.o']
        return result

    def _compile_lib(self) -> list:
        list_of_sources = find_sources('../src/', '.cpp', '.hpp', ['../src/math/cpu/', '../src/math/cuda/'])
        result = []
        for src in list_of_sources:
            result += self._compile_single_file(src[0], src[1])
        return result

    def _link(self) -> list:
        command = ''
        if self._main_compiler == 'gcc':
            command = 'ar rcs -o '
        if self._main_compiler == 'msvc':
            command = 'lib /OUT:'
        command += '\"../bin/' + self._lib_name() + '\"'

        for o in self._objects:
            command += ' \"' + o + '\"'
        return [command]

    def build(self):
        self._objects = []
        result = []
        result += self._compile_lib()
        result += self._link()
        return result

    def _libraries_path(self) -> str:
        result = ''
        if self._platform == 'windows' and self._use_cuda:
            if self._main_compiler == 'gcc':
                result += ' -L\"' + self._path_to_cuda + 'bin/\" -L\"../contrib/\"'
            if self._main_compiler == 'msvc':
                result += ' /link /LIBPATH:\"' + self._path_to_cuda + 'bin/\" /LIBPATH:\"../contrib/\" /LIBPATH:\"../bin\"'
        else:
            return result

    def _add_libraries(self) -> str:
        command = ' \"../bin/' + self._lib_name() + '\"'
        if self._use_cuda:
            command += ' \"../contrib/libml/bin/libml.a\"'
        else:
            command += ' \"../contrib/libml/bin/cpu_libml.a\"'
        if self._platform == 'windows':
            if self._main_compiler == 'gcc':
                command += ' \"../contrib/openblas/libopenblas.a\"'
                command += ' \"' + self._path_to_zlib + '/lib/libz.a\"'
            else:
                command += ' \"../contrib/openblas/libopenblas.dll.a\"'
        else:
            # command += ' -lopenblas'
            command += ' \"../contrib/blis/libblis.a\" -lz'
        return command

    def _add_cuda_libraries(self) -> str:
        command = ''
        if self._platform == 'linux':
            if self._build_target == 'release':
                command += ' \"../contrib/libml/bin/cuda_math.a\"'
            else:
                command += ' \"../contrib/libml/bin/cuda_math_d.a\"'
            command += ' -lcudart -lcublas'
        if self._platform == 'windows':
            if self._main_compiler == 'gcc':
                command += ' -L\"../bin\"'
                if self._build_target == 'release':
                    command += ' -lcuda_math'
                else:
                    command += ' -lcuda_math_d'
                command += ' -lcudart64_90 -lcublas64_90'
            else:
                command += 'cudart_static.lib cublas.lib'
                if self._build_target == 'release':
                    command += ' cuda_math.lib'
                else:
                    command += ' cuda_math_d.lib'
        return command

    def _executable_name(self) -> str:
        if not self._use_cuda:
            tmp = 'cpu_'
        else:
            tmp = ''
        tmp += self._build_target + '_launcher'
        if self._platform == 'windows':
            return tmp + '.exe'
        else:
            return tmp + '.out'

    def build_test(self) -> str:
        result = ''
        list_of_sources = find_sources('../test/', '.cpp', '.hpp', [])
        for src in list_of_sources:
            result += ' \"' + src[0] + src[1] + '\"'
        result += ' \"../contrib/gtest/gtest-all.cc\"'
        return result

    def create_exec(self) -> list:
        command = ''
        if self._main_compiler == 'gcc':
            command = 'g++ -std=c++17 -m64 -fopenmp'
            command += ' -o \"../bin/' + self._executable_name() + '\"'
        if self._main_compiler == 'msvc':
            command = 'cl /std:c++17 /openmp /EHsc'
            command += ' /Fe\"../bin/' + self._executable_name() + '\"'

        if self._build_target == 'test':
            command += self.build_test()
        else:
            command += ' \"../training_launcher/launcher.cpp\"'

        command += self._includes() + self._optimizations() + self._math_flags() + self._libraries_path() + self._add_libraries()
        if self._use_cuda:
            command += self._add_cuda_libraries()

        if self._main_compiler == 'msvc':
            return [setup_msvc(), command]
        else:
            return [command]

    def clear_objects(self) -> None:
        for o in self._objects:
            os.remove(o)


if __name__ == '__main__':
    if not os.path.exists('../bin'):
        os.mkdir('../bin')
    if not os.path.exists('../bin/objects'):
        os.mkdir('../bin/objects')

    compiler = sys.argv[1]

    use_cuda = True
    if len(sys.argv) == 3:
        use_cuda = not (sys.argv[2] == 'no_cuda')

    lb = LibBuilder('release', compiler, use_cuda)
    run_commands(lb.build())
    run_commands(lb.create_exec())

    lb = LibBuilder('debug', compiler, use_cuda)
    run_commands(lb.build())
    run_commands(lb.create_exec())

    lb = LibBuilder('test', compiler, use_cuda)
    run_commands(lb.create_exec())
