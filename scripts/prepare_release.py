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
	response = response.split(' ')[-1]
	response = response.replace('.', '_')
	return response
	

def compile_latex(path: str, filename: str) -> str:
	response = subprocess.run(['pdflatex', filename + '.tex'], stdout=subprocess.PIPE, cwd=path)
	return path + filename + '.pdf'
	
	
def edit_config_file(path: str) -> None:
	f = open(path, 'r')
	cfg = json.loads(f.read())
	f.close()
	cfg['devices'] = cfg['devices'][0]
	del cfg['devices']['omp_threads']
	cfg['devices']['batch_size'] = 12
	cfg['search_threads'] = 1
	cfg['search_config']['max_batch_size'] = 12
	f = open(path, 'w')
	f.write(json.dumps(cfg, indent=2))
	f.close()
	


platform = get_platform()
conv_network_paths = {
	'freestyle_conv_8x128': '/home/maciek/alphagomoku/final_runs_2025/freestyle_20x20/network_swa.bin',
	'freestyle_conv_8x128_15x15': '/home/maciek/alphagomoku/final_runs_2025/freestyle_15x15/network_swa.bin',
	'standard_conv_8x128': '/home/maciek/alphagomoku/final_runs_2025/standard_15x15/network_swa.bin',
	'renju_conv_8x128': '/home/maciek/alphagomoku/final_runs_2025/renju_15x15/network_swa.bin',
	'caro5_conv_8x128': '/home/maciek/alphagomoku/final_runs_2025/caro5/network_swa.bin',
	'caro6_conv_8x128': '/home/maciek/alphagomoku/final_runs_2025/caro6/network_swa.bin'
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
	exe_extension = '.exe'
	

print('Step 2 - building docs')
if platform == 'linux':
	protocols_pdf = compile_latex('../doc/protocols/', 'protocols')
	user_manual_pdf = compile_latex('../doc/user_manual/', 'user_manual')


print('Step 3 - copying files')
dst_directory = '../release_' + get_version(build_directory + 'ag_player_cpu') + '/'
create_directory(dst_directory)
create_directory(dst_directory + 'logs')
create_directory(dst_directory + 'networks')

shutil.copy(build_directory + 'ag_player_cpu' + exe_extension, dst_directory)
shutil.copy(build_directory + 'ag_player_cuda' + exe_extension, dst_directory)
shutil.copy(build_directory + 'ag_player_opencl' + exe_extension, dst_directory)

os.rename(dst_directory + 'ag_player_cpu' + exe_extension, dst_directory + 'pbrain-AlphaGomoku' + exe_extension)
os.rename(dst_directory + 'ag_player_cuda' + exe_extension, dst_directory + 'pbrain-AlphaGomoku_cuda' + exe_extension)
os.rename(dst_directory + 'ag_player_opencl' + exe_extension, dst_directory + 'pbrain-AlphaGomoku_opencl' + exe_extension)

shutil.copy('../doc/protocols/protocols.pdf', dst_directory)
shutil.copy('../doc/user_manual/user_manual.pdf', dst_directory)


for key in conv_network_paths:
	tmp = dst_directory + 'networks/'
	shutil.copy(conv_network_paths[key], tmp)
	os.rename(tmp + 'network_swa.bin', tmp + key + '.bin')

if platform == 'windows':
	pass # TODO add copying libraries


print('Step 4 - configuring')
#os.system(dst_directory + 'pbrain-AlphaGomoku' + exe_extension + ' --benchmark')
os.system(dst_directory + 'pbrain-AlphaGomoku' + exe_extension + ' --configure')
# os.remove(dst_directory + 'benchmark.json')

edit_config_file(dst_directory + 'config.json')

f = open(dst_directory + 'swap2_openings.json', 'w')
f.write(json.dumps(swap2_openings, indent=2))
f.close()


if platform == 'windows':
	print('Step 5 - prepare Gomocup release')
	gomocup_directory = '../gomocup_' + get_version(build_directory + 'ag_player_cpu') + '/'
	shutil.copytree(dst_directory, gomocup_directory, dirs_exist_ok=True)
	
	os.remove(gomocup_directory + 'protocols.pdf')
	os.remove(gomocup_directory + 'user_manual.pdf')
	os.remove(gomocup_directory + 'pbrain-AlphaGomoku_cuda.exe')
	os.remove(gomocup_directory + 'pbrain-AlphaGomoku_opencl.exe')



