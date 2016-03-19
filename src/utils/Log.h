/*!
 * \file Log.h
 * \brief Logging header
 *
 * \author Sven Gowal
 * \date 1 July 2007
 */

#ifndef _LOG_H_
#define _LOG_H_

#include <iostream>
#include <ostream>
#include <stdio.h>
#include <string>
#include <sstream>
#include <fstream>

#define INFO 1
#define ADDITIONAL_INFO 2

using namespace std;

class NullStream : public std::ostringstream {
 public:
  template<typename T> NullStream& operator<<(T t) {
    return *this;
  }
};

class Doublebuf: public std::streambuf {
 public:
  typedef std::char_traits<char> traits_type;
  typedef traits_type::int_type  int_type;

 Doublebuf(std::streambuf* sb1, std::streambuf* sb2):
  m_sb1(sb1),
    m_sb2(sb2)
    {}

  int_type overflow(int_type c) {
    if (m_sb1->sputc(c) == traits_type::eof() || m_sb2->sputc(c) == traits_type::eof())
      return traits_type::eof();
    return c;
  }

 private:
  std::streambuf* m_sb1;
  std::streambuf* m_sb2;
};

class DoubleStream: public ostream {
 public:
 DoubleStream(ostream &str1, ostream &str2) : ios(0), ostream(new Doublebuf(str1.rdbuf(), str2.rdbuf())) {}
  ~DoubleStream() { delete rdbuf(); };
};

class Log {
 public:
  static int getVerboseLevel();
  static void setVerboseLevel(int level);
  static ostream& getStream(int level);
  static void setGenericLogFile(const char *filename);
  static void setConsoleOutput(bool value);
  static void stop();

 private:
  static int verbose_level;
  static bool generic_logging;
  static bool console;
  static NullStream nStream;
  static ofstream genericFileStream;
  static DoubleStream dualStream;
};

#endif
