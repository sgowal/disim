--[[
This script works with: maps/I-210W.map

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

verbose = false
rampcontrol = 5
rampstocontrol = {'myrtle', 'huntington', 'santa_anita_1', 'santa_anita_2', 'baldwin_1', 'baldwin_2'}
nlanes = {4, 4, 4, 4, 4, 4}
criticaldensity     = {27.16, 27.16, 27.16, 27.16, 27.16, 27.16}
criticalqueuetime   = {120.0, 120.0, 120.0, 120.0, 120.0, 120.0} -- 2,3,5
criticalqueuelength = {10, 10, 10, 10, 10, 10}                   -- 4,6
K_alinea            = 0.1
K_queuetime         = 0.1
K_queuelength       = 0.5
K_coordinatetime    = 0.01
K_coordinatelength  = 0.05

lanes = {}
entryrates = {}
entryspeeds = {}
rampmeters = {}
densitysensors = {}
speedlimits = {}
previoustime = "xx:xx"
previousrampcontroltime = 1

function printf(...)
  io.stdout:write(string.format(unpack(arg)))
end

function init(self)
  printf("Initialize control for map: %s\n", self:getName())

  -- Get all rampmeters and density sensors
  if (rampcontrol ~= 0) then
    printf("Number of controlled on-ramps: %d.\n", #rampstocontrol)
    for i,ramptocontrol in ipairs(rampstocontrol) do
      r = self:getRoadActuator(string.format("rampmeter%d",i))
      q = self:getRoadSensor(string.format("rampmeter%d_queue", i))
      if (rampcontrol == 2 or rampcontrol == 3 or rampcontrol == 5) then
        rampmeters[r] = {green = 3, red = 5, lastchange = 0, density = criticaldensity[i], queuelimit = criticalqueuetime[i], queue = q}
      else
        rampmeters[r] = {green = 3, red = 5, lastchange = 0, density = criticaldensity[i], queuelimit = criticalqueuelength[i], queue = q}
      end
      densitysensors[r] = {}
      speedlimits[r] = {}
      printf(" %s found with sensor %s.\n", r:getName(), q:getName());
      for j = 1,nlanes[i] do
	d = self:getRoadSensor(string.format("%s_density_%d", ramptocontrol, j))
        densitysensors[r][j] = d
        printf("  %s found.\n", d:getName());
        s = self:getRoadActuator(string.format("%s_speedlimit_%d", ramptocontrol, j))
        speedlimits[r][j] = s
        if (s) then
          printf("  %s found.\n", s:getName());
          s:setSpeedLimit(70)
        end
      end
    end
  end

  -- Get all lanes
  lanes = self:getEntryLanes();
  printf("There are %d entry lanes on this map.\n", #lanes)
  for i,lane in ipairs(lanes) do
    printf(" %d) Lane: %-30s", i, lane:getName())
    if (entryrates[lane:getName()] == nil) then
      entryrates[lane:getName()] = {}
      entryspeeds[lane:getName()] = {}
      -- Read the PEMS data file located in data/
      --[[
            It is important to note that Disim sets the current
            working directory to be the location of this script.
            This, however, is only valid for the control script
            as it is fairly inefficient to change the working
            directory for individual car behaviors scripts.
      --]]
      for t=1,2 do
        local filename = "data/" .. lane:getName() .. "_flow.txt"
        if (t == 2) then
          filename = "data/" .. lane:getName() .. "_speed.txt"
        end
        local fp = io.open(filename, "r")
        if (fp) then
          local nl = 0
          for line in fp:lines() do
            if (line:find("%d+/%d+/%d+") ~= nil) then
              nl = nl + 1
              -- date time data1 data2 ... datan data nlanes observed
              nums = {}
              for n in line:gfind("%d+%.?%d*") do
                table.insert(nums, n)
              end
              local nlanes = nums[#nums-1]
              local time = string.format("%02d:%02d", nums[4], nums[5]) -- time of day
              if (t == 1) then
                local data = nums[#nums-2]*12 -- veh/h
                entryrates[lane:getName()][time] = data/nlanes;
              else
                local data = nums[#nums-2]*1.609344 -- km/h
                entryspeeds[lane:getName()][time] = data;
              end
            end
          end
          if (t == 2) then
            printf(string.rep(" ",40));
          end
          printf("[ OK ] - read %d data points\n", nl)
          fp:close()
        else
          if (t == 2) then
            printf(string.rep(" ",40));
          end
          printf("[FAIL]\n")
        end
      end
    else
      printf("[SKIP]\n");
    end
  end
end

function update(self, t, dt)

  --[[
       Here we control the rampmeters using
       1) ALINEA
       2) ALINEA + Queue Control via time of vehicles in the queue    (controlling the queue time)
       3) ALINEA + Queue Control via number of vehicles               (controlling the queue time)
       4) ALINEA + Queue Control via number of vehicles               (controlling the queue length)
       5) Coordinated ALINEA + Queue Control (queue time)
       5) Coordinated ALINEA + Queue Control (queue length)
  --]]

  if (rampcontrol > 0) then
    for rampmeter, dts in pairs(rampmeters) do
      -- Setting the ramp meter light
      dts.lastchange = dts.lastchange + dt
      if (rampmeter:getColor() == RED and dts.lastchange > dts.red) then
        dts.lastchange = 0
        rampmeter:green()
      elseif (rampmeter:getColor() == GREEN and dts.lastchange > dts.green) then
        dts.lastchange = 0
        rampmeter:red()
      end
    end

    -- Update with respect to the queue length
    if (rampcontrol > 1) then
      -- Gather queue lengths
      if (rampcontrol == 5 or rampcontrol == 6) then
        queuelength = {}
        for rampmeter, dts in pairs(rampmeters) do
          queuelength[rampmeter] = dts.queue:getVehicleCount()
          if (rampcontrol == 5) then
            queuelength[rampmeter] = queuelength[rampmeter]*(dts.red + dts.green)
          end
        end
      end

      -- Apply control
      for rampmeter, dts in pairs(rampmeters) do
        q,newvalue = rampmeter:getInstantQueueLength()
        if (rampcontrol == 3 or rampcontrol == 4 or rampcontrol == 5 or rampcontrol == 6) then
          q = dts.queue:getVehicleCount()
          if (rampcontrol == 3 or rampcontrol == 5) then
            q = q*(dts.red + dts.green)
          end
          -- keep same newvalue variable
        end
        if (newvalue and (rampcontrol == 5 or rampcontrol == 6)) then
          -- Consensus control of queue lengths
          for rm, ql in pairs(queuelength) do
            if (rampcontrol == 5) then
              dts.red = dts.red - (q - ql)*K_coordinatetime
            else
              dts.red = dts.red - (q - ql)*K_coordinatelength
            end
          end
        end
        if (newvalue and q > dts.queuelimit) then
          if (rampcontrol == 2 or rampcontrol == 3 or rampcontrol == 5) then
            dts.red = dts.red - (q - dts.queuelimit)*K_queuetime
          else
            dts.red = dts.red - (q - dts.queuelimit)*K_queuelength
          end
        end
        if (dts.red < 0) then
          dts.red = 0
        elseif (dts.red > 50) then
          dts.red = 50
        end
      end
    end

    -- Aggregation of data happens every minute
    if (t - previousrampcontroltime >= 60) then
      previousrampcontroltime = previousrampcontroltime + 60

      for rampmeter, ds in pairs(densitysensors) do
        -- Compute average density
        density = 0
        for i,d in ipairs(ds) do
          density = density + d:getValue();
        end
        density = density / #ds
        -- Change rate
        dts = rampmeters[rampmeter]
        dts.red = dts.red + (density - dts.density)*K_alinea
        if (dts.red < 0) then
          dts.red = 0
        elseif (dts.red > 50) then
          dts.red = 50
        end
      end
    end
  end

  --[[
        Here we set the entry rates and speeds from the
        saved data read in the init function. The lookup
        is fairly simple as we can get the time of the day
        from the utily function getTimeOfDay.
  --]]

  -- Get the previous 5 min boundary (as the data is stored)
  local timeofday = self:getTimeOfDay(t)
  h,m = timeofday:match("(%d+):(%d+)")
  h,m = tonumber(h), tonumber(m)
  m = math.floor(m/5)*5
  timeofday = string.format("%02d:%02d", h, m)
  if (previoustime == timeofday) then
    return
  end
  previoustime = timeofday

  -- Change entry rate only if previous boundary was different
  for i,lane in ipairs(lanes) do
    if (entryrates[lane:getName()][timeofday] ~= nil) then
      lane:setEntryRate(entryrates[lane:getName()][timeofday])
      if (verbose) then
        printf("Changed entry rate of %s to %.2f veh/h\n", lane:getName(), entryrates[lane:getName()][timeofday])
      end
    end
    if (entryspeeds[lane:getName()][timeofday] ~= nil) then
      lane:setEntrySpeed(entryspeeds[lane:getName()][timeofday])
      if (verbose) then
        printf("Changed entry speed of %s to %.2f km/h\n", lane:getName(), entryspeeds[lane:getName()][timeofday])
      end
    end
  end
end

function destroy(self)
  
end
