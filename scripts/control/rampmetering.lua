--[[
This script works with the map: maps/example.map

This LUA script updates the infrastructure depending on the current
time (useful to simulate peak hours) or sensor values placed on the road network.
It can modify the entry rates as well as the different actuators present
on the infrastructure.

LUA documentation is available here: http://www.lua.org/manual/5.1/manual.html

Quick note on how create LUA control scripts for Disim
------------------------------------------------------

The LUA script need to provide 1 function, namely update().

update() takes 2 arguments:
 - self: which is a pointer to the infrastructure that this script is controlling.
 - t: which is the current simulation time in seconds.
 - dt: which is the current simulation time step.
It should not return any value.

The LUA API is documented at: http://en.wikibooks.org/wiki/Disim_Highway_Simulator/The_LUA_API
--]]

-- Actuator
ramp_meter = nil
-- Sensors
density_sensor = {}
-- Variables
K = 0.1
optimal_density = 60
tprev = 1
last_change = 0
dt_red = 5
dt_green = 3

function init(self)
  ramp_meter = self:getRoadActuator("rampmeter")
  density_sensor[0] = self:getRoadSensor("densitysensor1")
  density_sensor[1] = self:getRoadSensor("densitysensor2")
end

function update(self, t, dt)
  -- Setting the ramp meter light
  last_change = last_change + dt
  if (ramp_meter:getColor() == RED and last_change > dt_red) then
    last_change = 0
    ramp_meter:green()
  elseif (ramp_meter:getColor() == GREEN and last_change > dt_green) then
    last_change = 0
    ramp_meter:red()
  end

  -- Aggregation of data happens every minute
  if (t - tprev < 60) then
    return
  end
  tprev = t

  density = density_sensor[0]:getValue() + density_sensor[1]:getValue()
  dt_red = dt_red + (density - optimal_density)*K
  if (dt_red < 0) then dt_red = 0; end
end

function destroy(self)
  -- Nothing to do here
end
