import os
import sys


def setup_msvc() -> str:
    return 'call \"C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat\"'


def get_platform() -> str:
    platforms = {'linux1': 'linux',
                 'linux2': 'linux',
                 'darwin': 'os x',
                 'win32': 'windows'}
    if sys.platform not in platforms:
        return sys.platform
    else:
        return platforms[sys.platform]


def remove_ext(filename: str) -> str:
    return filename[:filename.find('.')]


def find_sources(path: str, source_extension: str, header_extension: str, exclude_folders: list = []) -> list:
    if path in exclude_folders:
        return []

    try:
        lista = os.listdir(path)
    except Exception as e:
        return []
    result = []
    for f_name in lista:
        if f_name.endswith(source_extension):
            result.append([path, f_name])
        else:
            if not f_name.endswith(header_extension):
                result += find_sources(path + f_name + '/', source_extension, header_extension, exclude_folders)
    return result


def run_commands(commands: list) -> None:
    if type(commands) != list:
        commands = [commands]
    if get_platform() == 'linux':
        for cmd in commands:
            print(cmd)
            os.system(cmd)
    else:
        f = open('tmp_script.bat', 'w')
        for cmd in commands:
            f.write(cmd + '\n')
        f.close()
        os.system('tmp_script.bat')
        # os.remove('tmp_script.bat')
