#!/usr/bin/python

"""
Example: ./calibrate_IDM_MOBIL.py --start-time=05:00 --end-time=08:30 --lua="../../scripts/car/IDM_MOBIL.lua" --luacontrol="../../scripts/control/I-210W.lua" --map="../../maps/I-210W.map" --ncpu=6 data/Huntington_speed.txt ./logs/speedsensor_huntington_0.txt
"""

import random, sys, math, getopt, re, os

"""
This file execute a PSO algorithm to match the speed data
from the PEMS database and the speed gathered in simulation.
"""

nparticles = 5
vmaxfactor = 1.0
K0 = 1.0
K1 = 0.5
K2 = 2.0
K3 = 2.0
niteration = 100

mode = 0
argsname   = [ 'v0', 'v0_truck', 'a', 'a_truck', 'b', 'gamma', 't', 't_truck', 's0', 's0_truck', 'b_safe', 'p' ]
default    = [ 120/3.6, 85/3.6, 1.0, 0.7, 1.0, 4.0, 1.5, 1.5, 2.5, 4.0, 8.0, 0.25]
lowerbound = [ 90/3.6, 70/3.6, 0.2, 0.2, 0.5, 1.0, 0.5, 0.5, 0.5, 0.5, 0.5, 0.0]
upperbound = [ 140/3.6, 120/3.6, 4.0, 4.0, 4.0, 8.0, 3.0, 3.0, 5.0, 10.0, 8.0, 1.0]

executable = "../../disim"
transform = "{0}*1.609344"
starttime = 0
disimargs = ""
DISIMfilename = ""

PEMSdata = {}
DISIMdata = {}

""" Read the PEMS data file """
def readPEMSfile(filename):
    global PEMSdata
    fp = open(filename, 'r')
    for line in fp.readlines():
        if (not re.match('^\d+/\d+/\d+', line)):
            continue
        n = re.split('[^0-9\./:]', line)
        time = n[1]
        nlanes = n[-3]
        data = n[-4]
        # Convert things here
        PEMSdata[time] = eval(transform.format(data,nlanes))
    fp.close()

""" Read the Disim data file """
def readDISIMfile():
    global DISIMdata, DISIMfilename
    DISIMdata.clear()
    predatax = []
    predatay = []
    fp = open(DISIMfilename, 'r')
    for line in fp.readlines():
        n = re.split('[^0-9\.]', line)
        if (len(n) >= 2 and n[1] != ''):
            predatax.append(float(n[0]))
            predatay.append(float(n[1]))
    fp.close()
    # Transform data into PEMS data
    for k in PEMSdata.keys():
        xi = re.split(':', k)
        xi = (float(xi[0])*60 + float(xi[1]))*60-starttime # seconds
        # interpolate
        for i in range(len(predatax)-1):
            if (xi >= predatax[i] and xi <= predatax[i+1]):
                x1, x2, y1, y2 = predatax[i], predatax[i+1], predatay[i], predatay[i+1]
                yi = (xi-x1)*(y2-y1)/(x2-x1)+y1
                DISIMdata[k] = yi
                break
    
""" Computing the fitness """
def computeFitness(x):
    if (mode == 1):
        return reduce(lambda x, y: x+y, [i**2 for i in x])
    
    # The real stuff here
    luaargs = "--lua-args=\""
    for n,v in zip(argsname, x):
        luaargs += n+"={0},".format(v)
    luaargs += "\""
    command=executable+" "+disimargs+" "+luaargs+" > /dev/null"
    # print command
    os.system(command)
    readDISIMfile()
    # Compute RMSE
    f = float('inf')
    if (len(DISIMdata) > 0):
        f = math.sqrt(reduce(lambda x,y: x+y, [(PEMSdata[k]-DISIMdata[k])**2 for k in DISIMdata.keys()]))
    return f

""" Particles of the PSO """
class Particle:
    def __init__(self):
        self.x = []
        self.v = []
        for i in range(len(default)):
            if (mode == 2):
                self.x.append(default[i])
            else:
                self.x.append(random.uniform(lowerbound[i], upperbound[i]))
            self.v.append(random.uniform(-vmax[i], vmax[i]))
        self.b = self.x[:]
        self.fb = float('inf')
        self.fx = float('inf')

    def __str__(self):
        return self.x.__str__()

    def fitness(self):
        self.fx = computeFitness(self.x)
        if (self.fx < self.fb):
            self.fb = self.fx
            self.b = self.x[:]
        return self.fx

    def update(self, best):
        rp = random.uniform(0,1)
        rg = random.uniform(0,1)
        for i in range(len(default)):
            self.v[i] = K0*(K1*self.v[i] + K2*rp*(self.b[i]-self.x[i]) + K3*rg*(best[i]-self.x[i]))
            self.v[i] = min(self.v[i], vmax[i])
            self.v[i] = max(self.v[i], -vmax[i])
            self.x[i] += self.v[i];
            while (self.x[i] > upperbound[i]):
                self.x[i] -= upperbound[i]-lowerbound[i]
            while (self.x[i] < lowerbound[i]):
                self.x[i] += upperbound[i]-lowerbound[i]

""" PSO Swarm """
class Swarm:
    def __init__(self):
        self.particles = []
        for i in range(nparticles):
            self.particles.append(Particle())
        self.b = default[:]
        self.fb = float('inf')

    def __str__(self):
        s = "[ "
        for i in self.b:
            s += '{0:.2f} '.format(i)
        s += "]"
        return s

    def run(self):
        for i in range(niteration):
            print "Iteration %s..." % i
            for p in self.particles:
                p.fitness()
                sys.stdout.write("[ ")
                for v in p.x:
                    sys.stdout.write('{0:.2f} '.format(v))
                sys.stdout.write("] = {0:.2f}\n".format(p.fx))
                if (p.fx < self.fb):
                    self.fb = p.fx
                    self.b = p.x[:]
            print ""

            for p in self.particles:
                p.update(self.b)

            print " -> Best solution:", self, "with", self.fb


def usage():
    print '''Usage: ./calibrate_IDM_MOBIL.py [OPTIONS] <PEMS data file> <DISIM data file>

  -h, --help                  Print help and exit
  -i, --niterations=INT       Executes PSO for a given number of iterations (default=100)
      --data-transform=STRING Operation to transform PEMS data into Disim data (default=`{0}*1.609344')
                              {0} is the main PEMS data and {1} is the number of lanes
      --executable=STRING     Path to the disim executable (default=`../../disim')
  -s, --start-time=HH:MM      The starting hour in hh:mm
  -e, --end-time=HH:MM        The final hour in hh:mm
  -m, --map=STRING            The map file  (default=`./maps/default.map')
      --lua=STRING            The LUA script to be executed as the car controller (default=`./scripts/car/default.lua')
      --luacontrol=STRING     The LUA script to be executed as the infrastructure controller
      --truck=DOUBLE          The proportion of trucks at all times (default=`0.1')
      --weather=STRING        The weather conditions. Either nice, rain, fog or rain+fog  (default=`nice')   
      --density=DOUBLE        Initial density of cars at startup in veh/km (default=`0')
      --ncpu=INT              The number of cores on your computer  (default=`0')
      --debug                 Start a dummy PSO optimization on a 4D sphere function
'''

"""----------- Main ------------"""
def main():
    global disimargs, starttime, executable, mode, vmax, upperbound, lowerbound, default, niteration, transform, DISIMfilename, nparticles
    st = "00:00"
    et = "00:00"

    # parse commandline
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hi:s:e:m:", ["help","niterations=","executable=","start-time=","end-time=","lua=","luacontrol=", "truck=","weather=","density=","ncpu=","map=","data-transform=","debug="])
    except getopt.error, msg:
        print "Wrong arguments"
        usage()
        sys.exit(2)
    if (len(args) < 2):
        print "Wrong arguments"
        usage()
        sys.exit(2)

    for o,a in opts:
        if o in ("-h", "--help"):
            usage()
            sys.exit()
        elif o == "--executable":
            executable = a
        elif o in ("-i", "--niterations"):
            niteration = int(a)
        elif o in ("-s", "--start-time"):
            st = a
        elif o in ("-e", "--end-time"):
            et = a
        elif o == "--debug":
            mode = int(a)
        elif o == "--data-transform":
            transform = a
        else:
            disimargs += o+"="+a+" "

    # Do something about the arguments
    if (mode == 1):
        default    = [ 0, 0, 0, 0 ]
        lowerbound = [ -1, -1, -1, -1 ]
        upperbound = [ 1, 1, 1, 1 ]
    elif (mode == 2):
        nparticles = 1
        niteration = 1

    t = re.split(':', st)
    starttime = (float(t[0])*60 + float(t[1]))*60
    t = re.split(':', et)
    endtime = (float(t[0])*60 + float(t[1]))*60

    disimargs += "--time-step=0.5 --start-time={0} --duration={1} --record --nogui".format(st, int(endtime-starttime))
    print executable, disimargs

    vmax = upperbound[:]
    for i in range(len(vmax)):
        vmax[i] -= lowerbound[i]
        vmax[i] *= vmaxfactor

    readPEMSfile(args[0])
    DISIMfilename = args[1]
    s = Swarm()
    s.run()

if __name__ == "__main__":
    main()
