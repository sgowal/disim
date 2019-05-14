"""Example of script that run Disim and plots data from logs."""

import numpy as np
import os
import matplotlib.pylab as plt

from scipy import optimize


def run_disim(idm_args):
  lua_args = "--lua-args=\""
  for n, v in idm_args.items():
    lua_args += n+"={},".format(v)
  lua_args += "\""
  duration = 3600 * 6

  os.system('../../disim --start-time=06:00 --duration={} --lua="../car/IDM_MOBIL.lua" '
            '--luacontrol="../control/I-210W.lua" --map="../../maps/I-210W.map" --ncpu=8 '
            '--record --nogui --time-step=0.5 '.format(duration) + lua_args)


def plot():
  flow = [np.genfromtxt('logs/huntington_flow_{}.txt'.format(i), delimiter=' ') for i in range(1, 5)]
  density = [np.genfromtxt('logs/huntington_density_{}.txt'.format(i), delimiter=' ') for i in range(1, 5)]
  flow = np.mean(np.stack(f[:, 1] for f in flow), axis=0)
  density = np.mean(np.stack(d[:, 1] for d in density), axis=0)

  def piecewise_linear(x, x0, y0, k):
    return np.piecewise(x, [x < x0], [lambda x: y0 / x0 * x, lambda x: k * x + y0 - k * x0])
  p, e = optimize.curve_fit(piecewise_linear, density, flow, p0=[40., 2000., -1.])
  x = np.linspace(np.min(density), np.max(density), 100)

  plt.figure()
  plt.scatter(density, flow)
  plt.plot(x, piecewise_linear(x, *p), lw=2, color='red')
  plt.xlabel('Average Density [veh/km/lane]')
  plt.ylabel('Average Flow [veh/h/lane]')
  plt.show()


if __name__ == '__main__':
  idm_args = {
      'v0': 105/3.6,       # 65 mph
      'v0_truck': 85/3.6,  # 55 mph
      'a': 1.4,
      'a_truck': 0.7,
      'b': 2.0,
      'gamma': 4.0,
      't': 1.0,
      't_truck': 1.5,
      's0': 2.0,
      's0_truck': 4.0,
      'b_safe': 4.0,
      'p': 0.25,
  }
  # run_disim(idm_args)
  plot()
