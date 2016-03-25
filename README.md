# disim

This simple highway traffic simulator is available here: http://en.wikibooks.org/wiki/Disim_Highway_Simulator

Disim is a lightweight microscopic highway traffic simulator that enables the emulation of a complete traffic over several kilometers. It allows the definition of a specific road network under variable entry rates and road conditions. Disim also offers the possibility to add road sensors to measure the density, speed and traffic flow and log this data. Variable speed limits as well as ramp metering are supported natively.
This simulator display is based on OpenGL and FLTK which make it particularly fast and enjoyable to use. Another particularity is that the simulator can be run without a graphical interface to gather quantitative data more efficiently.
Disim offers a powerful scripting API enabling everyone to modify the individual car behavior and use the intelligent traffic management using the LUA programming language. As LUA is interpreted it requires no compilation and allows quick prototyping of behaviors.

## Install on MacOSX

```bash
# Install XCode tools.
xcode-select --install

# Install Homebrew.
ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"

# Install necessary libraries.
brew install fltk --devel
brew install gengetopt
brew install lua
```
