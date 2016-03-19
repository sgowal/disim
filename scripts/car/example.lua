--[[
This LUA script does the exact same thing than the file CarControl.cpp does.
It shows how to initialize your controller, control the car depending on the
neighbors and destroy the controller when the car exits the highway.

LUA documentation is available here: http://www.lua.org/manual/5.1/manual.html

Quick note on how create LUA scripts for Disim
----------------------------------------------

The LUA script need to provide 3 functions, namely init(), think() and destroy().

init() takes 2 arguments (self) which is a pointer to the car that this script is controlling
and (options) the option string passed through the lua-args commandline option.

think() takes 3 arguments (self, dt, neighbors). self is the pointer to the car object, dt
is the time step in seconds and neighbors contains the neighbors array giving access to
the data of neighboring vehicles. You can obtain the corresponding car object: neighbors[LEAD].car and
the distance to that neighbor: neighbors[LEAD].distance. think() should return 2 values:
the acceleration and the lane change commands.

destroy() takes 1 arguments (self). It is responsible to clean up when a car exists the highway.

The LUA API is documented at: http://en.wikibooks.org/wiki/Disim_Highway_Simulator/The_LUA_API

--]]

K = 1.0
safety_distance = 5

function init(self, options)
  -- Nothing to do here
end

function think(self, dt, neighbors)
  -- Current maximal speed allowed on this lane
  lane_speed = self:getLane():getSpeedLimit()
  lane_acceleration = (lane_speed - self:getSpeed())*K

  -- Get distance to leading car
  lead = neighbors[LEAD].car
  if (lead) then
    f = self:getGeometry()
    phi,r = lead:getGeometry()
    lead_distance = neighbors[LEAD].distance - f - r
    lead_acceleration = (lead:getSpeed()^2 - self:getSpeed()^2)/(lead_distance-safety_distance)
  else
    lead_acceleration = 1000
  end

  self:setAcceleration(math.min(lane_acceleration, lead_acceleration))
  self:setLaneChange(0)
end

function destroy(self)
  -- Nothing to do here too
end
