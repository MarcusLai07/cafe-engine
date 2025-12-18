#ifndef CAFE_AUDIO_H
#define CAFE_AUDIO_H

#include <string>
#include <memory>
#include <cstdint>

namespace cafe {

// ============================================================================
// Audio Handle - Reference to a loaded sound
// ============================================================================

using SoundHandle = uint32_t;
constexpr SoundHandle INVALID_SOUND = 0;

// ============================================================================
// Audio Channel - A playing sound instance
// ============================================================================

using ChannelHandle = uint32_t;
constexpr ChannelHandle INVALID_CHANNEL = 0;

// ============================================================================
// Audio System Interface
// ============================================================================

class AudioSystem {
public:
    virtual ~AudioSystem() = default;

    // Lifecycle
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;

    // ========================================================================
    // Sound Loading
    // ========================================================================

    // Load sound from file (WAV, MP3, OGG depending on platform)
    virtual SoundHandle load_sound(const std::string& path) = 0;

    // Unload a sound
    virtual void unload_sound(SoundHandle sound) = 0;

    // Check if sound is loaded
    virtual bool is_sound_loaded(SoundHandle sound) const = 0;

    // ========================================================================
    // Playback
    // ========================================================================

    // Play sound effect (fire and forget)
    virtual ChannelHandle play_sound(SoundHandle sound, float volume = 1.0f) = 0;

    // Play sound with options
    struct PlayOptions {
        float volume = 1.0f;
        float pitch = 1.0f;   // 1.0 = normal, 0.5 = half speed, 2.0 = double
        float pan = 0.0f;     // -1.0 = left, 0.0 = center, 1.0 = right
        bool loop = false;
    };
    virtual ChannelHandle play_sound(SoundHandle sound, const PlayOptions& options) = 0;

    // ========================================================================
    // Music (streaming, only one at a time)
    // ========================================================================

    // Play music file (streams from disk)
    virtual bool play_music(const std::string& path, bool loop = true) = 0;

    // Stop music
    virtual void stop_music() = 0;

    // Pause/resume music
    virtual void pause_music() = 0;
    virtual void resume_music() = 0;

    // Music state
    virtual bool is_music_playing() const = 0;
    virtual bool is_music_paused() const = 0;

    // Music volume (0.0 to 1.0)
    virtual void set_music_volume(float volume) = 0;
    virtual float music_volume() const = 0;

    // ========================================================================
    // Channel Control
    // ========================================================================

    // Stop a playing sound
    virtual void stop_channel(ChannelHandle channel) = 0;

    // Stop all sounds
    virtual void stop_all_sounds() = 0;

    // Check if channel is still playing
    virtual bool is_channel_playing(ChannelHandle channel) const = 0;

    // Set channel volume
    virtual void set_channel_volume(ChannelHandle channel, float volume) = 0;

    // ========================================================================
    // Global Settings
    // ========================================================================

    // Master volume (affects everything)
    virtual void set_master_volume(float volume) = 0;
    virtual float master_volume() const = 0;

    // Sound effect volume (affects all sound effects)
    virtual void set_sound_volume(float volume) = 0;
    virtual float sound_volume() const = 0;

    // Mute/unmute
    virtual void set_muted(bool muted) = 0;
    virtual bool is_muted() const = 0;

    // ========================================================================
    // Update (call each frame)
    // ========================================================================

    virtual void update() = 0;
};

// Factory function - creates platform-appropriate audio system
std::unique_ptr<AudioSystem> create_audio_system();

} // namespace cafe

#endif // CAFE_AUDIO_H
