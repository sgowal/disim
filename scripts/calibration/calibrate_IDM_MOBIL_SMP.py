#!/usr/bin/python

"""
./calibrate_IDM_MOBIL_SMP.py --start-time=05:00 --end-time=08:30 --lua="../../scripts/car/IDM_MOBIL.lua" --luacontrol="../../scripts/control/I-210W.lua" --map="../../maps/I-210W.map" --ncpu=6 data/Huntington_speed.txt speedsensor_huntington_0
"""

import random, sys, math, getopt, re, os, subprocess as sub, time

"""
This file execute a PSO algorithm to match the speed data
from the PEMS database and the speed gathered in simulation.
"""

nparticles = 50
vmaxfactor = 1.0
K0 = 1.0
K1 = 0.5
K2 = 2.0
K3 = 2.0
niteration = 200

argsname   = [ 'v0', 'v0_truck', 'a', 'a_truck', 'b', 'gamma', 't', 't_truck', 's0', 's0_truck', 'b_safe', 'p' ]
default    = [ 120/3.6, 85/3.6, 1.4, 0.7, 2.0, 4.0, 1.0, 1.5, 2.0, 4.0, 4.0, 0.25]
lowerbound = [ 90/3.6, 70/3.6, 0.2, 0.2, 0.5, 1.0, 0.5, 0.5, 0.5, 0.5, 0.5, 0.0]
upperbound = [ 140/3.6, 120/3.6, 4.0, 4.0, 4.0, 8.0, 3.0, 3.0, 5.0, 10.0, 8.0, 1.0]

pythonargs = ['qsub', 'calibrate_IDM_MOBIL_fitness_SMP.sh']

""" Particles of the PSO """
class Particle:
    def __init__(self):
        self.x = []
        self.v = []
        for i in range(len(default)):
            self.x.append(random.uniform(lowerbound[i], upperbound[i]))
            self.v.append(random.uniform(-vmax[i], vmax[i]))
        self.b = self.x[:]
        self.fb = float('inf')
        self.fx = float('inf')

    def __str__(self):
        return self.x.__str__()

    def setFitness(self, f):
        self.fx = f
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

            # Start disim with qsub
            jobids = {}
            for p in self.particles:
                # The real stuff here
                luaargs = "--lua-args=\""
                for n,v in zip(argsname, p.x):
                    luaargs += n+"={0},".format(v)
                luaargs += "\""
                command = []
                command.extend(pythonargs)
                command.append(luaargs)
                #print " ".join(command)
                pr = sub.Popen(command,stdout=sub.PIPE,stderr=sub.PIPE)
                output, errors = pr.communicate()
                jobid = re.search('\d+', output)
                jobid = int(jobid.group(0))
                sys.stdout.write("Job ID: {0} ".format(jobid))
                sys.stdout.write("[ ")
                for v in p.x:
                    sys.stdout.write('{0:.2f} '.format(v))
                sys.stdout.write("]\n")
                jobids[jobid] = p

            # Wait for results
            while (True):
                for i in jobids.keys():
                    if (not os.path.exists('logs/job{0}.out'.format(i))):
                        break
                else:
                    # All file are here
                    break
                time.sleep(1)
            print "Got results."
            time.sleep(1)

            # Read results
            for i, p in jobids.iteritems():
                f = open('logs/job{0}.out'.format(i),'r')
                fn = float(f.read())
                f.close()
                p.setFitness(fn)
                if (p.fx < self.fb):
                    self.fb = p.fx
                    self.b = p.x[:]
            
            # Erase useless files
            time.sleep(1)
            os.system('rm *.o* logs/job*.out');

            # Update particles position
            for p in self.particles:
                p.update(self.b)

            print " -> Best solution:", self, "with", self.fb

def usage():
    print '''Usage: ./calibrate_IDM_MOBIL.py [OPTIONS] <PEMS data file> <DISIM sensor name>

  -h, --help                  Print help and exit
  -i, --niterations=INT       Executes PSO for a given number of iterations (default=100)
      --data-transform=STRING Operation to transform PEMS data into Disim data (default=`{0}*1.609344')
                              {0} is the main PEMS data and {1} is the number of lanes
      --executable=STRING     Path to the disim executable (default=`../../disim')
      --start-time=HH:MM      The starting hour in hh:mm
      --end-time=HH:MM        The final hour in hh:mm
      --map=STRING            The map file  (default=`./maps/default.map')
      --lua=STRING            The LUA script to be executed as the car controller (default=`./scripts/car/default.lua')
      --luacontrol=STRING     The LUA script to be executed as the infrastructure controller
      --truck=DOUBLE          The proportion of trucks at all times (default=`0.1')
      --weather=STRING        The weather conditions. Either nice, rain, fog or rain+fog  (default=`nice')   
      --density=DOUBLE        Initial density of cars at startup in veh/km (default=`0')
      --ncpu=INT              The number of cores on your computer  (default=`0')
'''

"""----------- Main ------------"""
def main():
    global pythonargs, vmax, upperbound, lowerbound, default, niteration 

    # parse commandline
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hi:", ["help","niterations=","executable=","start-time=","end-time=","lua=","luacontrol=", "truck=","weather=","density=","ncpu=","map=","data-transform="])
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
        elif o in ("-i", "--niterations"):
            niteration = int(a)
        else:
            pythonargs.append(o+"="+a)

    for a in args:
        pythonargs.append(a)

    print pythonargs

    vmax = upperbound[:]
    for i in range(len(vmax)):
        vmax[i] -= lowerbound[i]
        vmax[i] *= vmaxfactor

    s = Swarm()
    s.run()

if __name__ == "__main__":
    main()
