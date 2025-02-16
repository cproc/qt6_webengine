// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "util/exe_path.h"

#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "util/build_config.h"

#if defined(OS_MACOSX)
#include <mach-o/dyld.h>
#elif defined(OS_WIN)
#include <windows.h>

#include "base/win/win_util.h"
#elif defined(OS_FREEBSD) || defined(OS_NETBSD) || defined(OS_OPENBSD)
#include <limits.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#elif defined(OS_HAIKU)
#include <OS.h>
#include <image.h>
#elif defined(OS_SOLARIS)
#include <stdlib.h>
#endif

#if defined(OS_OPENBSD)
#include <kvm.h>
#endif

#if defined(OS_MACOSX)

base::FilePath GetExePath() {
  // Executable path can have relative references ("..") depending on
  // how the app was launched.
  uint32_t executable_length = 0;
  _NSGetExecutablePath(NULL, &executable_length);
  DCHECK_GT(executable_length, 1u);
  std::string executable_path;
  int rv = _NSGetExecutablePath(
      base::WriteInto(&executable_path, executable_length), &executable_length);
  DCHECK_EQ(rv, 0);

  // _NSGetExecutablePath may return paths containing ./ or ../ which makes
  // FilePath::DirName() work incorrectly, convert it to absolute path so that
  // paths such as DIR_SOURCE_ROOT can work, since we expect absolute paths to
  // be returned here.
  return base::MakeAbsoluteFilePath(base::FilePath(executable_path));
}

#elif defined(OS_WIN)

base::FilePath GetExePath() {
  char16_t system_buffer[MAX_PATH];
  system_buffer[0] = 0;
  if (GetModuleFileName(NULL, base::ToWCharT(system_buffer), MAX_PATH) == 0) {
    return base::FilePath();
  }
  return base::FilePath(system_buffer);
}

#elif defined(OS_FREEBSD)

base::FilePath GetExePath() {
  int mib[] = {CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1};
  char buf[PATH_MAX];
  size_t buf_size = PATH_MAX;
  if (sysctl(mib, 4, buf, &buf_size, nullptr, 0) == -1) {
    return base::FilePath();
  }
  return base::FilePath(buf);
}

#elif defined(OS_NETBSD)

base::FilePath GetExePath() {
  int mib[] = {CTL_KERN, KERN_PROC_ARGS, getpid(), KERN_PROC_PATHNAME};
  char buf[PATH_MAX];
  size_t buf_size = PATH_MAX;
  if (sysctl(mib, 4, buf, &buf_size, nullptr, 0) == -1) {
    return base::FilePath();
  }
  return base::FilePath(buf);
}

#elif defined(OS_HAIKU)

base::FilePath GetExePath() {
  image_info i_info;
  int32 image_cookie = 0;
  while (get_next_image_info(B_CURRENT_TEAM, &image_cookie, &i_info) == B_OK) {
    if (i_info.type == B_APP_IMAGE) {
      break;
    }
  }
  return base::FilePath(std::string(i_info.name));
}

#elif defined(OS_SOLARIS)

base::FilePath GetExePath() {
  const char *raw = getexecname();
  if (raw == NULL) {
    return base::FilePath();
  }
  return base::FilePath(raw);
}

#elif defined(OS_OPENBSD)

base::FilePath GetExePath() {
  struct kinfo_file *files;
  kvm_t *kd = NULL;
  char errbuf[_POSIX2_LINE_MAX];
  char **retvalargs;
#define MAXTOKENS 2
  char *tokens[MAXTOKENS];
  static char retval[PATH_MAX];
  int cnt;
  size_t len;
  struct stat sb;
  pid_t cpid = getpid();

  int mib[] = { CTL_KERN, KERN_PROC_ARGS, cpid, KERN_PROC_ARGV };

  if (sysctl(mib, 4, NULL, &len, NULL, 0) != -1) {
    retvalargs = static_cast<char**>(malloc(len));
    if (!retvalargs)
      goto out;

    if (sysctl(mib, 4, retvalargs, &len, NULL, 0) < 0)
      goto out;

    char *cr = strdup(retvalargs[0]);
    free(retvalargs);

    *tokens = strtok(cr, ":");
    if (tokens[0] == NULL)
      goto out;

    if (realpath(tokens[0], retval) == NULL)
      goto out;

    if (stat(retval, &sb) < 0)
      goto out;

    if ((kd = kvm_openfiles(NULL, NULL, NULL, KVM_NO_FILES, errbuf)) == NULL)
      goto out;

    if ((files = kvm_getfiles(kd, KERN_FILE_BYPID, cpid,
                              sizeof(struct kinfo_file), &cnt)) == NULL) {
      kvm_close(kd); 
      goto out;
    }

    for (int i = 0; i < cnt; i++) {
      if (files[i].fd_fd == KERN_FILE_TEXT &&
          files[i].va_fsid == static_cast<uint32_t>(sb.st_dev) &&
          files[i].va_fileid == sb.st_ino) {
        kvm_close(kd);
        return base::FilePath(retval);
      }
    }
  }

out:
  return base::FilePath();
}

#elif defined(OS_ZOS)

base::FilePath GetExePath() {
  char path[PATH_MAX];
  if (__getexepath(path, sizeof(path), getpid()) != 0) {
    return base::FilePath();
  }
  return base::FilePath(path);
}

#else

base::FilePath GetExePath() {
  base::FilePath result;
  const char kProcSelfExe[] = "/proc/self/exe";
  if (!ReadSymbolicLink(base::FilePath(kProcSelfExe), &result)) {
    NOTREACHED() << "Unable to resolve " << kProcSelfExe << ".";
    return base::FilePath();
  }
  return result;
}

#endif
