// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_AUDIO_SNDIO_SNDIO_INPUT_H_
#define MEDIA_AUDIO_SNDIO_SNDIO_INPUT_H_

#include <stdint.h>
#include <string>

#include "base/compiler_specific.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "media/audio/agc_audio_stream.h"
#include "media/audio/audio_io.h"
#include "media/audio/audio_device_description.h"
#include "media/base/audio_parameters.h"

namespace media {

class AudioManagerBase;

// Implementation of AudioOutputStream using sndio(7)
class OssAudioInputStream : public AgcAudioStream<AudioInputStream> {
 public:
  // Pass this to the constructor if you want to attempt auto-selection
  // of the audio recording device.
  static const char kAutoSelectDevice[];

  // Create a PCM Output stream for the SNDIO device identified by
  // |device_name|. If unsure of what to use for |device_name|, use
  // |kAutoSelectDevice|.
  OssAudioInputStream(AudioManagerBase* audio_manager,
                     const std::string& device_name,
                     const AudioParameters& params);

  ~OssAudioInputStream() override;

  // Implementation of AudioInputStream.
  OpenOutcome Open() override;
  void Start(AudioInputCallback* callback) override;
  void Stop() override;
  void Close() override;
  double GetMaxVolume() override;
  void SetVolume(double volume) override;
  double GetVolume() override;
  bool IsMuted() override;
  void SetOutputDeviceForAec(const std::string& output_device_id) override;

 private:

  enum StreamState {
    kClosed,            // Not opened yet
    kStopped,           // Device opened, but not started yet
    kRunning,           // Started, device playing
    kStopWait           // Stopping, waiting for the real-time thread to exit
  };

  // C-style call-backs
  static void* ThreadEntry(void *arg);

  // Continuously moves data from the device to the consumer
  void ThreadLoop();
  // Our creator, the audio manager needs to be notified when we close.
  AudioManagerBase* manager;
  // Parameters of the source
  AudioParameters params;
  // We store data here for consumer
  std::unique_ptr<AudioBus> audio_bus;
  // Call-back that consumes recorded data
  AudioInputCallback* callback;  // Valid during a recording session.

  // Handle of the OSS audio device
  int read_fd;

  // Current state of the stream
  enum StreamState state;
  // High priority thread running ThreadLoop()
  pthread_t thread;
  // Number of frames buffered in the hardware
  int hw_delay;
  // Temporary buffer where data is stored sndio-compatible format
  char* buffer;
};

}  // namespace media

#endif  // MEDIA_AUDIO_SNDIO_SNDIO_INPUT_H_
