#!/bin/sh

./calibrate_IDM_MOBIL_SMP.py --start-time=05:00 --end-time=08:30 --lua="../../scripts/car/IDM_MOBIL.lua" --luacontrol="../../scripts/control/I-210W.lua" --map="../../maps/I-210W.map" --ncpu=6 data/Huntington_speed.txt speedsensor_huntington_0
