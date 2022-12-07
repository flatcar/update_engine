// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gflags/gflags.h>
#include <glib.h>
#include <glog/logging.h>
#include <sodium.h>

#include "update_engine/certificate_checker.h"
#include "update_engine/dbus_constants.h"
#include "update_engine/dbus_interface.h"
#include "update_engine/dbus_service.h"
#include "update_engine/real_system_state.h"
#include "update_engine/subprocess.h"
#include "update_engine/terminator.h"
#include "update_engine/update_attempter.h"
#include "update_engine/update_check_scheduler.h"
#include "update_engine/utils.h"

extern "C" {
#include "update_engine/update_engine.dbusserver.h"
}

DEFINE_bool(foreground, false,
            "Don't daemon()ize; run in foreground.");


namespace chromeos_update_engine {

gboolean UpdateBootFlags(void* arg) {
  reinterpret_cast<UpdateAttempter*>(arg)->UpdateBootFlags();
  return FALSE;  // Don't call this callback again
}

gboolean BroadcastStatus(void* arg) {
  reinterpret_cast<UpdateAttempter*>(arg)->BroadcastStatus();
  return FALSE;  // Don't call this callback again
}

namespace {

void SetupDbusService(UpdateEngineService* service) {
  DBusGConnection *bus;
  DBusGProxy *proxy;
  GError *error = NULL;

  bus = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
  LOG_IF(FATAL, !bus) << "Failed to get bus: "
                      << utils::GetAndFreeGError(&error);
  proxy = dbus_g_proxy_new_for_name(bus,
                                    DBUS_SERVICE_DBUS,
                                    DBUS_PATH_DBUS,
                                    DBUS_INTERFACE_DBUS);
  guint32 request_name_ret;
  if (!org_freedesktop_DBus_request_name(proxy,
                                         kUpdateEngineServiceName,
                                         0,
                                         &request_name_ret,
                                         &error)) {
    LOG(FATAL) << "Failed to get name: " << utils::GetAndFreeGError(&error);
  }
  if (request_name_ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
    g_warning("Got result code %u from requesting name", request_name_ret);
    LOG(FATAL) << "Got result code " << request_name_ret
               << " from requesting name, but expected "
               << DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER;
  }
  dbus_g_connection_register_g_object(bus,
                                      "/com/coreos/update1",
                                      G_OBJECT(service));
}

}  // namespace {}
}  // namespace chromeos_update_engine

int main(int argc, char** argv) {
  PLOG_IF(FATAL, sodium_init() < 0 ) << "the cryptographic lib couldn't be initialized; it is not safe to use";

  // Disable glog's default behavior of logging to files.
  FLAGS_logtostderr = true;
  GFLAGS_NAMESPACE::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  dbus_threads_init_default();
  chromeos_update_engine::Terminator::Init();
  chromeos_update_engine::Subprocess::Init();

  if (!FLAGS_foreground)
    PLOG_IF(FATAL, daemon(0, 0) == 1) << "daemon() failed";

  LOG(INFO) << "Flatcar Update Engine starting";

  // Ensure that all written files have safe permissions.
  // This is a mask, so we _block_ execute for the owner, and ALL
  // permissions for other users.
  // Done _after_ log file creation.
  umask(S_IXUSR | S_IRWXG | S_IRWXO);

  // Create the single GMainLoop
  GMainLoop* loop = g_main_loop_new(g_main_context_default(), FALSE);

  chromeos_update_engine::RealSystemState real_system_state;
  LOG_IF(ERROR, !real_system_state.Initialize())
      << "Failed to initialize system state.";
  chromeos_update_engine::UpdateAttempter *update_attempter =
      real_system_state.update_attempter();
  CHECK(update_attempter);

  // Sets static members for the certificate checker.
  chromeos_update_engine::CertificateChecker::set_system_state(
      &real_system_state);
  chromeos_update_engine::OpenSSLWrapper openssl_wrapper;
  chromeos_update_engine::CertificateChecker::set_openssl_wrapper(
      &openssl_wrapper);

  // Create the dbus service object:
  dbus_g_object_type_install_info(UPDATE_ENGINE_TYPE_SERVICE,
                                  &dbus_glib_update_engine_service_object_info);
  UpdateEngineService* service =
      UPDATE_ENGINE_SERVICE(g_object_new(UPDATE_ENGINE_TYPE_SERVICE, NULL));
  service->system_state_ = &real_system_state;
  update_attempter->set_dbus_service(service);
  chromeos_update_engine::SetupDbusService(service);

  // Schedule periodic update checks.
  chromeos_update_engine::UpdateCheckScheduler scheduler(update_attempter,
                                                         &real_system_state);
  scheduler.Run();

  // Update boot flags after 45 seconds.
  g_timeout_add_seconds(45,
                        &chromeos_update_engine::UpdateBootFlags,
                        update_attempter);

  // Broadcast the update engine status on startup to ensure consistent system
  // state on crashes.
  g_idle_add(&chromeos_update_engine::BroadcastStatus, update_attempter);

  // Run the main loop until exit time:
  g_main_loop_run(loop);

  // Cleanup:
  g_main_loop_unref(loop);
  update_attempter->set_dbus_service(NULL);
  g_object_unref(G_OBJECT(service));

  LOG(INFO) << "Flatcar Update Engine terminating";
  return 0;
}
