/*
 * Copyright 2018 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkImage_Lazy_DEFINED
#define SkImage_Lazy_DEFINED

#include "include/core/SkColorSpace.h"
#include "include/core/SkImage.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkRefCnt.h"
#include "include/core/SkSamplingOptions.h"
#include "include/private/SkIDChangeListener.h"
#include "include/private/base/SkMutex.h"
#include "src/image/SkImage_Base.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <tuple>

#if SK_SUPPORT_GPU
#include "include/core/SkYUVAPixmaps.h"
class GrCaps;
class GrDirectContext;
class GrFragmentProcessor;
class GrRecordingContext;
class GrSurfaceProxyView;
#endif

class SharedGenerator;
class SkBitmap;
class SkCachedData;
class SkData;
class SkMatrix;
class SkPixmap;
enum SkColorType : int;
enum class GrColorType;
enum class GrImageTexGenPolicy : int;
enum class SkTileMode;
struct SkIRect;
struct SkRect;

namespace skgpu {
enum class Budgeted : bool;
enum class Mipmapped : bool;
}

class SkImage_Lazy : public SkImage_Base {
public:
    struct Validator {
        Validator(sk_sp<SharedGenerator>, const SkColorType*, sk_sp<SkColorSpace>);

        explicit operator bool() const { return fSharedGenerator.get(); }

        sk_sp<SharedGenerator> fSharedGenerator;
        SkImageInfo            fInfo;
        sk_sp<SkColorSpace>    fColorSpace;
        uint32_t               fUniqueID;
    };

    SkImage_Lazy(Validator* validator);

    bool onHasMipmaps() const override {
        // TODO: Should we defer to the generator? The generator interface currently doesn't have
        // a way to provide content for levels other than via SkImageGenerator::generateTexture().
        return false;
    }

    bool onReadPixels(GrDirectContext*, const SkImageInfo&, void*, size_t, int srcX, int srcY,
                      CachingHint) const override;
    sk_sp<SkData> onRefEncoded() const override;
    sk_sp<SkImage> onMakeSubset(const SkIRect&, GrDirectContext*) const override;
#ifdef SK_GRAPHITE_ENABLED
    sk_sp<SkImage> onMakeSubset(const SkIRect&,
                                skgpu::graphite::Recorder*,
                                RequiredImageProperties) const override;
    sk_sp<SkImage> onMakeColorTypeAndColorSpace(SkColorType targetCT,
                                                sk_sp<SkColorSpace> targetCS,
                                                skgpu::graphite::Recorder*,
                                                RequiredImageProperties) const override;
#endif
    bool getROPixels(GrDirectContext*, SkBitmap*, CachingHint) const override;
    bool onIsLazyGenerated() const override { return true; }
    sk_sp<SkImage> onMakeColorTypeAndColorSpace(SkColorType, sk_sp<SkColorSpace>,
                                                GrDirectContext*) const override;
    sk_sp<SkImage> onReinterpretColorSpace(sk_sp<SkColorSpace>) const final;

    bool onIsValid(GrRecordingContext*) const override;

#if SK_SUPPORT_GPU
    // Returns the texture proxy. CachingHint refers to whether the generator's output should be
    // cached in CPU memory. We will always cache the generated texture on success.
    GrSurfaceProxyView lockTextureProxyView(GrRecordingContext*,
                                            GrImageTexGenPolicy,
                                            skgpu::Mipmapped) const;

    // Returns the GrColorType to use with the GrTextureProxy returned from lockTextureProxy. This
    // may be different from the color type on the image in the case where we need up upload CPU
    // data to a texture but the GPU doesn't support the format of CPU data. In this case we convert
    // the data to RGBA_8888 unorm on the CPU then upload that.
    GrColorType colorTypeOfLockTextureProxy(const GrCaps* caps) const;
#endif

private:
    void addUniqueIDListener(sk_sp<SkIDChangeListener>) const;
    bool readPixelsProxy(GrDirectContext*, const SkPixmap&) const;
#if SK_SUPPORT_GPU
    std::tuple<GrSurfaceProxyView, GrColorType> onAsView(GrRecordingContext*,
                                                         skgpu::Mipmapped,
                                                         GrImageTexGenPolicy) const override;
    std::unique_ptr<GrFragmentProcessor> onAsFragmentProcessor(GrRecordingContext*,
                                                               SkSamplingOptions,
                                                               const SkTileMode[2],
                                                               const SkMatrix&,
                                                               const SkRect*,
                                                               const SkRect*) const override;

    GrSurfaceProxyView textureProxyViewFromPlanes(GrRecordingContext*, skgpu::Budgeted) const;
    sk_sp<SkCachedData> getPlanes(const SkYUVAPixmapInfo::SupportedDataTypes& supportedDataTypes,
                                  SkYUVAPixmaps* pixmaps) const;
#endif

#ifdef SK_GRAPHITE_ENABLED
    sk_sp<SkImage> onMakeTextureImage(skgpu::graphite::Recorder*,
                                      RequiredImageProperties) const override;
#endif

    class ScopedGenerator;

    // Note that this->imageInfo() is not necessarily the info from the generator. It may be
    // cropped by onMakeSubset and its color type/space may be changed by
    // onMakeColorTypeAndColorSpace.
    sk_sp<SharedGenerator> fSharedGenerator;

    // Repeated calls to onMakeColorTypeAndColorSpace will result in a proliferation of unique IDs
    // and SkImage_Lazy instances. Cache the result of the last successful call.
    mutable SkMutex        fOnMakeColorTypeAndSpaceMutex;
    mutable sk_sp<SkImage> fOnMakeColorTypeAndSpaceResult;

#if SK_SUPPORT_GPU
    // When the SkImage_Lazy goes away, we will iterate over all the listeners to inform them
    // of the unique ID's demise. This is used to remove cached textures from GrContext.
    mutable SkIDChangeListener::List fUniqueIDListeners;
#endif
};

#endif
