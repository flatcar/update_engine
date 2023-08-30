// Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "update_engine/omaha_response_handler_action.h"

#include <string>

#include <glog/logging.h>

#include "files/file_util.h"
#include "update_engine/payload_processor.h"
#include "update_engine/payload_state_interface.h"
#include "update_engine/prefs_interface.h"
#include "update_engine/utils.h"

using std::string;

namespace chromeos_update_engine {

OmahaResponseHandlerAction::OmahaResponseHandlerAction(
    SystemState* system_state)
    : system_state_(system_state),
      got_no_update_response_(false),
      key_path_(PayloadProcessor::kUpdatePayloadPublicKeyPath) {}

void OmahaResponseHandlerAction::PerformAction() {
  CHECK(HasInputObject());
  ScopedActionCompleter completer(processor_, this);
  const OmahaResponse& response = GetInputObject();
  if (!response.update_exists) {
    got_no_update_response_ = true;
    LOG(INFO) << "There are no updates. Aborting.";
    return;
  }

  // All decisions as to which URL should be used have already been done. So,
  // make the download URL as the payload URL at the current url index.
  uint32_t url_index = system_state_->payload_state()->GetUrlIndex();
  LOG(INFO) << "Using Url" << url_index << " as the download url this time";
  CHECK(url_index < response.payload_urls.size());
  install_plan_.download_url = response.payload_urls[url_index];

  // Fill up the other properties based on the response.
  install_plan_.display_version = response.display_version;
  install_plan_.payload_size = response.size;
  install_plan_.payload_hash = response.hash;
  install_plan_.is_resume =
      PayloadProcessor::CanResumeUpdate(system_state_->prefs(), response.hash);
  if (!install_plan_.is_resume) {
    LOG_IF(WARNING, !PayloadProcessor::ResetUpdateProgress(
        system_state_->prefs(), false))
        << "Unable to reset the update progress.";
    LOG_IF(WARNING, !system_state_->prefs()->SetString(
        kPrefsUpdateCheckResponseHash, response.hash))
        << "Unable to save the update check response hash.";
  }

  if (install_plan_.old_partition_path.empty())
    install_plan_.old_partition_path = utils::BootDevice();
  TEST_AND_RETURN(!install_plan_.old_partition_path.empty());

  TEST_AND_RETURN(GetInstallDev(
      install_plan_.old_partition_path,
      &install_plan_.partition_path));

  TEST_AND_RETURN(GetKernelPath(
      install_plan_.old_partition_path,
      &install_plan_.old_kernel_path));

  TEST_AND_RETURN(GetKernelPath(
      install_plan_.partition_path,
      &install_plan_.kernel_path));

  TEST_AND_RETURN(GetPCRPolicyPath(
      install_plan_.partition_path,
      &install_plan_.pcr_policy_path));

  TEST_AND_RETURN(HasOutputPipe());
  if (HasOutputPipe())
    SetOutputObject(install_plan_);
  LOG(INFO) << "Using this install plan:";
  install_plan_.Dump();

  completer.set_code(kActionCodeSuccess);
}

bool OmahaResponseHandlerAction::GetInstallDev(const std::string& boot_dev,
                                               std::string* install_dev) {
  TEST_AND_RETURN_FALSE(utils::StringHasPrefix(boot_dev, "/dev/"));
  string ret(boot_dev);
  string::reverse_iterator it = ret.rbegin();  // last character in string
  // Right now, we just switch '3' and '4' partition numbers.
  TEST_AND_RETURN_FALSE((*it == '3') || (*it == '4'));
  *it = (*it == '3') ? '4' : '3';
  *install_dev = ret;
  return true;
}

bool OmahaResponseHandlerAction::GetKernelPath(const std::string& part_path,
                                               std::string* kernel_path) {
  files::FilePath coreos_kernel_a = files::FilePath("/boot/coreos/vmlinuz-a");
  files::FilePath coreos_kernel_b = files::FilePath("/boot/coreos/vmlinuz-b");

  // If the target fs is 3, the kernel name is vmlinuz-a.
  // If the target fs is 4, the kernel name is vmlinuz-b.
  char last_char = part_path[part_path.size() - 1];
  if (last_char == '3') {
    if (files::PathExists(coreos_kernel_a) || files::PathExists(coreos_kernel_b))
      *kernel_path = "/boot/coreos/vmlinuz-a";
    else
      *kernel_path = "/boot/flatcar/vmlinuz-a";
    return true;
  }
  if (last_char == '4') {
    if (files::PathExists(coreos_kernel_a) || files::PathExists(coreos_kernel_b))
      *kernel_path = "/boot/coreos/vmlinuz-b";
    else
      *kernel_path = "/boot/flatcar/vmlinuz-b";
    return true;
  }
  return false;
}

bool OmahaResponseHandlerAction::GetPCRPolicyPath(const std::string& part_path,
                                                  std::string* policy_path) {
  char last_char = part_path[part_path.size() - 1];
  if (last_char == '3') {
    *policy_path = "/var/lib/update_engine/pcrs-a.zip";
    return true;
  }
  if (last_char == '4') {
    *policy_path = "/var/lib/update_engine/pcrs-b.zip";
    return true;
  }
  return false;
}

}  // namespace chromeos_update_engine
