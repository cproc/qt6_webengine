// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/metrics/histogram_macros.h"
#include "base/memory/ptr_util.h"

#include "media/audio/genode/audio_manager_genode.h"

#include "media/audio/audio_device_description.h"
#include "media/audio/audio_output_dispatcher.h"
#if defined(USE_OSS)
#include "media/audio/oss/oss_input.h"
#include "media/audio/oss/oss_output.h"
#else
#include "media/audio/fake_audio_manager.h"
#endif
#include "media/base/limits.h"
#include "media/base/media_switches.h"

namespace media {

enum GenodeAudioIO {
  kOss,
  kAudioIOMax = kOss
};

#if defined(USE_OSS)
// Maximum number of output streams that can be open simultaneously.
static const int kMaxOutputStreams = 2;

// Default sample rate for input and output streams.
static const int kDefaultSampleRate = 44100;

void AddDefaultDevice(AudioDeviceNames* device_names) {
  DCHECK(device_names->empty());
  device_names->push_front(AudioDeviceName::CreateDefault());
}

bool AudioManagerGenode::HasAudioOutputDevices() {
  return true;
}

bool AudioManagerGenode::HasAudioInputDevices() {
  return true;
}

void AudioManagerGenode::GetAudioInputDeviceNames(
    AudioDeviceNames* device_names) {
  DCHECK(device_names->empty());
  AddDefaultDevice(device_names);
}

void AudioManagerGenode::GetAudioOutputDeviceNames(
    AudioDeviceNames* device_names) {
  AddDefaultDevice(device_names);
}

#if defined(USE_OSS)
const char* AudioManagerGenode::GetName() {
  return "OSS";
}
#endif

AudioParameters AudioManagerGenode::GetInputStreamParameters(
    const std::string& device_id) {
  static const int kDefaultInputBufferSize = 1024;

  int user_buffer_size = GetUserBufferSize();
  int buffer_size = user_buffer_size ?
      user_buffer_size : kDefaultInputBufferSize;

  return AudioParameters(
      AudioParameters::AUDIO_PCM_LOW_LATENCY, ChannelLayoutConfig::Stereo(),
      kDefaultSampleRate, buffer_size);
}

AudioManagerGenode::AudioManagerGenode(std::unique_ptr<AudioThread> audio_thread,
                                         AudioLogFactory* audio_log_factory)
    : AudioManagerBase(std::move(audio_thread),
                       audio_log_factory) {
  DLOG(WARNING) << "AudioManagerGenode";
  SetMaxOutputStreamsAllowed(kMaxOutputStreams);
}

AudioManagerGenode::~AudioManagerGenode() {
  Shutdown();
}

AudioOutputStream* AudioManagerGenode::MakeLinearOutputStream(
    const AudioParameters& params,
    const LogCallback& log_callback) {
  DCHECK_EQ(AudioParameters::AUDIO_PCM_LINEAR, params.format());
  return MakeOutputStream(params);
}

AudioOutputStream* AudioManagerGenode::MakeLowLatencyOutputStream(
    const AudioParameters& params,
    const std::string& device_id,
    const LogCallback& log_callback) {
  DLOG_IF(ERROR, !device_id.empty()) << "Not implemented!";
  DCHECK_EQ(AudioParameters::AUDIO_PCM_LOW_LATENCY, params.format());
  return MakeOutputStream(params);
}

AudioInputStream* AudioManagerGenode::MakeLinearInputStream(
    const AudioParameters& params,
    const std::string& device_id,
    const LogCallback& log_callback) {
  DCHECK_EQ(AudioParameters::AUDIO_PCM_LINEAR, params.format());
  return MakeInputStream(params);
}

AudioInputStream* AudioManagerGenode::MakeLowLatencyInputStream(
    const AudioParameters& params,
    const std::string& device_id,
    const LogCallback& log_callback) {
  DCHECK_EQ(AudioParameters::AUDIO_PCM_LOW_LATENCY, params.format());
  return MakeInputStream(params);
}

AudioParameters AudioManagerGenode::GetPreferredOutputStreamParameters(
    const std::string& output_device_id,
    const AudioParameters& input_params) {
  // TODO(tommi): Support |output_device_id|.
  DLOG_IF(ERROR, !output_device_id.empty()) << "Not implemented!";
  static const int kDefaultOutputBufferSize = 4*2048;

  ChannelLayoutConfig channel_layout_config = ChannelLayoutConfig::Stereo();
  int sample_rate = kDefaultSampleRate;
  int buffer_size = kDefaultOutputBufferSize;
  if (input_params.IsValid()) {
    sample_rate = input_params.sample_rate();
    channel_layout_config = input_params.channel_layout_config();
    buffer_size = std::min(buffer_size, input_params.frames_per_buffer());
  }

  int user_buffer_size = GetUserBufferSize();
  if (user_buffer_size)
    buffer_size = user_buffer_size;

  return AudioParameters(
      AudioParameters::AUDIO_PCM_LOW_LATENCY,
      channel_layout_config, sample_rate, buffer_size);
}

AudioInputStream* AudioManagerGenode::MakeInputStream(
    const AudioParameters& params) {
  DLOG(WARNING) << "MakeInputStream";
  return new OssAudioInputStream(this,
             AudioDeviceDescription::kDefaultDeviceId, params);
}

AudioOutputStream* AudioManagerGenode::MakeOutputStream(
    const AudioParameters& params) {
  DLOG(WARNING) << "MakeOutputStream";
  return new OssAudioOutputStream(params, this);
}
#endif

std::unique_ptr<media::AudioManager> CreateAudioManager(
    std::unique_ptr<AudioThread> audio_thread,
    AudioLogFactory* audio_log_factory) {
  DLOG(WARNING) << "CreateAudioManager";
#if defined(USE_OSS)
  UMA_HISTOGRAM_ENUMERATION("Media.GenodeAudioIO", kOss, kAudioIOMax + 1);
  return std::make_unique<AudioManagerGenode>(std::move(audio_thread),
                                              audio_log_factory);
#else
  return std::make_unique<FakeAudioManager>(std::move(audio_thread),
                                            audio_log_factory);
#endif

}

}  // namespace media
