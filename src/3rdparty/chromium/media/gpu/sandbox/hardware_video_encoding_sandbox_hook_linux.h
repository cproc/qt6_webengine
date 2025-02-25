// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_SANDBOX_HARDWARE_VIDEO_ENCODING_SANDBOX_HOOK_LINUX_H_
#define MEDIA_GPU_SANDBOX_HARDWARE_VIDEO_ENCODING_SANDBOX_HOOK_LINUX_H_

#include "build/build_config.h"

#if BUILDFLAG(IS_BSD)
#include "sandbox/policy/openbsd/sandbox_openbsd.h"
#else
#include "sandbox/policy/linux/sandbox_linux.h"
#endif

namespace media {

bool HardwareVideoEncodingPreSandboxHook(
    sandbox::policy::SandboxLinux::Options options);

}  // namespace media

#endif  // MEDIA_GPU_SANDBOX_HARDWARE_VIDEO_ENCODING_SANDBOX_HOOK_LINUX_H_
