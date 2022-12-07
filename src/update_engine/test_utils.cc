// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "update_engine/test_utils.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <set>
#include <string>
#include <vector>

#include <glog/logging.h>

#include "strings/string_printf.h"
#include "update_engine/file_writer.h"
#include "update_engine/filesystem_iterator.h"
#include "update_engine/utils.h"

using std::set;
using std::string;
using std::vector;
using strings::StringPrintf;

namespace chromeos_update_engine {

bool WriteFileVector(const std::string& path, const std::vector<char>& data) {
  return utils::WriteFile(path.c_str(), &data[0], data.size());
}

bool WriteFileString(const std::string& path, const std::string& data) {
  return utils::WriteFile(path.c_str(), data.data(), data.size());
}

std::string Readlink(const std::string& path) {
  vector<char> buf(PATH_MAX + 1);
  ssize_t r = readlink(path.c_str(), &buf[0], buf.size());
  if (r < 0)
    return "";
  CHECK_LT(r, static_cast<ssize_t>(buf.size()));
  buf.resize(r);
  string ret;
  ret.insert(ret.begin(), buf.begin(), buf.end());
  return ret;
}

std::vector<char> GzipCompressData(const std::vector<char>& data) {
  const char fname[] = "/tmp/GzipCompressDataTemp";
  if (!WriteFileVector(fname, data)) {
    EXPECT_EQ(0, system((string("rm ") + fname).c_str()));
    return vector<char>();
  }
  EXPECT_EQ(0, system((string("cat ") + fname + "|gzip>" +
                       fname + ".gz").c_str()));
  EXPECT_EQ(0, system((string("rm ") + fname).c_str()));
  vector<char> ret;
  EXPECT_TRUE(utils::ReadFile(string(fname) + ".gz", &ret));
  EXPECT_EQ(0, system((string("rm ") + fname + ".gz").c_str()));
  return ret;
}

vector<char> GenerateSampleMbr() {
  // This is the actual MBR from my dev machine. Partition 1 (the first)
  // is currently marked bootable
  unsigned char mbr[512] = {
    0xeb, 0x48, 0x90, 0x10, 0x8e, 0xd0, 0xbc, 0x00,
    0xb0, 0xb8, 0x00, 0x00, 0x8e, 0xd8, 0x8e, 0xc0,
    0xfb, 0xbe, 0x00, 0x7c, 0xbf, 0x00, 0x06, 0xb9,
    0x00, 0x02, 0xf3, 0xa4, 0xea, 0x21, 0x06, 0x00,
    0x00, 0xbe, 0xbe, 0x07, 0x38, 0x04, 0x75, 0x0b,
    0x83, 0xc6, 0x10, 0x81, 0xfe, 0xfe, 0x07, 0x75,
    0xf3, 0xeb, 0x16, 0xb4, 0x02, 0xb0, 0x01, 0xbb,
    0x00, 0x7c, 0xb2, 0x80, 0x8a, 0x74, 0x03, 0x02,
    0xff, 0x00, 0x00, 0x20, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x02, 0xfa, 0x90, 0x90, 0xf6, 0xc2, 0x80,
    0x75, 0x02, 0xb2, 0x80, 0xea, 0x59, 0x7c, 0x00,
    0x00, 0x31, 0xc0, 0x8e, 0xd8, 0x8e, 0xd0, 0xbc,
    0x00, 0x20, 0xfb, 0xa0, 0x40, 0x7c, 0x3c, 0xff,
    0x74, 0x02, 0x88, 0xc2, 0x52, 0xbe, 0x7f, 0x7d,
    0xe8, 0x34, 0x01, 0xf6, 0xc2, 0x80, 0x74, 0x54,
    0xb4, 0x41, 0xbb, 0xaa, 0x55, 0xcd, 0x13, 0x5a,
    0x52, 0x72, 0x49, 0x81, 0xfb, 0x55, 0xaa, 0x75,
    0x43, 0xa0, 0x41, 0x7c, 0x84, 0xc0, 0x75, 0x05,
    0x83, 0xe1, 0x01, 0x74, 0x37, 0x66, 0x8b, 0x4c,
    0x10, 0xbe, 0x05, 0x7c, 0xc6, 0x44, 0xff, 0x01,
    0x66, 0x8b, 0x1e, 0x44, 0x7c, 0xc7, 0x04, 0x10,
    0x00, 0xc7, 0x44, 0x02, 0x01, 0x00, 0x66, 0x89,
    0x5c, 0x08, 0xc7, 0x44, 0x06, 0x00, 0x70, 0x66,
    0x31, 0xc0, 0x89, 0x44, 0x04, 0x66, 0x89, 0x44,
    0x0c, 0xb4, 0x42, 0xcd, 0x13, 0x72, 0x05, 0xbb,
    0x00, 0x70, 0xeb, 0x7d, 0xb4, 0x08, 0xcd, 0x13,
    0x73, 0x0a, 0xf6, 0xc2, 0x80, 0x0f, 0x84, 0xea,
    0x00, 0xe9, 0x8d, 0x00, 0xbe, 0x05, 0x7c, 0xc6,
    0x44, 0xff, 0x00, 0x66, 0x31, 0xc0, 0x88, 0xf0,
    0x40, 0x66, 0x89, 0x44, 0x04, 0x31, 0xd2, 0x88,
    0xca, 0xc1, 0xe2, 0x02, 0x88, 0xe8, 0x88, 0xf4,
    0x40, 0x89, 0x44, 0x08, 0x31, 0xc0, 0x88, 0xd0,
    0xc0, 0xe8, 0x02, 0x66, 0x89, 0x04, 0x66, 0xa1,
    0x44, 0x7c, 0x66, 0x31, 0xd2, 0x66, 0xf7, 0x34,
    0x88, 0x54, 0x0a, 0x66, 0x31, 0xd2, 0x66, 0xf7,
    0x74, 0x04, 0x88, 0x54, 0x0b, 0x89, 0x44, 0x0c,
    0x3b, 0x44, 0x08, 0x7d, 0x3c, 0x8a, 0x54, 0x0d,
    0xc0, 0xe2, 0x06, 0x8a, 0x4c, 0x0a, 0xfe, 0xc1,
    0x08, 0xd1, 0x8a, 0x6c, 0x0c, 0x5a, 0x8a, 0x74,
    0x0b, 0xbb, 0x00, 0x70, 0x8e, 0xc3, 0x31, 0xdb,
    0xb8, 0x01, 0x02, 0xcd, 0x13, 0x72, 0x2a, 0x8c,
    0xc3, 0x8e, 0x06, 0x48, 0x7c, 0x60, 0x1e, 0xb9,
    0x00, 0x01, 0x8e, 0xdb, 0x31, 0xf6, 0x31, 0xff,
    0xfc, 0xf3, 0xa5, 0x1f, 0x61, 0xff, 0x26, 0x42,
    0x7c, 0xbe, 0x85, 0x7d, 0xe8, 0x40, 0x00, 0xeb,
    0x0e, 0xbe, 0x8a, 0x7d, 0xe8, 0x38, 0x00, 0xeb,
    0x06, 0xbe, 0x94, 0x7d, 0xe8, 0x30, 0x00, 0xbe,
    0x99, 0x7d, 0xe8, 0x2a, 0x00, 0xeb, 0xfe, 0x47,
    0x52, 0x55, 0x42, 0x20, 0x00, 0x47, 0x65, 0x6f,
    0x6d, 0x00, 0x48, 0x61, 0x72, 0x64, 0x20, 0x44,
    0x69, 0x73, 0x6b, 0x00, 0x52, 0x65, 0x61, 0x64,
    0x00, 0x20, 0x45, 0x72, 0x72, 0x6f, 0x72, 0x00,
    0xbb, 0x01, 0x00, 0xb4, 0x0e, 0xcd, 0x10, 0xac,
    0x3c, 0x00, 0x75, 0xf4, 0xc3, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x50, 0xc1, 0x04, 0x00, 0x00, 0x00, 0x80, 0x01,
    0x01, 0x00, 0x83, 0xfe, 0xff, 0xff, 0x3f, 0x00,
    0x00, 0x00, 0x09, 0x7f, 0x32, 0x06, 0x00, 0xfe,
    0xff, 0xff, 0x05, 0xfe, 0xff, 0xff, 0x48, 0x7f,
    0x32, 0x06, 0x79, 0x59, 0x2d, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55, 0xaa
  };
  vector<char> ret;
  ret.insert(ret.begin(), reinterpret_cast<char *>(mbr),
             reinterpret_cast<char *>(mbr + sizeof(mbr)));
  return ret;
}

// Binds provided |filename| to an unused loopback device, whose name is written
// to the string pointed to by |lo_dev_name_p|. Returns true on success, false
// otherwise (along with corresponding test failures), in which case the content
// of |lo_dev_name_p| is unknown.
bool BindToUnusedLoopDevice(const string& filename, string* lo_dev_name_p) {
  CHECK(lo_dev_name_p);

  // Bind to an unused loopback device, sanity check the device name.
  lo_dev_name_p->clear();
  if (!(utils::ReadPipe("losetup --show -f " + filename, lo_dev_name_p) &&
        (*lo_dev_name_p).compare(0, sizeof("/dev/loop")-1, "/dev/loop") == 0)) {
    ADD_FAILURE();
    return false;
  }

  // Strip anything from the first newline char.
  size_t newline_pos = lo_dev_name_p->find('\n');
  if (newline_pos != string::npos)
    lo_dev_name_p->erase(newline_pos);

  return true;
}

bool ExpectVectorsEq(const vector<char>& expected, const vector<char>& actual) {
  EXPECT_EQ(expected.size(), actual.size());
  if (expected.size() != actual.size())
    return false;
  bool is_all_eq = true;
  for (unsigned int i = 0; i < expected.size(); i++) {
    EXPECT_EQ(expected[i], actual[i]) << "offset: " << i;
    is_all_eq = is_all_eq && (expected[i] == actual[i]);
  }
  return is_all_eq;
}

void FillWithData(vector<char>* buffer) {
  size_t input_counter = 0;
  for (vector<char>::iterator it = buffer->begin(); it != buffer->end(); ++it) {
    *it = kRandomString[input_counter];
    input_counter++;
    input_counter %= sizeof(kRandomString);
  }
}

void CreateEmptyExtImageAtPath(const string& path,
                               size_t size,
                               int block_size) {
  EXPECT_EQ(0, System(StringPrintf("dd if=/dev/zero of=%s"
                                   " seek=%zu bs=1 count=1",
                                   path.c_str(), size)));
  EXPECT_EQ(0, System(StringPrintf("mkfs.ext2 -b %d -F %s -I 128",
                                   block_size, path.c_str())));
}

void CreateExtImageAtPath(const string& path, vector<string>* out_paths) {
  // create 10MiB sparse file
  EXPECT_EQ(0, System(StringPrintf("dd if=/dev/zero of=%s"
                                   " seek=10485759 bs=1 count=1",
                                   path.c_str())));
  EXPECT_EQ(0, System(StringPrintf("mkfs.ext2 -b 4096 -F %s", path.c_str())));
  EXPECT_EQ(0, System(StringPrintf("mkdir -p %s", kMountPath)));
  EXPECT_EQ(0, System(StringPrintf("mount -o loop %s %s", path.c_str(),
                                   kMountPath)));
  EXPECT_EQ(0, System(StringPrintf("echo hi > %s/hi", kMountPath)));
  EXPECT_EQ(0, System(StringPrintf("echo hello > %s/hello", kMountPath)));
  EXPECT_EQ(0, System(StringPrintf("mkdir %s/some_dir", kMountPath)));
  EXPECT_EQ(0, System(StringPrintf("mkdir %s/some_dir/empty_dir", kMountPath)));
  EXPECT_EQ(0, System(StringPrintf("mkdir %s/some_dir/mnt", kMountPath)));
  EXPECT_EQ(0, System(StringPrintf("echo T > %s/some_dir/test", kMountPath)));
  EXPECT_EQ(0, System(StringPrintf("mkfifo %s/some_dir/fifo", kMountPath)));
  EXPECT_EQ(0, System(StringPrintf("mknod %s/cdev c 2 3", kMountPath)));
  EXPECT_EQ(0, System(StringPrintf("ln -s /some/target %s/sym", kMountPath)));
  EXPECT_EQ(0, System(StringPrintf("ln %s/some_dir/test %s/testlink",
                                   kMountPath, kMountPath)));
  EXPECT_EQ(0, System(StringPrintf("echo T > %s/srchardlink0", kMountPath)));
  EXPECT_EQ(0, System(StringPrintf("ln %s/srchardlink0 %s/srchardlink1",
                                   kMountPath, kMountPath)));
  EXPECT_EQ(0, System(StringPrintf("ln -s bogus %s/boguslink",
                                   kMountPath)));
  EXPECT_TRUE(utils::UnmountFilesystem(kMountPath));

  if (out_paths) {
    out_paths->clear();
    out_paths->push_back("");
    out_paths->push_back("/hi");
    out_paths->push_back("/boguslink");
    out_paths->push_back("/hello");
    out_paths->push_back("/some_dir");
    out_paths->push_back("/some_dir/empty_dir");
    out_paths->push_back("/some_dir/mnt");
    out_paths->push_back("/some_dir/test");
    out_paths->push_back("/some_dir/fifo");
    out_paths->push_back("/cdev");
    out_paths->push_back("/testlink");
    out_paths->push_back("/sym");
    out_paths->push_back("/srchardlink0");
    out_paths->push_back("/srchardlink1");
    out_paths->push_back("/lost+found");
  }
}

void VerifyAllPaths(const string& parent, set<string> expected_paths) {
  FilesystemIterator iter(parent, set<string>());
  ino_t test_ino = 0;
  ino_t testlink_ino = 0;
  while (!iter.IsEnd()) {
    string path = iter.GetFullPath();
    EXPECT_TRUE(expected_paths.find(path) != expected_paths.end()) << path;
    EXPECT_EQ(1, expected_paths.erase(path));
    if (utils::StringHasSuffix(path, "/hi") ||
        utils::StringHasSuffix(path, "/hello") ||
        utils::StringHasSuffix(path, "/test") ||
        utils::StringHasSuffix(path, "/testlink")) {
      EXPECT_TRUE(S_ISREG(iter.GetStat().st_mode));
      if (utils::StringHasSuffix(path, "/test"))
        test_ino = iter.GetStat().st_ino;
      else if (utils::StringHasSuffix(path, "/testlink"))
        testlink_ino = iter.GetStat().st_ino;
    } else if (utils::StringHasSuffix(path, "/some_dir") ||
               utils::StringHasSuffix(path, "/empty_dir") ||
               utils::StringHasSuffix(path, "/mnt") ||
               utils::StringHasSuffix(path, "/lost+found") ||
               parent == path) {
      EXPECT_TRUE(S_ISDIR(iter.GetStat().st_mode));
    } else if (utils::StringHasSuffix(path, "/fifo")) {
      EXPECT_TRUE(S_ISFIFO(iter.GetStat().st_mode));
    } else if (utils::StringHasSuffix(path, "/cdev")) {
      EXPECT_TRUE(S_ISCHR(iter.GetStat().st_mode));
    } else if (utils::StringHasSuffix(path, "/sym")) {
      EXPECT_TRUE(S_ISLNK(iter.GetStat().st_mode));
    } else {
      LOG(INFO) << "got non hardcoded path: " << path;
    }
    iter.Increment();
  }
  EXPECT_EQ(testlink_ino, test_ino);
  EXPECT_NE(0, test_ino);
  EXPECT_FALSE(iter.IsErr());
  EXPECT_TRUE(expected_paths.empty());
  if (!expected_paths.empty()) {
    for (const string& path : expected_paths) {
      LOG(INFO) << "extra path: " << path;
    }
  }
}

ScopedLoopMounter::ScopedLoopMounter(const string& file_path,
                                     string* mnt_path,
                                     unsigned long flags) {
  EXPECT_TRUE(utils::MakeTempDirectory("/tmp/mnt.XXXXXX", mnt_path));
  dir_remover_.reset(new ScopedDirRemover(*mnt_path));

  string loop_dev;
  loop_binder_.reset(new ScopedLoopbackDeviceBinder(file_path, &loop_dev));

  EXPECT_TRUE(utils::MountFilesystem(loop_dev, *mnt_path, flags));
  unmounter_.reset(new ScopedFilesystemUnmounter(*mnt_path));
}

}  // namespace chromeos_update_engine
