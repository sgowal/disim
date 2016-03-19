#!/usr/bin/python

"""
Example: ./calibrate_IDM_MOBIL.py --start-time=05:00 --end-time=08:30 --lua="../../scripts/car/IDM_MOBIL.lua" --luacontrol="../../scripts/control/I-210W.lua" --luaargs="v0=1.2" --map="../../maps/I-210W.map" --record-path="/scratch/user/job12/" --ncpu=6 data/Huntington_speed.txt speedsensor_huntington_0
"""

import random, sys, math, getopt, re, os

"""
This file executes disim and compute the fitness
"""

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
def computeFitness():
    command=executable+" "+disimargs+" > /dev/null"
    # print command
    os.system(command)
    readDISIMfile()
    # Compute RMSE
    f = float('inf')
    if (len(DISIMdata) > 0):
        f = math.sqrt(reduce(lambda x,y: x+y, [(PEMSdata[k]-DISIMdata[k])**2 for k in DISIMdata.keys()]))
    return f

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
      --luaargs=STRING        The arguments to the LUA script
      --luacontrol=STRING     The LUA script to be executed as the infrastructure controller
      --truck=DOUBLE          The proportion of trucks at all times (default=`0.1')
      --weather=STRING        The weather conditions. Either nice, rain, fog or rain+fog  (default=`nice')   
      --density=DOUBLE        Initial density of cars at startup in veh/km (default=`0')
      --ncpu=INT              The number of cores on your computer  (default=`0')
'''

"""----------- Main ------------"""
def main():
    global disimargs, starttime, executable, transform, DISIMfilename
    st = "00:00"
    et = "00:00"

    # parse commandline
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hi:s:e:m:", ["help","niterations=","executable=","start-time=","end-time=","lua=","luacontrol=", "truck=","weather=","density=","ncpu=","map=","data-transform=","luaargs=","record-path=","debug"])
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
        elif o == "--data-transform":
            transform = a
        elif o == "--record-path":
            DISIMfilename = a+"/"+args[1]+".txt"
            disimargs += o+"="+a+" "
        else:
            disimargs += o+"="+a+" "

    t = re.split(':', st)
    starttime = (float(t[0])*60 + float(t[1]))*60
    t = re.split(':', et)
    endtime = (float(t[0])*60 + float(t[1]))*60

    disimargs += "--time-step=0.5 --start-time={0} --duration={1} --record --nogui".format(st, int(endtime-starttime))
    readPEMSfile(args[0])
    print computeFitness()

if __name__ == "__main__":
    main()
