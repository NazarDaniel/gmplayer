#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <span>
#include <vector>
#include <SDL_audio.h> // SDL_AudioDeviceID
#include "common.hpp"
#include "format.hpp"
#include "callback_handler.hpp"

namespace mpris { struct Server; }
namespace io { class File; class MappedFile; }

namespace gmplayer {

struct SDLMutex {
    SDL_AudioDeviceID id;
    SDLMutex() = default;
    SDLMutex(SDL_AudioDeviceID id) : id{id} {}
    void lock()   { SDL_LockAudioDevice(id); }
    void unlock() { SDL_UnlockAudioDevice(id); }
};

struct PlayerOptions {
    int fade_out;
    bool autoplay;
    bool track_repeat;
    bool file_repeat;
    int default_duration;
    double tempo;
    int volume;
};

struct Playlist {
    enum class Type { Track, File };

    std::vector<int> order;
    int current = -1;
    bool repeat;

    void regen();
    void regen(int size);
    void shuffle();
    void clear()         { order.clear(); current = -1; }
    void remove(int i)   { order.erase(order.begin() + i); }

    int move(int i, int pos)
    {
        if (i + pos < 0 || i + pos > order.size() - 1)
            return i;
        std::swap(order[i], order[i+pos]);
        return i+pos;
    }

    std::optional<int> get(int off, int min, int max) const
    {
        return repeat && current != -1                    ? std::optional{current}
             : current + off < max && current + off > min ? std::optional{current + off}
             : std::nullopt;
    }

    std::optional<int> next() const { return get(+1, -1, order.size()); }
    std::optional<int> prev() const { return get(-1, -1, order.size()); }
};

class Player {
    std::unique_ptr<FormatInterface> format;
    std::vector<io::MappedFile> file_cache;
    std::vector<Metadata> track_cache;
    Playlist files;
    Playlist tracks;
    std::unique_ptr<mpris::Server> mpris;

    struct {
        SDL_AudioDeviceID dev_id = 0;
        mutable SDLMutex mutex;
        SDL_AudioSpec spec;
    } audio;

    struct {
        bool autoplay = false;
        int volume = 0;
    } options;

    struct {
        std::array<int, NUM_VOICES> volume = { MAX_VOLUME_VALUE / 2, MAX_VOLUME_VALUE / 2,
                                               MAX_VOLUME_VALUE / 2, MAX_VOLUME_VALUE / 2,
                                               MAX_VOLUME_VALUE / 2, MAX_VOLUME_VALUE / 2,
                                               MAX_VOLUME_VALUE / 2, MAX_VOLUME_VALUE / 2, };
    } effects;

    void audio_callback(std::span<u8> stream);
    Error add_file_internal(std::filesystem::path path);

public:
    Player();
    ~Player();

    Error add_file(std::filesystem::path path);
    std::pair<std::vector<Error>, int> add_files(std::span<std::filesystem::path> path);
    void remove_file(int fileno);
    bool load_file(int fileno);
    bool load_track(int num);
    void load_pair(int file, int track);
    void save_playlist(Playlist::Type which, io::File &to);
    void clear();
    const io::MappedFile & current_file()  const;
    const Metadata       & current_track() const;
    const Metadata       & track_info(int i) const;
    const std::vector<Metadata> file_info(int i) const;
    int get_track_count() const;

    bool is_playing() const;
    void start_or_resume();
    void pause();
    void play_pause();
    void stop();
    void seek(int ms);
    void seek_relative(int off);
    int position();
    int length() const;
    bool is_multi_channel() const;

    void next();
    void prev();
    bool has_next() const;
    bool has_prev() const;
    void shuffle(Playlist::Type which);
    int move(Playlist::Type which, int n, int pos);
    std::vector<std::string> names(Playlist::Type which) const;

    std::vector<std::string> channel_names();
    void mute_channel(int index, bool mute);
    void set_channel_volume(int index, int value);

    mpris::Server &mpris_server();

#define MAKE_SIGNAL(name, ...) \
private:                                            \
    CallbackHandler<void(__VA_ARGS__)> name;        \
public:                                             \
    void on_##name(auto &&fn) { name.add(fn); }     \

    MAKE_SIGNAL(file_changed, int)
    MAKE_SIGNAL(track_changed, int, const Metadata &)
    MAKE_SIGNAL(position_changed, int)
    MAKE_SIGNAL(track_ended, void)
    MAKE_SIGNAL(paused, void)
    MAKE_SIGNAL(played, void)
    MAKE_SIGNAL(seeked, void)
    MAKE_SIGNAL(volume_changed, int)
    MAKE_SIGNAL(tempo_changed, double)
    MAKE_SIGNAL(fade_changed, int);
    MAKE_SIGNAL(repeat_changed, bool, bool)
    MAKE_SIGNAL(shuffled, Playlist::Type)
    MAKE_SIGNAL(error, Error)
    MAKE_SIGNAL(cleared, void)
    MAKE_SIGNAL(playlist_changed, Playlist::Type)
    MAKE_SIGNAL(file_removed, int)
    MAKE_SIGNAL(samples_played, std::span<i16>, std::span<f32>)
    MAKE_SIGNAL(channel_volume_changed, int, int)

#undef MAKE_SIGNAL
};

inline bool is_playlist(std::filesystem::path filename) { return filename.extension() == ".playlist"; }

} // namespace gmplayer
