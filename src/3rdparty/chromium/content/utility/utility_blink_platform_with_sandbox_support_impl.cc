// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/utility/utility_blink_platform_with_sandbox_support_impl.h"

#include "build/build_config.h"
#include "content/public/utility/utility_thread.h"

#if BUILDFLAG(IS_MAC)
#include "content/child/child_process_sandbox_support_impl_mac.h"
#elif BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_CHROMEOS) || BUILDFLAG(IS_BSD)
#include "content/child/child_process_sandbox_support_impl_linux.h"
#endif

namespace content {

UtilityBlinkPlatformWithSandboxSupportImpl::
    UtilityBlinkPlatformWithSandboxSupportImpl() {
#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_CHROMEOS) || BUILDFLAG(IS_BSD)
  mojo::PendingRemote<font_service::mojom::FontService> font_service;
  UtilityThread::Get()->BindHostReceiver(
      font_service.InitWithNewPipeAndPassReceiver());
  font_loader_ = sk_make_sp<font_service::FontLoader>(std::move(font_service));
  SkFontConfigInterface::SetGlobal(font_loader_);
  sandbox_support_ = std::make_unique<WebSandboxSupportLinux>(font_loader_);
#elif BUILDFLAG(IS_MAC)
  sandbox_support_ = std::make_unique<WebSandboxSupportMac>();
#endif
}

UtilityBlinkPlatformWithSandboxSupportImpl::
    ~UtilityBlinkPlatformWithSandboxSupportImpl() {}

blink::WebSandboxSupport*
UtilityBlinkPlatformWithSandboxSupportImpl::GetSandboxSupport() {
#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_CHROMEOS) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_BSD)
  return sandbox_support_.get();
#else
  return nullptr;
#endif
}

}  // namespace content
