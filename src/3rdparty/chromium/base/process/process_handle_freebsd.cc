// Copyright 2011 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/process/process_handle.h"
#include "base/files/file_util.h"

#include <limits.h>
#include <stddef.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#if 0
#include <sys/user.h>
#endif
#include <unistd.h>

namespace base {

ProcessId GetParentProcessId(ProcessHandle process) {
#if 0
  struct kinfo_proc info;
  size_t length = sizeof(struct kinfo_proc);
  int mib[] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, process };

  if (sysctl(mib, std::size(mib), &info, &length, NULL, 0) < 0)
    return -1;

  if (length < sizeof(struct kinfo_proc))
    return -1;

  return info.ki_ppid;
#else
  return -1;
#endif
}

FilePath GetProcessExecutablePath(ProcessHandle process) {
  char pathname[PATH_MAX];
  size_t length;
  int mib[] = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, process };

  length = sizeof(pathname);

  if (sysctl(mib, std::size(mib), pathname, &length, NULL, 0) < 0 ||
      length == 0) {
    return FilePath();
  }

  return FilePath(std::string(pathname));
}

}  // namespace base
