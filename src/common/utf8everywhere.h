#pragma once

#ifndef UTF8_EVERYWHERE_H_
#define UTF8_EVERYWHERE_H_

#include <string>
#include <codecvt>

// widen and narrow (see http://utf8everywhere.org/)
// adapted from http://stackoverflow.com/a/18374698
inline std::wstring widen(std::string const &str) {
  typedef std::codecvt_utf8<wchar_t> convert_typeX;
  std::wstring_convert<convert_typeX, wchar_t> converterX;
  return converterX.from_bytes(str);
}

inline std::string narrow(const std::wstring &wstr) {
  typedef std::codecvt_utf8<wchar_t> convert_typeX;
  std::wstring_convert<convert_typeX, wchar_t> converterX;
  return converterX.to_bytes(wstr);
}

#endif
