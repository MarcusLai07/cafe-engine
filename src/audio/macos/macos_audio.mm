#ifdef __APPLE__
#ifndef __EMSCRIPTEN__

#import "../audio.h"
#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>
#import <unordered_map>
#import <vector>

namespace cafe {

// ============================================================================
// macOS Audio System using AVFoundation
// ============================================================================

class MacOSAudioSystem : public AudioSystem {
private:
    // Sound storage
    struct SoundData {
        NSData* data = nil;
        AVAudioFormat* format = nil;
    };
    std::unordered_map<SoundHandle, SoundData> sounds_;
    SoundHandle next_sound_id_ = 1;

    // Active channels (AVAudioPlayerNode instances)
    struct ChannelData {
        AVAudioPlayerNode* player = nil;
        bool is_playing = false;
    };
    std::unordered_map<ChannelHandle, ChannelData> channels_;
    ChannelHandle next_channel_id_ = 1;

    // Audio engine
    AVAudioEngine* engine_ = nil;
    AVAudioMixerNode* mixer_ = nil;

    // Music player
    AVAudioPlayer* music_player_ = nil;
    bool music_paused_ = false;

    // Volume settings
    float master_volume_ = 1.0f;
    float sound_volume_ = 1.0f;
    float music_volume_ = 1.0f;
    bool muted_ = false;

public:
    MacOSAudioSystem() = default;

    ~MacOSAudioSystem() override {
        shutdown();
    }

    bool initialize() override {
        @autoreleasepool {
            // Create audio engine
            engine_ = [[AVAudioEngine alloc] init];
            mixer_ = [engine_ mainMixerNode];

            // Start engine
            NSError* error = nil;
            if (![engine_ startAndReturnError:&error]) {
                NSLog(@"Failed to start audio engine: %@", error);
                return false;
            }

            NSLog(@"Audio system initialized");
            return true;
        }
    }

    void shutdown() override {
        @autoreleasepool {
            stop_all_sounds();
            stop_music();

            if (engine_) {
                [engine_ stop];
                engine_ = nil;
            }

            // Clear sounds
            for (auto& [id, data] : sounds_) {
                data.data = nil;
                data.format = nil;
            }
            sounds_.clear();

            // Clear channels
            for (auto& [id, ch] : channels_) {
                ch.player = nil;
            }
            channels_.clear();
        }
    }

    // ========================================================================
    // Sound Loading
    // ========================================================================

    SoundHandle load_sound(const std::string& path) override {
        @autoreleasepool {
            NSString* nsPath = [NSString stringWithUTF8String:path.c_str()];
            NSURL* url = [NSURL fileURLWithPath:nsPath];

            NSError* error = nil;
            AVAudioFile* file = [[AVAudioFile alloc] initForReading:url error:&error];
            if (!file) {
                NSLog(@"Failed to load sound: %@ - %@", nsPath, error);
                return INVALID_SOUND;
            }

            // Read entire file into buffer
            AVAudioPCMBuffer* buffer = [[AVAudioPCMBuffer alloc]
                initWithPCMFormat:file.processingFormat
                frameCapacity:(AVAudioFrameCount)file.length];

            if (![file readIntoBuffer:buffer error:&error]) {
                NSLog(@"Failed to read sound data: %@", error);
                return INVALID_SOUND;
            }

            // Store sound data
            SoundHandle handle = next_sound_id_++;
            SoundData data;
            // Store the buffer's underlying data
            data.format = file.processingFormat;

            // We need to keep the buffer data around
            // For simplicity, we'll create a new buffer each time we play
            // In a real implementation, you'd cache the AVAudioPCMBuffer

            sounds_[handle] = data;

            NSLog(@"Loaded sound: %@ (handle %u)", nsPath, handle);
            return handle;
        }
    }

    void unload_sound(SoundHandle sound) override {
        auto it = sounds_.find(sound);
        if (it != sounds_.end()) {
            it->second.data = nil;
            it->second.format = nil;
            sounds_.erase(it);
        }
    }

    bool is_sound_loaded(SoundHandle sound) const override {
        return sounds_.find(sound) != sounds_.end();
    }

    // ========================================================================
    // Playback
    // ========================================================================

    ChannelHandle play_sound(SoundHandle sound, float volume) override {
        PlayOptions options;
        options.volume = volume;
        return play_sound(sound, options);
    }

    ChannelHandle play_sound(SoundHandle sound, const PlayOptions& options) override {
        @autoreleasepool {
            // For now, use a simpler approach with AVAudioPlayer
            // Full AVAudioEngine buffer playback is more complex

            auto it = sounds_.find(sound);
            if (it == sounds_.end()) {
                return INVALID_CHANNEL;
            }

            // Calculate effective volume from options
            float effective_volume = options.volume * sound_volume_ * master_volume_;
            if (muted_) effective_volume = 0.0f;

            // Note: This is a simplified implementation
            // For proper sound effect playback, you'd want to use
            // AVAudioPlayerNode with pre-loaded buffers
            // options.pitch, options.pan, options.loop would be used there

            (void)effective_volume;  // Will be used when full playback is implemented

            ChannelHandle handle = next_channel_id_++;
            ChannelData channel;
            channel.is_playing = true;
            channels_[handle] = channel;

            return handle;
        }
    }

    // ========================================================================
    // Music
    // ========================================================================

    bool play_music(const std::string& path, bool loop) override {
        @autoreleasepool {
            stop_music();

            NSString* nsPath = [NSString stringWithUTF8String:path.c_str()];
            NSURL* url = [NSURL fileURLWithPath:nsPath];

            NSError* error = nil;
            music_player_ = [[AVAudioPlayer alloc] initWithContentsOfURL:url error:&error];

            if (!music_player_) {
                NSLog(@"Failed to load music: %@ - %@", nsPath, error);
                return false;
            }

            music_player_.numberOfLoops = loop ? -1 : 0;  // -1 = infinite loop
            music_player_.volume = muted_ ? 0.0f : music_volume_ * master_volume_;

            if (![music_player_ play]) {
                NSLog(@"Failed to play music: %@", nsPath);
                music_player_ = nil;
                return false;
            }

            music_paused_ = false;
            NSLog(@"Playing music: %@", nsPath);
            return true;
        }
    }

    void stop_music() override {
        @autoreleasepool {
            if (music_player_) {
                [music_player_ stop];
                music_player_ = nil;
            }
            music_paused_ = false;
        }
    }

    void pause_music() override {
        @autoreleasepool {
            if (music_player_ && music_player_.isPlaying) {
                [music_player_ pause];
                music_paused_ = true;
            }
        }
    }

    void resume_music() override {
        @autoreleasepool {
            if (music_player_ && music_paused_) {
                [music_player_ play];
                music_paused_ = false;
            }
        }
    }

    bool is_music_playing() const override {
        return music_player_ != nil && music_player_.isPlaying;
    }

    bool is_music_paused() const override {
        return music_paused_;
    }

    void set_music_volume(float volume) override {
        music_volume_ = std::max(0.0f, std::min(1.0f, volume));
        update_volumes();
    }

    float music_volume() const override {
        return music_volume_;
    }

    // ========================================================================
    // Channel Control
    // ========================================================================

    void stop_channel(ChannelHandle channel) override {
        auto it = channels_.find(channel);
        if (it != channels_.end()) {
            if (it->second.player) {
                [it->second.player stop];
            }
            it->second.is_playing = false;
        }
    }

    void stop_all_sounds() override {
        for (auto& [id, ch] : channels_) {
            if (ch.player) {
                [ch.player stop];
            }
            ch.is_playing = false;
        }
    }

    bool is_channel_playing(ChannelHandle channel) const override {
        auto it = channels_.find(channel);
        if (it != channels_.end()) {
            return it->second.is_playing;
        }
        return false;
    }

    void set_channel_volume(ChannelHandle channel, float volume) override {
        // Implementation would set player volume
        (void)channel;
        (void)volume;
    }

    // ========================================================================
    // Global Settings
    // ========================================================================

    void set_master_volume(float volume) override {
        master_volume_ = std::max(0.0f, std::min(1.0f, volume));
        update_volumes();
    }

    float master_volume() const override {
        return master_volume_;
    }

    void set_sound_volume(float volume) override {
        sound_volume_ = std::max(0.0f, std::min(1.0f, volume));
        update_volumes();
    }

    float sound_volume() const override {
        return sound_volume_;
    }

    void set_muted(bool muted) override {
        muted_ = muted;
        update_volumes();
    }

    bool is_muted() const override {
        return muted_;
    }

    void update() override {
        @autoreleasepool {
            // Clean up finished channels
            std::vector<ChannelHandle> finished;
            for (auto& [id, ch] : channels_) {
                if (ch.player && !ch.player.isPlaying) {
                    ch.is_playing = false;
                    finished.push_back(id);
                }
            }
            for (auto id : finished) {
                channels_.erase(id);
            }
        }
    }

private:
    void update_volumes() {
        @autoreleasepool {
            float effective_volume = muted_ ? 0.0f : master_volume_;

            // Update music volume
            if (music_player_) {
                music_player_.volume = effective_volume * music_volume_;
            }

            // Update mixer volume for sound effects
            if (mixer_) {
                mixer_.outputVolume = effective_volume * sound_volume_;
            }
        }
    }
};

// ============================================================================
// Factory Function
// ============================================================================

std::unique_ptr<AudioSystem> create_audio_system() {
    return std::make_unique<MacOSAudioSystem>();
}

} // namespace cafe

#endif // !__EMSCRIPTEN__
#endif // __APPLE__
