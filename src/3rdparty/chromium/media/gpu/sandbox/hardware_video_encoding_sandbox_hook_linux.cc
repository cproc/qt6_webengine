// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/sandbox/hardware_video_encoding_sandbox_hook_linux.h"

#include <dlfcn.h>
#include <sys/stat.h>

#include "base/strings/stringprintf.h"
#include "media/gpu/buildflags.h"

#if BUILDFLAG(USE_VAAPI)
#include "media/gpu/vaapi/vaapi_wrapper.h"
#endif

#if BUILDFLAG(USE_V4L2_CODEC)
#include "media/gpu/v4l2/v4l2_device.h"
#endif

#if !BUILDFLAG(IS_BSD)
using sandbox::syscall_broker::BrokerFilePermission;
#endif

namespace media {

bool HardwareVideoEncodingPreSandboxHook(
    sandbox::policy::SandboxLinux::Options options) {
#if !BUILDFLAG(IS_BSD)
  sandbox::syscall_broker::BrokerCommandSet command_set;
  std::vector<BrokerFilePermission> permissions;

#if BUILDFLAG(USE_V4L2_CODEC)
  command_set.set(sandbox::syscall_broker::COMMAND_OPEN);

  // Device nodes for V4L2 video encode accelerator drivers.
  // We do not use a FileEnumerator because the device files may not exist
  // yet when the sandbox is created. But since we are restricting access
  // to the video-enc* prefix we know that we cannot
  // authorize a non-encoder device by accident.
  static constexpr size_t MAX_V4L2_ENCODERS = 5;
  static const base::FilePath::CharType kVideoEncBase[] = "/dev/video-enc";
  permissions.push_back(BrokerFilePermission::ReadWrite(kVideoEncBase));
  for (size_t i = 0; i < MAX_V4L2_ENCODERS; i++) {
    std::ostringstream encoderPath;
    encoderPath << kVideoEncBase << i;
    permissions.push_back(BrokerFilePermission::ReadWrite(encoderPath.str()));
  }

  // Image processor used on ARM platforms.
  // TODO(b/248528896): it's possible not all V4L2 platforms need an image
  // processor for video encoding. Look into whether we can restrict this
  // permission to only platforms that need it.
  static const char kDevImageProc0Path[] = "/dev/image-proc0";
  permissions.push_back(BrokerFilePermission::ReadWrite(kDevImageProc0Path));
#elif BUILDFLAG(USE_VAAPI)
  command_set.set(sandbox::syscall_broker::COMMAND_OPEN);

  if (options.use_amd_specific_policies) {
    command_set.set(sandbox::syscall_broker::COMMAND_ACCESS);
    command_set.set(sandbox::syscall_broker::COMMAND_STAT);
    command_set.set(sandbox::syscall_broker::COMMAND_READLINK);

    permissions.push_back(BrokerFilePermission::ReadOnly("/dev/dri"));

    static const char* kDevices[] = {"/sys/dev/char", "/sys/devices"};
    for (const char* item : kDevices) {
      std::string path(item);
      permissions.push_back(
          BrokerFilePermission::StatOnlyWithIntermediateDirs(path));
      permissions.push_back(
          BrokerFilePermission::ReadOnlyRecursive(path + "/"));
    }
  }
#endif

  // TODO(b/248528896): figure out if the render node needs to be opened after
  // entering the sandbox or if this can be restricted based on API (VA-API vs.
  // V4L2) or use case (e.g., Chrome vs. ARC++/ARCVM).
  for (int i = 128; i <= 137; ++i) {
    const std::string path = base::StringPrintf("/dev/dri/renderD%d", i);
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
      permissions.push_back(options.use_amd_specific_policies
                                ? BrokerFilePermission::ReadWrite(path)
                                : BrokerFilePermission::ReadOnly(path));
    }
  }

  sandbox::policy::SandboxLinux::GetInstance()->StartBrokerProcess(
      command_set, permissions, sandbox::policy::SandboxLinux::PreSandboxHook(),
      options);

  // TODO(b/248528896): the hardware video encoding sandbox is really only
  // useful when building with VA-API or V4L2 (otherwise, we're not really doing
  // hardware video encoding). Consider restricting the kHardwareVideoEncoding
  // sandbox type to exist only in those configurations so that the presandbox
  // hook is only reached in those scenarios. As it is now,
  // kHardwareVideoEncoding exists for all ash-chrome builds because
  // chrome/browser/ash/arc/video/gpu_arc_video_service_host.cc is expected to
  // depend on it eventually and that file is built for ash-chrome regardless
  // of VA-API/V4L2. That means that bots like linux-chromeos-rel would end up
  // reaching this presandbox hook.
#if BUILDFLAG(USE_VAAPI)
  VaapiWrapper::PreSandboxInitialization();

  if (options.use_amd_specific_policies) {
    const char* radeonsi_lib = "/usr/lib64/dri/radeonsi_dri.so";
#if defined(DRI_DRIVER_DIR)
    radeonsi_lib = DRI_DRIVER_DIR "/radeonsi_dri.so";
#endif
    if (nullptr ==
        dlopen(radeonsi_lib, RTLD_NOW | RTLD_GLOBAL | RTLD_NODELETE)) {
      LOG(ERROR) << "dlopen(radeonsi_dri.so) failed with error: " << dlerror();
      return false;
    }
  }
#elif BUILDFLAG(USE_V4L2_CODEC)
  if (V4L2Device::UseLibV4L2()) {
#if defined(__aarch64__)
    dlopen("/usr/lib64/libv4l2.so", RTLD_NOW | RTLD_GLOBAL | RTLD_NODELETE);
    dlopen("/usr/lib64/libv4l/plugins/libv4l-encplugin.so",
           RTLD_NOW | RTLD_GLOBAL | RTLD_NODELETE);
#else
    dlopen("/usr/lib/libv4l2.so", RTLD_NOW | RTLD_GLOBAL | RTLD_NODELETE);
    dlopen("/usr/lib/libv4l/plugins/libv4l-encplugin.so",
           RTLD_NOW | RTLD_GLOBAL | RTLD_NODELETE);
#endif  // defined(__aarch64__)
  }
#endif
#endif
  return true;
}

}  // namespace media
