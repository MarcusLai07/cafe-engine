#ifdef __EMSCRIPTEN__

#include "../audio.h"
#include <emscripten.h>
#include <unordered_map>
#include <iostream>

namespace cafe {

// ============================================================================
// Web Audio System using Web Audio API via Emscripten
// ============================================================================

// JavaScript functions will be called via EM_JS or EM_ASM

class WebAudioSystem : public AudioSystem {
private:
    std::unordered_map<SoundHandle, std::string> sound_paths_;
    SoundHandle next_sound_id_ = 1;
    ChannelHandle next_channel_id_ = 1;

    float master_volume_ = 1.0f;
    float sound_volume_ = 1.0f;
    float music_volume_ = 1.0f;
    bool muted_ = false;
    bool music_playing_ = false;
    bool music_paused_ = false;

public:
    bool initialize() override {
        // Initialize Web Audio API context
        EM_ASM({
            if (!window.cafeAudio) {
                window.cafeAudio = {
                    context: null,
                    sounds: {},
                    channels: {},
                    music: null,
                    musicGain: null,
                    masterGain: null,

                    init: function() {
                        this.context = new (window.AudioContext || window.webkitAudioContext)();
                        this.masterGain = this.context.createGain();
                        this.masterGain.connect(this.context.destination);
                        this.musicGain = this.context.createGain();
                        this.musicGain.connect(this.masterGain);
                        console.log('Web Audio initialized');
                    },

                    loadSound: function(id, path) {
                        var self = this;
                        fetch(path)
                            .then(response => response.arrayBuffer())
                            .then(buffer => self.context.decodeAudioData(buffer))
                            .then(audioBuffer => {
                                self.sounds[id] = audioBuffer;
                                console.log('Loaded sound:', path);
                            })
                            .catch(err => console.error('Failed to load sound:', path, err));
                    },

                    playSound: function(id, volume) {
                        var buffer = this.sounds[id];
                        if (!buffer) return 0;

                        var source = this.context.createBufferSource();
                        source.buffer = buffer;

                        var gain = this.context.createGain();
                        gain.gain.value = volume;
                        source.connect(gain);
                        gain.connect(this.masterGain);

                        source.start();
                        return 1;
                    },

                    playMusic: function(path, loop) {
                        var self = this;
                        if (this.music) {
                            this.music.pause();
                        }
                        this.music = new Audio(path);
                        this.music.loop = loop;
                        this.music.volume = this.musicGain.gain.value;
                        this.music.play().catch(err => console.error('Music play failed:', err));
                    },

                    stopMusic: function() {
                        if (this.music) {
                            this.music.pause();
                            this.music.currentTime = 0;
                        }
                    },

                    setMasterVolume: function(vol) {
                        if (this.masterGain) {
                            this.masterGain.gain.value = vol;
                        }
                    },

                    setMusicVolume: function(vol) {
                        if (this.music) {
                            this.music.volume = vol;
                        }
                    }
                };
                window.cafeAudio.init();
            }
        });

        std::cout << "Web Audio system initialized\n";
        return true;
    }

    void shutdown() override {
        EM_ASM({
            if (window.cafeAudio) {
                if (window.cafeAudio.music) {
                    window.cafeAudio.music.pause();
                }
                if (window.cafeAudio.context) {
                    window.cafeAudio.context.close();
                }
                window.cafeAudio = null;
            }
        });
    }

    SoundHandle load_sound(const std::string& path) override {
        SoundHandle handle = next_sound_id_++;
        sound_paths_[handle] = path;

        EM_ASM_({
            if (window.cafeAudio) {
                window.cafeAudio.loadSound($0, UTF8ToString($1));
            }
        }, handle, path.c_str());

        return handle;
    }

    void unload_sound(SoundHandle sound) override {
        sound_paths_.erase(sound);

        EM_ASM_({
            if (window.cafeAudio && window.cafeAudio.sounds[$0]) {
                delete window.cafeAudio.sounds[$0];
            }
        }, sound);
    }

    bool is_sound_loaded(SoundHandle sound) const override {
        return sound_paths_.find(sound) != sound_paths_.end();
    }

    ChannelHandle play_sound(SoundHandle sound, float volume) override {
        PlayOptions options;
        options.volume = volume;
        return play_sound(sound, options);
    }

    ChannelHandle play_sound(SoundHandle sound, const PlayOptions& options) override {
        float vol = options.volume * sound_volume_ * master_volume_;
        if (muted_) vol = 0.0f;

        EM_ASM_({
            if (window.cafeAudio) {
                window.cafeAudio.playSound($0, $1);
            }
        }, sound, vol);

        return next_channel_id_++;
    }

    bool play_music(const std::string& path, bool loop) override {
        float vol = music_volume_ * master_volume_;
        if (muted_) vol = 0.0f;

        EM_ASM_({
            if (window.cafeAudio) {
                window.cafeAudio.playMusic(UTF8ToString($0), $1);
                window.cafeAudio.setMusicVolume($2);
            }
        }, path.c_str(), loop, vol);

        music_playing_ = true;
        music_paused_ = false;
        return true;
    }

    void stop_music() override {
        EM_ASM({
            if (window.cafeAudio) {
                window.cafeAudio.stopMusic();
            }
        });
        music_playing_ = false;
        music_paused_ = false;
    }

    void pause_music() override {
        EM_ASM({
            if (window.cafeAudio && window.cafeAudio.music) {
                window.cafeAudio.music.pause();
            }
        });
        music_paused_ = true;
    }

    void resume_music() override {
        EM_ASM({
            if (window.cafeAudio && window.cafeAudio.music) {
                window.cafeAudio.music.play();
            }
        });
        music_paused_ = false;
    }

    bool is_music_playing() const override { return music_playing_ && !music_paused_; }
    bool is_music_paused() const override { return music_paused_; }

    void set_music_volume(float volume) override {
        music_volume_ = std::max(0.0f, std::min(1.0f, volume));
        update_volumes();
    }

    float music_volume() const override { return music_volume_; }

    void stop_channel(ChannelHandle) override {
        // Web Audio sources can't be easily stopped by channel
    }

    void stop_all_sounds() override {
        // Would need to track all sources
    }

    bool is_channel_playing(ChannelHandle) const override { return false; }
    void set_channel_volume(ChannelHandle, float) override {}

    void set_master_volume(float volume) override {
        master_volume_ = std::max(0.0f, std::min(1.0f, volume));
        update_volumes();
    }

    float master_volume() const override { return master_volume_; }

    void set_sound_volume(float volume) override {
        sound_volume_ = std::max(0.0f, std::min(1.0f, volume));
        update_volumes();
    }

    float sound_volume() const override { return sound_volume_; }

    void set_muted(bool muted) override {
        muted_ = muted;
        update_volumes();
    }

    bool is_muted() const override { return muted_; }

    void update() override {
        // Nothing to do each frame for web audio
    }

private:
    void update_volumes() {
        float master = muted_ ? 0.0f : master_volume_;
        float music = master * music_volume_;

        EM_ASM_({
            if (window.cafeAudio) {
                window.cafeAudio.setMasterVolume($0);
                window.cafeAudio.setMusicVolume($1);
            }
        }, master, music);
    }
};

std::unique_ptr<AudioSystem> create_audio_system() {
    return std::make_unique<WebAudioSystem>();
}

} // namespace cafe

#endif // __EMSCRIPTEN__
