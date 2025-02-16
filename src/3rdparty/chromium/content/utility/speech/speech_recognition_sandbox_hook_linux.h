// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_UTILITY_SPEECH_SPEECH_RECOGNITION_SANDBOX_HOOK_LINUX_H_
#define CONTENT_UTILITY_SPEECH_SPEECH_RECOGNITION_SANDBOX_HOOK_LINUX_H_

#if defined(OS_BSD)
#include "sandbox/policy/openbsd/sandbox_openbsd.h"
#else
#include "sandbox/policy/linux/sandbox_linux.h"
#endif

namespace speech {

// Opens the libsoda.so binary and grants broker file permissions to the
// necessary files required by the binary.
bool SpeechRecognitionPreSandboxHook(
    sandbox::policy::SandboxLinux::Options options);

}  // namespace speech

#endif  // CONTENT_UTILITY_SPEECH_SPEECH_RECOGNITION_SANDBOX_HOOK_LINUX_H_
