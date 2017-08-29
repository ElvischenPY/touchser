#!/usr/bin/python
########################2015-03-13
import os
import string
import sys
import math
import pylab as pl
import numpy as np
import datetime
from matplotlib.ticker import MultipleLocator, FuncFormatter
from matplotlib import pyplot as plt  
from matplotlib import animation 


#you can change this arguments as follows
#********************************************#
#every frame point delay time(ms) 
speed=100 

ts_max_x=1080
ts_max_y=1920

pad_max_x=1080
pad_max_y=100

#save csv file name
save_point_flie = "touch_point.csv"
#********************************************#


def get_slot(sum):
	return ((sum>>40) & 0xf)
def get_time(sum):
	return ((sum>>24) & 0xffff)
def get_id(sum):
	return ((sum>>23) & 0x1)
def get_x(sum):
	return ((sum>>12) & 0x7ff)
def get_type(sum):
	return ((sum>>11) & 0x1)
def get_y(sum):
	return (sum & 0x7ff)

def get_year(sum):
	return ((sum>>20) & 0x7ff)
def get_month(sum):
	return ((sum>>16) & 0xf)
def get_day(sum):
	return ((sum>>11) & 0x1f)
def get_hour(sum):
	return ((sum>>6) & 0x1f)
def get_min(sum):
	return (sum & 0x3f)
'''
point_info[]
0 time --str
1 delta time  -- ms
2 device-id  0-ts 1-pad
3 slot-id 
4 type 0-up 1-down 
5 x
6 y
'''
class ReadLog:
	def __init__(self,filename):
		self.logname = filename
	def parse(self):
		fb = file(self.logname,'r')
		lines = fb.readlines()
		if lines != []:
			print 'read file ok'
		else:
			print'read file fail'
		result = []
		sfb=open(save_point_flie, 'w')
		sfb.write('Rawdata, Time, Device, Slot-id, type, x, y\n')
		time_bb = 0
		delta_min = 0
		last_time_ms = -1
		for line in lines:
			point_info = [0 for col in range(7) ]
			line = line.rstrip('\n')
			line = line.rstrip('\r')
			if(len(line) == 11):
				pointer = int(line, 16)#string.atoi(line)
				time_str = time_min+"%02d.%03d" % (get_time(pointer)/1000, get_time(pointer)%1000)
				sfb.write('%011x, %s, %d, %d, %s, %d, %d\n' % (pointer, time_str, get_id(pointer), get_slot(pointer), ("down" if (get_type(pointer)) else "up"), get_x(pointer), get_y(pointer)))
#				print time_str,'slot=',get_slot(pointer), 'time=',get_time(pointer), 'id=',get_id(pointer), 'x=',get_x(pointer), 'type=',get_type(pointer), 'y=',get_y(pointer)
				time_ms = get_time(pointer)
				if (last_time_ms == -1):
					delta_time = 0
				else:
					delta_time = delta_min*60*1000 + time_ms - last_time_ms
#					print 'delta_min=', delta_min, 'time_ms=', time_ms, 'last_time_ms=', last_time_ms
				point_info[0] = time_str
				point_info[1] = delta_time
				point_info[2] = get_id(pointer)
				point_info[3] = get_slot(pointer)
				point_info[4] =	get_type(pointer) 
				point_info[5] = get_x(pointer) 
				point_info[6] = get_y(pointer)
				last_time_ms = time_ms
				delta_min = 0
				result.append(point_info)
			elif (len(line) == 8):
				#time = int(line.rstrip('\n'), 16)
				time = int(line, 16)
				time_min = "%04d-%02d-%02d-%02d:%02d:" % (get_year(time) ,get_month(time), get_day(time), get_hour(time), get_min(time))
				time_aa = datetime.datetime(get_year(time), get_month(time), get_day(time), get_hour(time), get_min(time))
				if (time_bb != 0):
					delta_min = (time_aa - time_bb).seconds/60
				time_bb = time_aa
				min_sum = get_month(time)
#				print time_min
#				print get_month(time),'-', get_day(time),' ', get_hour(time),':',get_min(time)
			else:
				print 'error str:',line,len(line)
		sfb.close()
		return result

###start
print '\n\nAuthor  :  Elvis.Chen'
print 'Time    :  2017-06-01'
print 'Version :  V1.0\n\n'

inputarg = sys.argv
if len(inputarg) < 2:
    print 'Please input a rawdata log'
    exit()

filename = inputarg[1]
readlog = ReadLog(filename)
inputlist = readlog.parse()
#######################step 1 
#print inputlist 

aa = [100*i for i in range(ts_max_x/100)]
bb = [100*i for i in range((ts_max_y+pad_max_y)/100)]

xx = [[] for i in range(10)]
yy = [[] for i in range(10)]
xx2 = [[] for i in range(10)]
yy2 = [[] for i in range(10)]

fig = plt.figure(figsize=(8,12))
ax = fig.add_subplot(111, aspect='equal', axisbg='tan', autoscale_on=False)
ax.set_xlim(left=0, right=ts_max_x)
ax.set_ylim(bottom=ts_max_y+pad_max_y, top=0)
ax.xaxis.tick_top()
ax.set_xticks([100*i for i in range(ts_max_x/100+1)])
ax.set_yticks([100*i for i in range((ts_max_y+pad_max_y)/100+1)])
ax.grid()

line0, = ax.plot([], [], lw=1)
line1, = ax.plot([], [], lw=1)
line2, = ax.plot([], [], lw=1)
line3, = ax.plot([], [], lw=1)
line4, = ax.plot([], [], lw=1)
line5, = ax.plot([], [], lw=1)
line6, = ax.plot([], [], lw=1)
line7, = ax.plot([], [], lw=1)
line8, = ax.plot([], [], lw=1)
line9, = ax.plot([], [], lw=1)
linecut, = ax.plot([], [], lw=1,color="k",linewidth=2)
line20, = ax.plot([], [], lw=1)
line21, = ax.plot([], [], lw=1)
line22, = ax.plot([], [], lw=1)
line23, = ax.plot([], [], lw=1)
line24, = ax.plot([], [], lw=1)
line25, = ax.plot([], [], lw=1)
line26, = ax.plot([], [], lw=1)
line27, = ax.plot([], [], lw=1)
line28, = ax.plot([], [], lw=1)
line29, = ax.plot([], [], lw=1)

#point0 = ax.scatter(0, 0, s=40, c='red', alpha=0.8, marker='o')
#point2 = ax.scatter(0, 0, s=40, c='red', alpha=0.8, marker='o')
time_text = ax.text(0.02, 0.98, '', transform=ax.transAxes)
point_text = ax.text(0.6,0.98, '', transform=ax.transAxes)
point_text.set_verticalalignment('top')
point_text2 = ax.text(0.02,0.1, '', transform=ax.transAxes)
point_text2.set_verticalalignment('top')

def init():
	linecut.set_data([i for i in range(ts_max_x)],[ts_max_y for i in range(ts_max_x)])
	line0.set_data([], [])
	line1.set_data([], [])
	line2.set_data([], [])
	line3.set_data([], [])
	line4.set_data([], [])
	line5.set_data([], [])
	line6.set_data([], [])
	line7.set_data([], [])
	line8.set_data([], [])
	line9.set_data([], [])
	time_text.set_text('')
	point_text.set_text('')
	point0 = ax.scatter(0, 0, s=40, c='red', alpha=0.8, marker='o')
	point2 = ax.scatter(0, 0, s=40, c='red', alpha=0.8, marker='o')
	return (point0,line0,line1,line2,line3,line4,line5,line6,line7,line8,line9,time_text,point_text,linecut,
		point2,line20,line21,line22,line23,line24,line25,line26,line27,line28,line29,point_text2)
def animate(i):
	time = inputlist[i][0]
	device_id = inputlist[i][2]
	slot = inputlist[i][3]
	type1 = inputlist[i][4]
	x = inputlist[i][5]
	y = inputlist[i][6]

	if (device_id == 1): #pad
		if (type1 ==1): #down
			xx2[slot].append(x)
			yy2[slot].append(y + ts_max_y)
		else: #up
			xx2[slot]=[]
			yy2[slot]=[]
	else: #TP
		if (type1 ==1): #down
			xx[slot].append(x)
			yy[slot].append(y)
		else: #up
			xx[slot]=[]
			yy[slot]=[]

	line0.set_data(xx[0], yy[0])
	line1.set_data(xx[1], yy[1])
	line2.set_data(xx[2], yy[2])
	line3.set_data(xx[3], yy[3])
	line4.set_data(xx[4], yy[4])
	line5.set_data(xx[5], yy[5])
	line6.set_data(xx[6], yy[6])
	line7.set_data(xx[7], yy[7])
	line8.set_data(xx[8], yy[8])
	line9.set_data(xx[9], yy[9])


	linecut.set_data([i for i in range(ts_max_x)],[ts_max_y for i in range(ts_max_x)])
	line20.set_data(xx2[0], yy2[0])
	line21.set_data(xx2[1], yy2[1])
	line22.set_data(xx2[2], yy2[2])
	line23.set_data(xx2[3], yy2[3])
	line24.set_data(xx2[4], yy2[4])
	line25.set_data(xx2[5], yy2[5])
	line26.set_data(xx2[6], yy2[6])
	line27.set_data(xx2[7], yy2[7])
	line28.set_data(xx2[8], yy2[8])
	line29.set_data(xx2[9], yy2[9])
	time = "Time: " + time
	time_text.set_text(time)

	pointx = []
	pointy = []
	point_str=''
	pointx2 = []
	pointy2 = []
	point_str2 =''
	for n in range(10):
		if(xx[n] != []):
			pointx.append(xx[n][-1])
			pointy.append(yy[n][-1])
			point_str = point_str + "Slot%d: x=%d y=%d\n" % (n,xx[n][-1],yy[n][-1])
		if(xx2[n] != []):
			pointx2.append(xx2[n][-1])
			pointy2.append(yy2[n][-1])
			point_str2 = point_str2 + "Slot%d: x=%d y=%d\n" % (n,xx2[n][-1],(yy2[n][-1] -ts_max_y))
	point0 = ax.scatter(pointx, pointy, s=40, c='red', alpha=0.8, marker='o')
	point_text.set_text(point_str)
	point2 = ax.scatter(pointx2, pointy2, s=40, c='red', alpha=0.8, marker='o')
	point_text2.set_text(point_str2)

	return (point0,line0,line1,line2,line3,line4,line5,line6,line7,line8,line9,time_text,point_text,linecut,
		point2,line20,line21,line22,line23,line24,line25,line26,line27,line28,line29,point_text2)


#frames_time=(inputlist[i+1][1] for i in range(len(inputlist)-1))

anim = animation.FuncAnimation(fig, animate, init_func=init,frames=len(inputlist), interval=speed, blit=True, repeat=False)
#animation.FuncAnimation(fig, animate, frames=frames_time,  interval=20, blit=True)

plt.show()
