--[[
This LUA script implements the IDM from Martin Treiber:
http://www.vwi.tu-dresden.de/~treiber/
--]]

-- Variables specific to each car
vars = {}

function printf(...)
  io.stdout:write(string.format(...))
end

--[[
The init function: it initializes the variables
of the self car depending on the type of vehicle it is.
--]]
function init(self, options)
  -- If options are specified, read them
  local v0_arg, v0_truck_arg, a_arg, a_truck_arg, b_arg, gamma_arg, t_arg
  local t_truck_arg, s0_arg, s0_truck_arg, b_safe_arg, p_arg, a_thr_arg

  if (options ~= "") then
    v0_arg = tonumber(options:match("v0=(%d+%.?%d*)"))
    v0_truck_arg = tonumber(options:match("v0_truck=(%d+%.?%d*)"))
    a_arg = tonumber(options:match("a=(%d+%.?%d*)"))
    a_truck_arg = tonumber(options:match("a_truck=(%d+%.?%d*)"))
    b_arg = tonumber(options:match("b=(%d+%.?%d*)"))
    gamma_arg = tonumber(options:match("gamma=(%d+%.?%d*)"))
    t_arg = tonumber(options:match("t=(%d+%.?%d*)"))
    t_truck_arg = tonumber(options:match("t_truck=(%d+%.?%d*)"))
    s0_arg = tonumber(options:match("s0=(%d+%.?%d*)"))
    s0_truck_arg = tonumber(options:match("s0_truck=(%d+%.?%d*)"))
    b_safe_arg = tonumber(options:match("b_safe=(%d+%.?%d*)"))
    p_arg = tonumber(options:match("p=(%d+%.?%d*)"))
  else
    v0_arg = 105/3.6       -- 65 mph
    v0_truck_arg = 85/3.6  -- 55 mph
    a_arg = 1.4
    a_truck_arg = 0.7
    b_arg = 2.0
    gamma_arg = 4.0
    t_arg = 1.0
    t_truck_arg = 1.5
    s0_arg = 2.0
    s0_truck_arg = 4.0
    b_safe_arg = 4.0
    p_arg = 0.25
  end
  a_thr_arg = math.min(a_arg, a_truck_arg);

  -- Store necessary variables specific to that car
  vars[self] = { v0 = v0_arg, a = a_arg, b = b_arg, gamma = gamma_arg, t = t_arg, s0 = s0_arg, b_safe = b_safe_arg, p = p_arg, a_thr = a_thr_arg, llc = 0.0 }

  if (self:getType() == TRUCK) then
    vars[self].v0 = v0_truck_arg
    vars[self].a = a_truck_arg
    vars[self].b = b_arg
    vars[self].gamma = gamma_arg
    vars[self].t = t_truck_arg
    vars[self].s0 = s0_truck_arg
    vars[self].b_safe = b_safe_arg
    vars[self].p = p_arg
    vars[self].a_thr = a_thr_arg
    vars[self].llc = 0.0
  end
end

--[[
The think function: it performs the IDM and MOBIL algorithms.
--]]
function think(self, dt, neighbors)
  -- Declare variables
  local speed_pref, dist, acceleration, lane_change

  -- Get prefered speed
  speed_pref = self:getLane():getSpeedLimit()
  if (speed_pref > vars[self].v0) then
    speed_pref = vars[self].v0
  end

  -- Compute IDM acceleration
  acceleration = IDM(self, self:getLane(), self, neighbors[LEAD].car, speed_pref, neighbors[LEAD].distance)

  -- If I have to exit the highway...
  if (self:getDestination()) then
    vars[self].llc = vars[self].llc + 4*dt
  end

  lane_change = 0
  if (vars[self].llc > 5.0) then
    -- Perform MOBIL of the right lane
    if (self:isRightAllowed()) then
      lane_change = MOBIL(self, self:getLane():getRight(), self, speed_pref, acceleration,
                          neighbors[TRAIL].car, neighbors[TRAIL].distance,
                          neighbors[LEAD].car, neighbors[LEAD].distance,
                          neighbors[RIGHT_TRAIL].car, neighbors[RIGHT_TRAIL].distance,
                          neighbors[RIGHT_LEAD].car, neighbors[RIGHT_LEAD].distance)
    end
    -- Perform MOBIL of the left lane
    if (lane_change == 0 and self:isLeftAllowed()) then
      lane_change = -MOBIL(self, self:getLane():getLeft(), self, speed_pref, acceleration,
                           neighbors[TRAIL].car, neighbors[TRAIL].distance,
                           neighbors[LEAD].car, neighbors[LEAD].distance,
                           neighbors[LEFT_TRAIL].car, neighbors[LEFT_TRAIL].distance,
                           neighbors[LEFT_LEAD].car, neighbors[LEFT_LEAD].distance)
    end

    if (lane_change ~= 0) then
      vars[self].llc = 0.0
    end
  end

  -- Update variables
  vars[self].llc = vars[self].llc + dt;

  self:setAcceleration(acceleration)
  self:setLaneChange(lane_change)
end

--[[
The destroy function: it clears the variables of the
self vehicle.
--]]
function destroy(self)
  -- Remove the variables stored for that car
  vars[self] = nil
end

--[[
This function performs the IDM using self's variables for the
host vehicle behind the lead vehicle using a specific preferred
speed, a specific leader speed and distance to that leading vehicle.
--]]
function IDM(self, lane, host, lead, speed_pref, dist)
  -- Declare variables
  local speed, s_star, dv, f, r, ndist, acceleration, nlead, lead_speed

  -- Check host
  if (not host) then
    return 0
  end

  -- See if we need to merge or stop for a red light and set variables correctly
  ndist = lane:getLengthLeft(self)
  f = self:getGeometry()
  nlead = nil
  ndist = ndist - f
  if (ndist < 0) then
    ndist = 1000;
  end
  lead_speed = 0
  r = 0
  if (lead) then
    phi,r = lead:getGeometry()
    if (dist-f-r < ndist) then
      nlead = lead
      ndist = dist - f - r
      lead_speed = nlead:getSpeed()
    end
  end
  if (ndist < 0) then
    return -9.0
  end

  -- Current speed
  speed = host:getSpeed()

  -- Get wanted range s*
  dv = speed - lead_speed
  s_star = vars[self].s0 + speed*vars[self].t + speed*dv/(2*math.sqrt(vars[self].a*vars[self].b))
  if (s_star < 0) then
    s_star = 0
  end

  -- Compute IDM acceleration
  acceleration = vars[self].a*(1 - (speed/speed_pref)^vars[self].gamma - s_star^2/ndist^2)
  if (acceleration < -9.0) then
    acceleration = -9.0
  end

  -- Return
  return acceleration
end

--[[
This function performs the MOBIL on the host using self's variables.
It returns 1 if a lane change occurs.
--]]
function MOBIL(self, lane, host, speed_pref, host_oacc,
               trail, trail_dist, lead, lead_dist, ntrail, ntrail_dist, nlead, nlead_dist)
  -- Declare variables
  local ntrail_nacc, host_nacc, hlane, ntrail_oacc, otrail_oacc, otrail_nacc, dist_left

  -- Check host
  if ((not host) or (not lane)) then
    return 0
  end

  -- Check merge direction: do not change lane in the wrong direction
  hlane = host:getLane()
  dist_left = lane:getLengthLeft(self)
  if ((lane:getMergeDirection() == 1 and lane:getRight() == hlane and dist_left < 300) or
      (lane:getMergeDirection() == -1 and lane:getLeft() == hlane and dist_left < 300)) then
    return 0
  end

  -- Check the safety criterion for ntrail vehicle
  ntrail_nacc = IDM(self, lane, ntrail, self, speed_pref, ntrail_dist)
  if (ntrail_nacc < -vars[self].b_safe) then
    return 0
  end

  -- New rear vehicle acceleration
  host_nacc = IDM(self, lane, self, nlead, speed_pref, nlead_dist)

  -- I am trying to enter the highway (More aggresive)
  if ((hlane:getMergeDirection() == 1 and hlane:getRight() == lane and dist_left < 200) or
      (hlane:getMergeDirection() == -1 and hlane:getLeft() == lane and dist_left < 200)) then
    if (host_nacc < -2*vars[self].b_safe) then
      return 0
    else
      return 1
    end
  end

  -- Check safety for host vehicle
  if (host_nacc < -vars[self].b_safe) then
    return 0
  end

  -- I should exit the highway
  dlane = self:getDestination()
  if (dlane) then
    if (lane == hlane:getLeft()) then
      while (hlane:getLeft() ~= dlane) do
        if (hlane:getLeft() == nil) then
          return 0
        end
        hlane = hlane:getLeft()
      end
      return 1
    elseif (lane == hlane:getRight()) then
      while (hlane:getRight() ~= dlane) do
        if (hlane:getRight() == nil) then
          return 0
        end
        hlane = hlane:getRight()
      end
      return 1
    end
  else
    -- I want to stay on the highway
    if (hlane:getType() == EXIT) then
      if (lane == hlane:getLeft()) then
        while (hlane) do
          if (hlane:getLeft() and hlane:getLeft():getType() ~= EXIT) then
            return 1
          end
          hlane = hlane:getLeft()
        end
        return 0
      elseif (lane == hlane:getRight()) then
        while (hlane) do
          if (hlane:getRight() and hlane:getRight():getType() ~= EXIT) then
            return 1
          end
          hlane = hlane:getRight()
        end
        return 0
      end
    elseif (lane:getType() == EXIT) then
      return 0
    end
  end

  -- Check if leading left/right needs to merge
  if (lane == hlane:getRight() and hlane:getLeft() and
      hlane:getLeft():getMergeDirection() == 1 and hlane:getLeft():getVehicleCount() > 0) then
    return 1
  elseif (lane == hlane:getLeft() and hlane:getRight() and
          hlane:getRight():getMergeDirection() == -1 and hlane:getRight():getVehicleCount() > 0) then
    return 1
  end

  -- Check the benefits
  ntrail_oacc = IDM(self, lane, ntrail, nlead, speed_pref, ntrail_dist + nlead_dist)
  otrail_oacc = IDM(self, lane, trail, self, speed_pref, trail_dist)
  otrail_nacc = IDM(self, lane, trail, lead, speed_pref, trail_dist + lead_dist)
  if (host_nacc - host_oacc > vars[self].p*(otrail_oacc + ntrail_oacc - otrail_nacc - ntrail_nacc) + vars[self].a_thr) then
    return 1
  end

  return 0
end


--[[
This function is an extra function linked with the LuaLane
object. It returns the distance left on that lane in meters.
--]]
function LuaLane:getLengthLeft(car)
  ok = false
  p = car:getPosition()
  pf = 1000.0
  traffic_light = car:nextTrafficLight()

  -- Is the lane ending?
  if (self:getMergeDirection() ~= 0) then
    ok = true
    -- Final position of the lane
    if (self:getGeometry() == CIRCULAR) then
      pf = math.abs(self:getAngleSpan())
    else
      pf = self:getLength()
    end
  end

  -- Any traffic light?
  if (traffic_light and traffic_light:getColor() == RED) then
    ok = true
    pf = traffic_light:getPosition()
  end

  if (not ok) then
    return -1
  end

  -- If the lane is circular
  if (self:getGeometry() == CIRCULAR and self:getMergeDirection() ~= 0) then
    return (pf - p)*self:getRadius()
  end

  -- If it is straight
  return (pf - p)
end
