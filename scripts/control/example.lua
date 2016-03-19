--[[
This script works with: maps/default.map

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

ramp_meter = nil
ramp_entry = nil
speed_limit = nil

function printf(...)
  io.write(string.format(unpack(arg)))
end

function init(self)
  printf("Initialize control for map: %s\n", self:getName());
  if (self:getName() == "The default map") then
    ramp_meter = self:getRoadActuator("rampmeter")
    ramp_entry = self:getLane("rampmeter_entry")
    speed_limit = self:getRoadActuator("speedlimit")
  end
end

function update(self, t, dt)
  if (self:getName() == "The default map") then

    -- Setting the ramp meter light
    v = math.sin(2*math.pi*t/10) -- Rate of 1/5 Hz
    if (v < 0.0) then
      ramp_meter:red()
    else
      ramp_meter:green()
    end

    -- Setting the entry rate of the ramp
    ramp_entry:setEntryRate(math.sin(2*math.pi*t/100)*1000 + 1000)

    -- Setting the speed limit
    speed_limit:setSpeedLimit(math.sin(2*math.pi*t/100)*60 + 60);
  end
end

function destroy(self)
  printf("Destroy control\n");
end
