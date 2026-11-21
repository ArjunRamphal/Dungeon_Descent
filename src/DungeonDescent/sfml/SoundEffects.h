#pragma once

#include <SFML/Audio.hpp>
#include <array>
#include <cmath>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <vector>

namespace dungeon {

enum class SfxId : std::size_t {
    UiClick = 0,
    Strike,
    CritHit,
    GuardHit,
    TakeDamage,
    ChestUnlock,
    RiddleCorrect,
    RiddleWrong,
    TimerTick,
    Count
};

// Procedural 16-bit mono PCM synthesizer using SFML SoundBuffer + Sound.
// Generates authentic, crisp waveform audio buffers at construction time
// (without requiring external WAV/OGG files) and supports --no-audio and mute.
class SoundEffects {
public:
    explicit SoundEffects(bool audioEnabled) : audioEnabled_(audioEnabled) {
        if (audioEnabled_) {
            synthesizeAll();
        }
    }

    void play(SfxId id, bool muted) {
        lastTriggered_ = id;
        ++triggerCounts_[static_cast<std::size_t>(id)];
        if (!audioEnabled_ || muted || !initialized_) return;
        const auto index = static_cast<std::size_t>(id);
        if (index >= buffers_.size() || buffers_[index].getSampleCount() == 0) return;

        // Clean up finished sounds while keeping a bounded pool for overlapping triggers.
        while (!activeSounds_.empty() && activeSounds_.front().getStatus() == sf::Sound::Status::Stopped) {
            activeSounds_.pop_front();
        }
        if (activeSounds_.size() >= 12) {
            activeSounds_.front().stop();
            activeSounds_.pop_front();
        }
        activeSounds_.emplace_back(buffers_[index]);
        auto& sound = activeSounds_.back();
        sound.setVolume(volumeFor(id));
        sound.play();
        ++playbackCount_;
    }

    void setMuted(bool muted) {
        if (!muted) return;
        for (auto& sound : activeSounds_) {
            sound.stop();
        }
        activeSounds_.clear();
    }

    void stopAll() {
        for (auto& sound : activeSounds_) {
            sound.stop();
        }
        activeSounds_.clear();
    }

    bool audioEnabled() const { return audioEnabled_; }
    bool initialized() const { return initialized_; }
    SfxId lastTriggered() const { return lastTriggered_; }
    std::uint64_t triggerCount(SfxId id) const { return triggerCounts_[static_cast<std::size_t>(id)]; }
    std::uint64_t totalTriggers() const {
        std::uint64_t total = 0;
        for (const auto c : triggerCounts_) total += c;
        return total;
    }
    std::uint64_t playbackCount() const { return playbackCount_; }
    const sf::SoundBuffer& buffer(SfxId id) const { return buffers_[static_cast<std::size_t>(id)]; }

private:
    static constexpr unsigned SampleRate = 44100;
    static constexpr float TwoPi = 6.2831853071795864769f;

    static float volumeFor(SfxId id) {
        switch (id) {
        case SfxId::UiClick: return 38.f;
        case SfxId::TimerTick: return 34.f;
        case SfxId::CritHit: return 65.f;
        case SfxId::TakeDamage: return 55.f;
        default: return 50.f;
        }
    }

    static std::int16_t clampSample(float v) {
        if (v > 1.f) v = 1.f;
        if (v < -1.f) v = -1.f;
        return static_cast<std::int16_t>(v * 32000.f);
    }

    static std::vector<std::int16_t> buildTone(float durationSec, const std::function<float(float, float)>& generator) {
        const std::size_t count = static_cast<std::size_t>(durationSec * SampleRate);
        std::vector<std::int16_t> samples(count);
        for (std::size_t i = 0; i < count; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(SampleRate);
            const float norm = static_cast<float>(i) / static_cast<float>(count);
            samples[i] = clampSample(generator(t, norm));
        }
        return samples;
    }

    void synthesizeAll() {
        const std::vector<sf::SoundChannel> mono{sf::SoundChannel::Mono};

        // 1. UiClick: crisp wooden/parchment tick (45ms)
        {
            auto s = buildTone(0.045f, [](float t, float n) {
                const float env = std::exp(-n * 8.f);
                const float freq = 920.f - 280.f * n;
                return env * (0.65f * std::sin(TwoPi * freq * t) + 0.35f * std::sin(TwoPi * (freq * 2.1f) * t)) * 0.45f;
            });
            (void)buffers_[static_cast<std::size_t>(SfxId::UiClick)].loadFromSamples(s.data(), s.size(), 1, SampleRate, mono);
        }

        // 2. Strike: metallic slash / downward sweep + subtle grit (160ms)
        {
            std::uint32_t rng = 0x13579bdfu;
            auto s = buildTone(0.16f, [&rng](float t, float n) {
                rng = rng * 1664525u + 1013904223u;
                const float noise = (static_cast<float>(rng & 0xffffu) / 32768.f) - 1.f;
                const float env = std::exp(-n * 5.2f);
                const float freq = 540.f * std::exp(-n * 1.4f);
                const float blade = std::sin(TwoPi * freq * t) + 0.4f * std::sin(TwoPi * freq * 1.5f * t);
                return env * (0.55f * blade + 0.30f * noise * std::exp(-n * 7.f)) * 0.6f;
            });
            (void)buffers_[static_cast<std::size_t>(SfxId::Strike)].loadFromSamples(s.data(), s.size(), 1, SampleRate, mono);
        }

        // 3. CritHit: resonant heavy impact + bright high harmonic ring (260ms)
        {
            std::uint32_t rng = 0x2468ace1u;
            auto s = buildTone(0.26f, [&rng](float t, float n) {
                rng = rng * 1664525u + 1013904223u;
                const float noise = (static_cast<float>(rng & 0xffffu) / 32768.f) - 1.f;
                const float punchEnv = std::exp(-n * 6.5f);
                const float ringEnv = std::exp(-n * 3.5f);
                const float thump = std::sin(TwoPi * (220.f * std::exp(-n * 1.6f)) * t);
                const float chime = 0.6f * std::sin(TwoPi * 1046.5f * t) + 0.4f * std::sin(TwoPi * 1567.98f * t);
                return (0.55f * punchEnv * (thump + 0.45f * noise) + 0.45f * ringEnv * chime) * 0.7f;
            });
            (void)buffers_[static_cast<std::size_t>(SfxId::CritHit)].loadFromSamples(s.data(), s.size(), 1, SampleRate, mono);
        }

        // 4. GuardHit: metallic shield parry clang (180ms)
        {
            auto s = buildTone(0.18f, [](float t, float n) {
                const float env = std::exp(-n * 4.5f);
                const float metal = std::sin(TwoPi * 680.f * t) + 0.7f * std::sin(TwoPi * 1025.f * t) +
                                    0.4f * std::sin(TwoPi * 1490.f * t);
                return env * metal * 0.35f;
            });
            (void)buffers_[static_cast<std::size_t>(SfxId::GuardHit)].loadFromSamples(s.data(), s.size(), 1, SampleRate, mono);
        }

        // 5. TakeDamage: low heavy thud + dull crunch (190ms)
        {
            std::uint32_t rng = 0x9e3779b9u;
            auto s = buildTone(0.19f, [&rng](float t, float n) {
                rng = rng * 1664525u + 1013904223u;
                const float noise = (static_cast<float>(rng & 0xffffu) / 32768.f) - 1.f;
                const float env = std::exp(-n * 5.0f);
                const float low = std::sin(TwoPi * (145.f - 75.f * n) * t);
                return env * (0.7f * low + 0.28f * noise * std::exp(-n * 6.f)) * 0.65f;
            });
            (void)buffers_[static_cast<std::size_t>(SfxId::TakeDamage)].loadFromSamples(s.data(), s.size(), 1, SampleRate, mono);
        }

        // 6. ChestUnlock: ascending metallic latch + shimmering chord (300ms)
        {
            auto s = buildTone(0.30f, [](float t, float n) {
                if (n < 0.22f) {
                    const float ln = n / 0.22f;
                    const float env = std::sin(3.14159f * ln);
                    return env * std::sin(TwoPi * 440.f * t) * 0.45f;
                }
                const float cn = (n - 0.22f) / 0.78f;
                const float env = std::exp(-cn * 3.8f);
                const float chord = std::sin(TwoPi * 587.33f * t) + 0.75f * std::sin(TwoPi * 880.f * t) +
                                    0.5f * std::sin(TwoPi * 1174.66f * t);
                return env * chord * 0.3f;
            });
            (void)buffers_[static_cast<std::size_t>(SfxId::ChestUnlock)].loadFromSamples(s.data(), s.size(), 1, SampleRate, mono);
        }

        // 7. RiddleCorrect: warm ascending major arpeggio chime (320ms)
        {
            auto s = buildTone(0.32f, [](float t, float n) {
                const float f1 = 523.25f, f2 = 659.25f, f3 = 783.99f; // C5 E5 G5
                float val = 0.f;
                if (n < 0.33f) {
                    const float ln = n / 0.33f;
                    val = std::exp(-ln * 2.5f) * std::sin(TwoPi * f1 * t);
                } else if (n < 0.66f) {
                    const float ln = (n - 0.33f) / 0.33f;
                    val = std::exp(-ln * 2.5f) * std::sin(TwoPi * f2 * t);
                } else {
                    const float ln = (n - 0.66f) / 0.34f;
                    val = std::exp(-ln * 3.2f) * (std::sin(TwoPi * f3 * t) + 0.35f * std::sin(TwoPi * f1 * 2.f * t));
                }
                return val * 0.55f;
            });
            (void)buffers_[static_cast<std::size_t>(SfxId::RiddleCorrect)].loadFromSamples(s.data(), s.size(), 1, SampleRate, mono);
        }

        // 8. RiddleWrong: descending dissonant buzzer/chime (280ms)
        {
            auto s = buildTone(0.28f, [](float t, float n) {
                const float env = std::exp(-n * 3.2f);
                const float freq = n < 0.45f ? 260.f : 195.f;
                const float buzz = std::sin(TwoPi * freq * t) + 0.5f * std::sin(TwoPi * (freq * 1.414f) * t);
                return env * buzz * 0.4f;
            });
            (void)buffers_[static_cast<std::size_t>(SfxId::RiddleWrong)].loadFromSamples(s.data(), s.size(), 1, SampleRate, mono);
        }

        // 9. TimerTick: crisp clock/hourglass pulse (55ms)
        {
            auto s = buildTone(0.055f, [](float t, float n) {
                const float env = std::exp(-n * 9.f);
                return env * (std::sin(TwoPi * 1120.f * t) + 0.4f * std::sin(TwoPi * 2240.f * t)) * 0.4f;
            });
            (void)buffers_[static_cast<std::size_t>(SfxId::TimerTick)].loadFromSamples(s.data(), s.size(), 1, SampleRate, mono);
        }

        initialized_ = true;
    }

    bool audioEnabled_ = false;
    bool initialized_ = false;
    std::array<sf::SoundBuffer, static_cast<std::size_t>(SfxId::Count)> buffers_{};
    std::deque<sf::Sound> activeSounds_;
    SfxId lastTriggered_ = SfxId::UiClick;
    std::array<std::uint64_t, static_cast<std::size_t>(SfxId::Count)> triggerCounts_{};
    std::uint64_t playbackCount_ = 0;
};

} // namespace dungeon
