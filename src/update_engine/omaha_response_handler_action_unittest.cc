// Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include <gtest/gtest.h>

#include "update_engine/omaha_response_handler_action.h"
#include "update_engine/mock_system_state.h"
#include "update_engine/test_utils.h"
#include "update_engine/utils.h"

using std::string;
using testing::NiceMock;
using testing::Return;

namespace chromeos_update_engine {

class OmahaResponseHandlerActionTest : public ::testing::Test {
 public:
  // Return true iff the OmahaResponseHandlerAction succeeded.
  // If out is non-NULL, it's set w/ the response from the action.
  bool DoTestCommon(MockSystemState* mock_system_state,
                    const OmahaResponse& in,
                    const string& boot_dev,
                    InstallPlan* out);
  bool DoTest(const OmahaResponse& in,
              const string& boot_dev,
              InstallPlan* out);
};

namespace {
const string kLongName =
    "very_long_name_and_no_slashes-very_long_name_and_no_slashes"
    "very_long_name_and_no_slashes-very_long_name_and_no_slashes"
    "very_long_name_and_no_slashes-very_long_name_and_no_slashes"
    "very_long_name_and_no_slashes-very_long_name_and_no_slashes"
    "very_long_name_and_no_slashes-very_long_name_and_no_slashes"
    "very_long_name_and_no_slashes-very_long_name_and_no_slashes"
    "very_long_name_and_no_slashes-very_long_name_and_no_slashes"
    "-the_update_a.b.c.d_DELTA_.tgz";
}  // namespace {}

bool OmahaResponseHandlerActionTest::DoTestCommon(
    MockSystemState* mock_system_state,
    const OmahaResponse& in,
    const string& boot_dev,
    InstallPlan* out) {
  ActionProcessor processor;
  ActionTestDelegate<OmahaResponseHandlerAction> delegate;

  ObjectFeederAction<OmahaResponse> feeder_action;
  feeder_action.set_obj(in);
  if (in.update_exists) {
    EXPECT_CALL(*(mock_system_state->mock_prefs()),
                SetString(kPrefsUpdateCheckResponseHash, in.hash))
        .WillOnce(Return(true));
  }
  OmahaResponseHandlerAction response_handler_action(mock_system_state);
  response_handler_action.set_boot_device(boot_dev);
  BondActions(&feeder_action, &response_handler_action);
  ObjectCollectorAction<InstallPlan> collector_action;
  BondActions(&response_handler_action, &collector_action);
  processor.EnqueueAction(&feeder_action);
  processor.EnqueueAction(&response_handler_action);
  processor.EnqueueAction(&collector_action);
  delegate.RunProcessor(&processor);
  if (out)
    *out = collector_action.object();
  EXPECT_TRUE(delegate.ran());
  return delegate.code() == kActionCodeSuccess;
}

bool OmahaResponseHandlerActionTest::DoTest(const OmahaResponse& in,
                                            const string& boot_dev,
                                            InstallPlan* out) {
  MockSystemState mock_system_state;
  return DoTestCommon(&mock_system_state, in, boot_dev, out);
}

TEST_F(OmahaResponseHandlerActionTest, SimpleTest) {
  {
    OmahaResponse in;
    in.update_exists = true;
    in.display_version = "a.b.c.d";
    in.payload_urls.push_back("http://foo/the_update_a.b.c.d.tgz");
    in.hash = "HASH+";
    in.size = 12;
    in.needs_admin = true;
    in.prompt = false;
    InstallPlan install_plan;
    EXPECT_TRUE(DoTest(in, "/dev/sda3", &install_plan));
    EXPECT_EQ(in.payload_urls[0], install_plan.download_url);
    EXPECT_EQ(in.hash, install_plan.payload_hash);
    EXPECT_EQ(in.display_version, install_plan.display_version);
    EXPECT_EQ("/dev/sda4", install_plan.partition_path);
  }
  {
    OmahaResponse in;
    in.update_exists = true;
    in.display_version = "a.b.c.d";
    in.payload_urls.push_back("http://foo/the_update_a.b.c.d.tgz");
    in.hash = "HASHj+";
    in.size = 12;
    in.needs_admin = true;
    in.prompt = true;
    InstallPlan install_plan;
    EXPECT_TRUE(DoTest(in, "/dev/sda4", &install_plan));
    EXPECT_EQ(in.payload_urls[0], install_plan.download_url);
    EXPECT_EQ(in.hash, install_plan.payload_hash);
    EXPECT_EQ(in.display_version, install_plan.display_version);
    EXPECT_EQ("/dev/sda3", install_plan.partition_path);
  }
  {
    OmahaResponse in;
    in.update_exists = true;
    in.display_version = "a.b.c.d";
    in.payload_urls.push_back(kLongName);
    in.hash = "HASHj+";
    in.size = 12;
    in.needs_admin = true;
    in.prompt = true;
    InstallPlan install_plan;
    EXPECT_TRUE(DoTest(in, "/dev/sda3", &install_plan));
    EXPECT_EQ(in.payload_urls[0], install_plan.download_url);
    EXPECT_EQ(in.hash, install_plan.payload_hash);
    EXPECT_EQ(in.display_version, install_plan.display_version);
    EXPECT_EQ("/dev/sda4", install_plan.partition_path);
  }
}

TEST_F(OmahaResponseHandlerActionTest, NoUpdatesTest) {
  OmahaResponse in;
  in.update_exists = false;
  InstallPlan install_plan;
  EXPECT_FALSE(DoTest(in, "/dev/sda1", &install_plan));
  EXPECT_EQ("", install_plan.download_url);
  EXPECT_EQ("", install_plan.payload_hash);
  EXPECT_EQ("", install_plan.partition_path);
}

}  // namespace chromeos_update_engine
