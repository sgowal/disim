#!/bin/sh

./disim --start-time=07:00 --lua="./scripts/car/IDM_MOBIL.lua" --luacontrol="./scripts/control/I-210W.lua" --map="./maps/I-210W.map" --ncpu=6 --verbose=1 $1 $2 $3 $4 $5 $6 $7 $8 $9
