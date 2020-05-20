#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Thu Mar  5 11:29:41 2020

@author: glesur
"""

import idefixTools as idfx
import sod
import numpy as np
import matplotlib.pyplot as plt
from scipy.interpolate import interp1d

V=idfx.readVTKCart('../data.0002.vtk')
gamma = 5.0/3.0
npts = 500

# left_state and right_state set p, rho and u
# geometry sets left boundary on 0., right boundary on 1 and initial
# position of the shock xi on 0.5
# t is the time evolution for which positions and states in tube should be calculated
# gamma denotes specific heat
# note that gamma and npts are default parameters (1.4 and 500) in solve function
positions, regions, values = sod.solve(left_state=(1, 1, 0), right_state=(0.1, 0.125, 0.),
                                       geometry=(0., 1., 0.5), t=0.2, gamma=gamma, npts=npts)


# Finally, let's plot solutions
p = values['p']
rho = values['rho']
u = values['u']
x= values['x']


solinterp=interp1d(x,p)

error=np.sqrt(np.mean((V.data['PRS'][:,0,0]-solinterp(V.x))**2/solinterp(V.x)**2))

print("Error=%e"%error)
if error<1e-1:
    print("SUCCESS!")
else:
    print("FAILURE!")



plt.figure(1)
plt.plot(x,rho)
plt.plot(V.x,V.data['RHO'][:,0,0],'+',markersize=2)
plt.title('Density')

plt.figure(2)
plt.plot(x,u)
plt.plot(V.x,V.data['VX1'][:,0,0],'+',markersize=2)
plt.title('Velocity')

plt.figure(3)
plt.plot(x,p)
plt.plot(V.x,V.data['PRS'][:,0,0],'+',markersize=2)
plt.title('Pressure')

plt.ioff()
plt.show()