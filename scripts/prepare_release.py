import os
import sys
import shutil
import subprocess
import json


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
    

def create_directory(path: str) -> None:
	if not os.path.exists(path):
		os.mkdir(path)

	
def get_version(exe_path: str) -> str:
	response = subprocess.run([exe_path, '--version'], stdout=subprocess.PIPE).stdout
	response = response.decode('utf-8')
	response = response.split('\n')[0]
	response = response.replace('\r', '')
	response = response.split(' ')[-1]
	response = response.replace('.', '_')
	return response


def run_command(cmd: str, args: str) -> None:
	response = subprocess.run([cmd, args], stdout=subprocess.PIPE).stdout
	

def compile_latex(path: str, filename: str) -> str:
	response = subprocess.run(['pdflatex', filename + '.tex'], stdout=subprocess.PIPE, cwd=path)
	return path + filename + '.pdf'
	
	
def edit_config_file(path: str) -> None:
	f = open(path, 'r')
	cfg = json.loads(f.read())
	f.close()
	cfg['devices'] = [cfg['devices'][0]]
	cfg['devices'][0]['batch_size'] = 12
	cfg['search_threads'] = 1
	cfg['search_config']['max_batch_size'] = 12
	f = open(path, 'w')
	f.write(json.dumps(cfg, indent=2))
	f.close()
	
	
if __name__ == '__main__':
	dst_path = sys.argv[1]
	create_directory(dst_path)

	platform = get_platform()

	if platform == 'linux':
		conv_network_paths = {
			'freestyle_conv_8x128': '/home/maciek/alphagomoku/final_runs_2025/freestyle_20x20/network_swa.bin',
			'freestyle_conv_8x128_15x15': '/home/maciek/alphagomoku/final_runs_2025/freestyle_15x15/network_swa.bin',
			'standard_conv_8x128': '/home/maciek/alphagomoku/final_runs_2025/standard_15x15/network_swa.bin',
			'renju_conv_8x128': '/home/maciek/alphagomoku/final_runs_2025/renju_15x15/network_swa.bin',
			'caro5_conv_8x128': '/home/maciek/alphagomoku/final_runs_2025/caro5/network_swa.bin',
			'caro6_conv_8x128': '/home/maciek/alphagomoku/final_runs_2025/caro6/network_swa.bin'
		}
	if platform == 'windows':
		conv_network_paths = {
			'freestyle_conv_8x128': 'C:\\Users\\mk\\Desktop\\AlphaGomoku592\\networks\\freestyle_conv_8x128.bin',
			'freestyle_conv_8x128_15x15': 'C:\\Users\\mk\\Desktop\\AlphaGomoku592\\networks\\freestyle_conv_8x128_15x15.bin',
			'standard_conv_8x128': 'C:\\Users\\mk\\Desktop\\AlphaGomoku592\\networks\\standard_conv_8x128.bin',
			'renju_conv_8x128': 'C:\\Users\\mk\\Desktop\\AlphaGomoku592\\networks\\renju_conv_8x128.bin',
			'caro5_conv_8x128': 'C:\\Users\\mk\\Desktop\\AlphaGomoku592\\networks\\freestyle_conv_8x128.bin',
			'caro6_conv_8x128': 'C:\\Users\\mk\\Desktop\\AlphaGomoku592\\networks\\freestyle_conv_8x128.bin',
		}

	swap2_openings = [
		[{"row": 5, "col": 11, "sign": "CROSS"}, {"row": 6, "col": 11, "sign": "CIRCLE"}, {"row": 6, "col": 13, "sign": "CROSS"}],
		[{"row": 12, "col": 11, "sign": "CROSS"}, {"row": 13, "col": 13, "sign": "CIRCLE"}, {"row": 14, "col": 12, "sign": "CROSS"}],
		[{"row": 13, "col": 11, "sign": "CROSS"}, {"row": 12, "col": 10, "sign": "CIRCLE"}, {"row": 10, "col": 11, "sign": "CROSS"}],
		[{"row": 11, "col": 8, "sign": "CROSS"}, {"row": 10, "col": 9, "sign": "CIRCLE"}, {"row": 11, "col": 12, "sign": "CROSS"}],
		[{"row": 10, "col": 9, "sign": "CROSS"}, {"row": 12, "col": 8, "sign": "CIRCLE"}, {"row": 13, "col": 4, "sign": "CROSS"}]
	]


	print('Step 1 - building executables')
	build_directory = '../build/Release/bin/'
	exe_extension = None
	if platform == 'linux':
		os.system('sh build_all.sh')
		exe_extension = ''
	if platform == 'windows':
		os.system('windows_build_all.bat')
		exe_extension = '.exe'


	print('Step 2 - building docs')
	if platform == 'linux':
		protocols_pdf = compile_latex('../doc/protocols/', 'protocols')
		user_manual_pdf = compile_latex('../doc/user_manual/', 'user_manual')


	print('Step 3 - copying files')
	dst_directory = dst_path + '/AlphaGomoku_' + platform + '/'
	create_directory(dst_directory)
	create_directory(dst_directory + 'logs')
	create_directory(dst_directory + 'networks')

	shutil.copy(build_directory + 'ag_player_cpu' + exe_extension, dst_directory + 'pbrain-AlphaGomoku' + exe_extension)
	shutil.copy(build_directory + 'ag_player_cuda' + exe_extension, dst_directory + 'pbrain-AlphaGomoku_cuda' + exe_extension)
	shutil.copy(build_directory + 'ag_player_opencl' + exe_extension, dst_directory + 'pbrain-AlphaGomoku_opencl' + exe_extension)

	shutil.copy('../doc/protocols/protocols.pdf', dst_directory)
	shutil.copy('../doc/user_manual/user_manual.pdf', dst_directory)


	for key in conv_network_paths:
		tmp = dst_directory + 'networks/'
		shutil.copy(conv_network_paths[key], tmp + key + '.bin')
		

	if platform == 'windows':
		mingw_path = 'C:\\mingw64\\bin\\'
		libraries = ['libdl.dll', 'libgcc_s_seh-1.dll', 'libgomp-1.dll', 'libstdc++-6.dll', 'libwinpthread-1.dll', 'libOpenCL.dll', 'libclblast.dll']
		for lib in libraries:
			shutil.copy(mingw_path + lib, dst_directory)
		
		shutil.copy('../../minml/build/cudaml.dll', dst_directory)


	print('Step 4 - configuring')
	run_command(dst_directory + 'pbrain-AlphaGomoku' + exe_extension, '--benchmark')
	run_command(dst_directory + 'pbrain-AlphaGomoku' + exe_extension, '--configure')
	os.remove(dst_directory + 'benchmark.json')

	edit_config_file(dst_directory + 'config.json')

	f = open(dst_directory + 'swap2_openings.json', 'w')
	f.write(json.dumps(swap2_openings, indent=2))
	f.close()


	if platform == 'windows' and 'gomocup' in sys.argv[1:]:
		print('Step 5 - prepare Gomocup release')
		gomocup_directory = dst_path + '/gomocup/'
		shutil.copytree(dst_directory, gomocup_directory, dirs_exist_ok=True)
		
		os.remove(gomocup_directory + 'protocols.pdf')
		os.remove(gomocup_directory + 'user_manual.pdf')
		os.remove(gomocup_directory + 'pbrain-AlphaGomoku_cuda.exe')
		os.remove(gomocup_directory + 'pbrain-AlphaGomoku_opencl.exe')
		os.remove(gomocup_directory + 'cudaml.dll')
		os.remove(gomocup_directory + 'libOpenCL.dll')
		os.remove(gomocup_directory + 'libclblast.dll')





