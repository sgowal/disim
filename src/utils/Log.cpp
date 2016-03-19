/*!
 * \file Log.cpp
 * \brief Source code for logging
 *
 * \author Sven Gowal
 * \date 1 July 2007
 */

#include "Log.h"
#include <ctime>

/** Initialize verbose level to maximum */
int Log::verbose_level = 9; 
NullStream Log::nStream;
bool Log::generic_logging = false;
bool Log::console = true;
ofstream Log::genericFileStream;
DoubleStream Log::dualStream(genericFileStream, cout);

int Log::getVerboseLevel()
{
  return verbose_level;
}

void Log::setVerboseLevel(int level)
{
  verbose_level = level;
}

void Log::setGenericLogFile(const char *filename)
{
  genericFileStream.open(filename);
  if (genericFileStream.is_open()) {
    generic_logging = true;
    /* Generate date */
    struct tm *current;
    time_t now;
    time(&now);
    current = localtime(&now);

    genericFileStream << "Car Simulator Log: " << asctime(current) << endl;
  }
}

void Log::setConsoleOutput(bool value)
{
  console = value;
}

void Log::stop()
{
  genericFileStream.close();
  generic_logging = false;
}

ostream& Log::getStream(int level)
{
  if (verbose_level >= level) {
    if (generic_logging) {
      if (console)
        return dualStream;
      else {
        return genericFileStream;
      }
    } else {
      if (console)
        return cout;
      else 
        return nStream;
    }
  } else {
    return nStream;
  }
}
