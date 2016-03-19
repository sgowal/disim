--[[
This LUA script does the exact same thing than the file CarControl.cpp does.
It shows how to initialize your controller, control the car depending on the
neighbors and destroy the controller when the car exits the highway.

LUA documentation is available here: http://www.lua.org/manual/5.1/manual.html

Quick note on how create LUA scripts for Disim
----------------------------------------------

The LUA script need to provide 3 functions, namely init(), think() and destroy().

init() takes 2 arguments (self) which is a pointer to the car that this script is controlling.
and (options) the option string passed through the lua-args commandline option.

think() takes 3 arguments (self, dt, neighbors). self is the pointer to the car object, dt
is the time step in seconds and neighbors contains the neighbors array giving access to
the data of neighboring vehicles. You can obtain the corresponding car object: neighbors[LEAD].car and
the distance to that neighbor: neighbors[LEAD].distance. think() should return 2 values:
the acceleration and the lane change commands.

destroy() takes 1 arguments (self). It is responsible to clean up when a car exists the highway.

The LUA API is documented at: http://en.wikibooks.org/wiki/Disim_Highway_Simulator/The_LUA_API

--]]

vars = {}

function printf(...)
  io.write(string.format(unpack(arg)))
end

function init(self, options)
  -- Store necessary variables specific to that car
  vars[self] = {max_speed = 120/3.6};
  if (self:getType() == TRUCK) then
    vars[self].max_speed = 80/3.6;
  end
end

function think(self, dt, neighbors)
  -- Current maximal speed allowed on this lane
  lane_speed = self:getLane():getSpeedLimit();

  -- Get lead car
  lead = neighbors[LEAD].car;

  -- If there is a leader, set the speed to be the one of the leader
  if (lead) then
    lane_speed = lead:getSpeed()
  end

  -- If this speed is higher than the maximal speed I get reach
  if (lane_speed > vars[self].max_speed) then
    lane_speed = vars[self].max_speed
  end

  -- Try to reach that speed
  acceleration = (lane_speed - self:getSpeed())*1.0

  -- Randomly change lanes
  lane_change = 0
  if (math.random() < 0.1*dt and self:isRightAllowed()) then
    lane_change = 1
  elseif (math.random() < 0.1*dt and self:isLeftAllowed()) then
    lane_change = -1
  end

  self:setAcceleration(acceleration)
  self:setLaneChange(lane_change)
end

function destroy(self)
  -- Remove the variables stored for that car
  vars[self] = nil
end
