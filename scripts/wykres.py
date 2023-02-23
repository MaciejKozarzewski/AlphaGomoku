import numpy as np
import subprocess
import matplotlib.pyplot as plt
from matplotlib import ticker
from matplotlib.backends.backend_pdf import PdfPages
from matplotlib import rc, rcParams
rcParams['legend.numpoints'] = 1
rcParams['legend.scatterpoints'] = 1
rcParams['legend.handlelength'] = 1
rcParams['legend.handletextpad'] = 0.1

rc('font', **{'family':'serif', 'serif':['Times New Roman']})
rc('text', usetex=True)

legend_font_size = 16
label_font_size = 16
tick_font_size = 16
note_font_size = 16
line_width = 1.0

fig = plt.figure()
fig.set_figheight(8)
fig.set_figwidth(10)
rect = fig.patch
rect.set_facecolor('#FFFFFF')
fig.subplots_adjust(left=0.1, bottom=0.1, right=0.90, top=0.97, wspace=0.08, hspace=0.17)


plt.clf()
partC = fig.add_subplot(1, 1, 1)
partC.tick_params(labelsize=tick_font_size)

def generate_rating():
	p = subprocess.Popen(['./bayeselo'], bufsize=1048576, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
	pin, pout = p.stdin, p.stdout
	pin.write('readpgn rating.pgn\n'.encode())
	pin.flush()
	pin.write('elo\n'.encode())
	pin.flush()
	pin.write('advantage 0\n'.encode())
	pin.flush()
	pin.write('drawelo 0\n'.encode())
	pin.flush()
	pin.write('mm\n'.encode())
	pin.flush()
	pin.write('ratings>rat.txt\n'.encode())
	pin.flush()
	pin.write('x\n'.encode())
	pin.flush()
	pin.write('x\n'.encode())
	pin.flush()
	p.wait()

generate_rating()

data=[]
for line in open('rat.txt', 'r'):
	values = [s for s in line.split()]
	data.append(values)

sorted_data=[]
elos = np.zeros((len(data)-1))
err = np.zeros((2,len(data)-1))

for i in range(len(data)-1):
	name='AG_'
	if i<10:
		name+='0'
	if i<100:
		name+='0'
#	if i<1000:
#		name+='0'
	name+=str(i)
	for j in range(1, len(data), 1):
		if data[j][1]==name:
			sorted_data.append(data[j])
			break
	elos[i]=int(sorted_data[i][2])
	err[0][i]=int(sorted_data[i][3])
	err[1][i]=int(sorted_data[i][4])

elos = elos-elos[0]
err = np.abs(err)

partC.axis([0, elos.shape[0], 0, np.max(elos+err[0])])

partC.plot(range(elos.shape[0]), elos, '-o', color='blue')
#partC.errorbar(range(elos.shape[0]), elos, yerr=err, xerr=None , color='blue')
#partC.legend(loc='lower left', ncol=2, frameon=False, prop={'size':legend_font_size})
plt.tight_layout()
pp = PdfPages('elo_graph.pdf')
pp.savefig(fig)
pp.close()
plt.clf()


data = np.genfromtxt('training_history.txt', skip_header=1)

partP = fig.add_subplot(2, 1, 1)
partP.set_ylabel('loss', size=label_font_size, labelpad=10)
partP.plot(data[:,0], data[:,1], '-', color='blue', label='policy train')
partP.plot(data[:,0], data[:,4], '--', color='blue', label='policy valid')
partP.legend(loc='upper right', frameon=False, prop={'size':legend_font_size})

partV = fig.add_subplot(2, 1, 2)
partV.set_ylabel('loss', size=label_font_size, labelpad=10)
partV.plot(data[:,0], data[:,2], '-', color='red', label='value train')
partV.plot(data[:,0], data[:,5], '--', color='red', label='value valid')
partV.legend(loc='upper right', frameon=False, prop={'size':legend_font_size})

plt.tight_layout()
pp = PdfPages('training.pdf')
pp.savefig(fig)
pp.close()
plt.clf()
    


