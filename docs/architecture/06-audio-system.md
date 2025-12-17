# Audio System Architecture

The audio system provides cross-platform sound playback for music and sound effects.

## Design Goals

1. **Platform abstraction** - Same API for CoreAudio, Web Audio, etc.
2. **Simple API** - Play, stop, volume - no complex mixing needed
3. **Music + SFX** - Support background music and one-shot sound effects
4. **Positional audio** - Basic stereo panning for isometric world

## Audio Interface

```cpp
// engine/audio.h

namespace cafe {

using SoundHandle = uint32_t;
using ChannelHandle = uint32_t;

constexpr SoundHandle INVALID_SOUND = 0;
constexpr ChannelHandle INVALID_CHANNEL = 0;

// Raw audio data loaded from file
struct SoundData {
    std::vector<int16_t> samples;
    int sample_rate;
    int channels;  // 1 = mono, 2 = stereo
};

class Audio {
public:
    virtual ~Audio() = default;

    // Lifecycle
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual void update() = 0;  // Called each frame

    // Sound loading
    virtual SoundHandle load_sound(const SoundData& data) = 0;
    virtual void unload_sound(SoundHandle handle) = 0;

    // Playback
    virtual ChannelHandle play(SoundHandle sound, float volume = 1.0f, bool loop = false) = 0;
    virtual void stop(ChannelHandle channel) = 0;
    virtual void stop_all() = 0;

    // Channel control
    virtual void set_channel_volume(ChannelHandle channel, float volume) = 0;
    virtual void set_channel_pan(ChannelHandle channel, float pan) = 0;  // -1 left, 0 center, 1 right
    virtual bool is_playing(ChannelHandle channel) = 0;

    // Global control
    virtual void set_master_volume(float volume) = 0;
    virtual void pause_all() = 0;
    virtual void resume_all() = 0;
};

// Factory
std::unique_ptr<Audio> create_audio(Platform* platform);

} // namespace cafe
```

## macOS Implementation (CoreAudio)

```cpp
// platform/macos/macos_audio.mm

#import <AudioToolbox/AudioToolbox.h>
#import <AVFoundation/AVFoundation.h>

class MacOSAudio : public Audio {
private:
    static constexpr int MAX_CHANNELS = 32;
    static constexpr int SAMPLE_RATE = 44100;

    AudioQueueRef audio_queue_;
    std::vector<AudioQueueBufferRef> buffers_;

    struct Channel {
        SoundHandle sound = INVALID_SOUND;
        float volume = 1.0f;
        float pan = 0.0f;
        bool loop = false;
        bool playing = false;
        size_t position = 0;
    };

    std::array<Channel, MAX_CHANNELS> channels_;
    std::unordered_map<SoundHandle, SoundData> sounds_;
    SoundHandle next_sound_id_ = 1;

    float master_volume_ = 1.0f;
    bool paused_ = false;
    std::mutex mutex_;

public:
    bool initialize() override {
        // Set up AudioQueue for output
        AudioStreamBasicDescription format = {
            .mSampleRate = SAMPLE_RATE,
            .mFormatID = kAudioFormatLinearPCM,
            .mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked,
            .mBitsPerChannel = 16,
            .mChannelsPerFrame = 2,
            .mBytesPerFrame = 4,
            .mFramesPerPacket = 1,
            .mBytesPerPacket = 4
        };

        OSStatus status = AudioQueueNewOutput(
            &format,
            audio_callback,
            this,
            nullptr,
            nullptr,
            0,
            &audio_queue_
        );

        if (status != noErr) return false;

        // Create and enqueue buffers
        for (int i = 0; i < 3; i++) {
            AudioQueueBufferRef buffer;
            AudioQueueAllocateBuffer(audio_queue_, 4096, &buffer);
            buffer->mAudioDataByteSize = 4096;
            memset(buffer->mAudioData, 0, 4096);
            AudioQueueEnqueueBuffer(audio_queue_, buffer, 0, nullptr);
            buffers_.push_back(buffer);
        }

        AudioQueueStart(audio_queue_, nullptr);
        return true;
    }

    ChannelHandle play(SoundHandle sound, float volume, bool loop) override {
        std::lock_guard<std::mutex> lock(mutex_);

        // Find free channel
        for (ChannelHandle i = 0; i < MAX_CHANNELS; i++) {
            if (!channels_[i].playing) {
                channels_[i] = {sound, volume, 0.0f, loop, true, 0};
                return i + 1;  // 1-indexed handle
            }
        }

        return INVALID_CHANNEL;  // No free channels
    }

private:
    static void audio_callback(void* user_data, AudioQueueRef queue, AudioQueueBufferRef buffer) {
        auto* self = static_cast<MacOSAudio*>(user_data);
        self->fill_buffer(buffer);
        AudioQueueEnqueueBuffer(queue, buffer, 0, nullptr);
    }

    void fill_buffer(AudioQueueBufferRef buffer) {
        std::lock_guard<std::mutex> lock(mutex_);

        int16_t* output = static_cast<int16_t*>(buffer->mAudioData);
        size_t frame_count = buffer->mAudioDataByteSize / 4;  // 4 bytes per frame (stereo 16-bit)

        // Clear buffer
        memset(output, 0, buffer->mAudioDataByteSize);

        if (paused_) return;

        // Mix all playing channels
        for (auto& channel : channels_) {
            if (!channel.playing) continue;

            auto it = sounds_.find(channel.sound);
            if (it == sounds_.end()) {
                channel.playing = false;
                continue;
            }

            const SoundData& sound = it->second;
            mix_channel(output, frame_count, channel, sound);
        }
    }

    void mix_channel(int16_t* output, size_t frame_count, Channel& channel, const SoundData& sound) {
        float vol_l = channel.volume * master_volume_ * (1.0f - std::max(0.0f, channel.pan));
        float vol_r = channel.volume * master_volume_ * (1.0f + std::min(0.0f, channel.pan));

        for (size_t i = 0; i < frame_count; i++) {
            if (channel.position >= sound.samples.size() / sound.channels) {
                if (channel.loop) {
                    channel.position = 0;
                } else {
                    channel.playing = false;
                    break;
                }
            }

            int16_t sample_l, sample_r;
            if (sound.channels == 2) {
                sample_l = sound.samples[channel.position * 2];
                sample_r = sound.samples[channel.position * 2 + 1];
            } else {
                sample_l = sample_r = sound.samples[channel.position];
            }

            // Mix (with clamping)
            int32_t left = output[i * 2] + static_cast<int32_t>(sample_l * vol_l);
            int32_t right = output[i * 2 + 1] + static_cast<int32_t>(sample_r * vol_r);

            output[i * 2] = static_cast<int16_t>(std::clamp(left, -32768, 32767));
            output[i * 2 + 1] = static_cast<int16_t>(std::clamp(right, -32768, 32767));

            channel.position++;
        }
    }
};
```

## Web Implementation (Web Audio API)

```cpp
// platform/web/web_audio.cpp

class WebAudio : public Audio {
private:
    // Web Audio uses JavaScript interop
    // Audio context and nodes managed via Emscripten

public:
    bool initialize() override {
        // Create AudioContext via JS
        EM_ASM({
            window.cafeAudio = {
                ctx: new (window.AudioContext || window.webkitAudioContext)(),
                sounds: {},
                channels: {}
            };
        });
        return true;
    }

    SoundHandle load_sound(const SoundData& data) override {
        SoundHandle handle = next_sound_id_++;

        // Convert samples to Float32 and create AudioBuffer
        EM_ASM({
            var handle = $0;
            var sampleRate = $1;
            var channels = $2;
            var sampleCount = $3;
            var samplesPtr = $4;

            var ctx = window.cafeAudio.ctx;
            var buffer = ctx.createBuffer(channels, sampleCount / channels, sampleRate);

            for (var ch = 0; ch < channels; ch++) {
                var channelData = buffer.getChannelData(ch);
                for (var i = 0; i < channelData.length; i++) {
                    var idx = (channels == 2) ? i * 2 + ch : i;
                    var sample = HEAP16[(samplesPtr >> 1) + idx];
                    channelData[i] = sample / 32768.0;
                }
            }

            window.cafeAudio.sounds[handle] = buffer;
        }, handle, data.sample_rate, data.channels, data.samples.size(), data.samples.data());

        return handle;
    }

    ChannelHandle play(SoundHandle sound, float volume, bool loop) override {
        ChannelHandle channel = next_channel_id_++;

        EM_ASM({
            var soundHandle = $0;
            var channelHandle = $1;
            var volume = $2;
            var loop = $3;

            var ctx = window.cafeAudio.ctx;
            var buffer = window.cafeAudio.sounds[soundHandle];
            if (!buffer) return;

            var source = ctx.createBufferSource();
            source.buffer = buffer;
            source.loop = loop;

            var gainNode = ctx.createGain();
            gainNode.gain.value = volume;

            source.connect(gainNode);
            gainNode.connect(ctx.destination);

            source.start(0);

            window.cafeAudio.channels[channelHandle] = {
                source: source,
                gain: gainNode
            };
        }, sound, channel, volume, loop ? 1 : 0);

        return channel;
    }

    void stop(ChannelHandle channel) override {
        EM_ASM({
            var ch = window.cafeAudio.channels[$0];
            if (ch) {
                ch.source.stop();
                delete window.cafeAudio.channels[$0];
            }
        }, channel);
    }
};
```

## Sound Effect Helper

```cpp
// engine/sfx.h

namespace cafe {

// High-level sound manager for game use
class SoundManager {
private:
    Audio* audio_;
    ResourceManager* resources_;

    std::unordered_map<std::string, SoundHandle> loaded_sounds_;
    ChannelHandle music_channel_ = INVALID_CHANNEL;

public:
    SoundManager(Audio* audio, ResourceManager* resources)
        : audio_(audio), resources_(resources) {}

    // Preload sounds
    void preload(const std::string& name, const std::string& path) {
        ResourceHandle res = resources_->load(path);
        // Convert to audio format and register
        // loaded_sounds_[name] = audio_->load_sound(data);
    }

    // Play one-shot sound effect
    void play_sfx(const std::string& name, float volume = 1.0f) {
        auto it = loaded_sounds_.find(name);
        if (it != loaded_sounds_.end()) {
            audio_->play(it->second, volume, false);
        }
    }

    // Play positional sound (panned based on world position)
    void play_sfx_at(const std::string& name, float world_x, float camera_x, float volume = 1.0f) {
        auto it = loaded_sounds_.find(name);
        if (it == loaded_sounds_.end()) return;

        // Calculate pan based on position relative to camera
        float pan = std::clamp((world_x - camera_x) / 200.0f, -1.0f, 1.0f);

        ChannelHandle ch = audio_->play(it->second, volume, false);
        if (ch != INVALID_CHANNEL) {
            audio_->set_channel_pan(ch, pan);
        }
    }

    // Music control
    void play_music(const std::string& name, float volume = 0.5f) {
        if (music_channel_ != INVALID_CHANNEL) {
            audio_->stop(music_channel_);
        }

        auto it = loaded_sounds_.find(name);
        if (it != loaded_sounds_.end()) {
            music_channel_ = audio_->play(it->second, volume, true);
        }
    }

    void stop_music() {
        if (music_channel_ != INVALID_CHANNEL) {
            audio_->stop(music_channel_);
            music_channel_ = INVALID_CHANNEL;
        }
    }

    void set_music_volume(float volume) {
        if (music_channel_ != INVALID_CHANNEL) {
            audio_->set_channel_volume(music_channel_, volume);
        }
    }
};

} // namespace cafe
```

## WAV Loading

```cpp
// engine/wav_loader.cpp

namespace cafe {

bool load_wav(const std::vector<uint8_t>& file_data, SoundData* out) {
    if (file_data.size() < 44) return false;  // Minimum WAV header size

    const uint8_t* data = file_data.data();

    // Check RIFF header
    if (memcmp(data, "RIFF", 4) != 0) return false;
    if (memcmp(data + 8, "WAVE", 4) != 0) return false;

    // Find fmt chunk
    size_t pos = 12;
    while (pos < file_data.size() - 8) {
        uint32_t chunk_size = *reinterpret_cast<const uint32_t*>(data + pos + 4);

        if (memcmp(data + pos, "fmt ", 4) == 0) {
            uint16_t audio_format = *reinterpret_cast<const uint16_t*>(data + pos + 8);
            if (audio_format != 1) return false;  // Only PCM supported

            out->channels = *reinterpret_cast<const uint16_t*>(data + pos + 10);
            out->sample_rate = *reinterpret_cast<const uint32_t*>(data + pos + 12);
            uint16_t bits_per_sample = *reinterpret_cast<const uint16_t*>(data + pos + 22);

            if (bits_per_sample != 16) return false;  // Only 16-bit supported
        }
        else if (memcmp(data + pos, "data", 4) == 0) {
            // Found audio data
            const int16_t* samples = reinterpret_cast<const int16_t*>(data + pos + 8);
            size_t sample_count = chunk_size / sizeof(int16_t);

            out->samples.assign(samples, samples + sample_count);
            return true;
        }

        pos += 8 + chunk_size;
        if (chunk_size % 2 == 1) pos++;  // Pad byte
    }

    return false;
}

} // namespace cafe
```
