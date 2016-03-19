#!/bin/sh

./disim --start-time=06:00 --duration=3600 --lua="./scripts/car/IDM_MOBIL.lua" --luacontrol="./scripts/control/I-210W.lua" --map="./maps/I-210W.map" --ncpu=6 --record --nogui --time-step=0.5 --log
