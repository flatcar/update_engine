// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "update_engine/simple_key_value_store.h"

#include <map>
#include <string>
#include <vector>

#include "strings/string_split.h"

using std::map;
using std::string;
using std::vector;

namespace chromeos_update_engine {
namespace simple_key_value_store {

// Parses a string. 
map<std::string, std::string> ParseString(const string& str) {
  // Split along '\n', then along '='
  std::map<std::string, std::string> ret;
  vector<string> lines = strings::SplitDontTrim(str, '\n');
  for (vector<string>::const_iterator it = lines.begin();
       it != lines.end(); ++it) {
    string::size_type pos = it->find('=');
    if (pos == string::npos)
      continue;
    string val = it->substr(pos + 1);
    if ((val.length() >= 2) &&
      ((val.front() == '\"' && val.back() == '\"') ||
      (val.front() == '\'' && val.back() == '\''))) {
      val = val.substr(1, val.length() - 2);
    }

    ret[it->substr(0, pos)] = val;
  }
  return ret;
}

string AssembleString(const std::map<string, string>& data) {
  string ret;
  for (std::map<string, string>::const_iterator it = data.begin();
       it != data.end(); ++it) {
    ret += it->first;
    ret += "=";
    ret += it->second;
    ret += "\n";
  }
  return ret;
}

}  // namespace simple_key_value_store
}  // namespace chromeos_update_engine
