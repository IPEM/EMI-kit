#ifndef MIDI_DUPLICATE_FILTER_H
#define MIDI_DUPLICATE_FILTER_H

#include <stddef.h>
#include <stdint.h>

// Suppresses repeated Control Change messages.
//
// Some MIDI receivers (like DAWs) do not like to receive the same CC value
// twice in a row. The filter remembers the last value per (status, controller)
// pair, so CC80, CC81 and CC82 are tracked independently.
//
// Only Control Change messages (0xB0-0xBF) are filtered. Note On/Off and other
// messages always pass: repeated identical note-ons (e.g. every tap) are
// meaningful and must never be dropped.
//
// Kept free of Arduino dependencies so it can be unit tested natively
// (pio test -e native).
template <size_t N>
class MidiDuplicateFilter {
public:
    // Returns true when the message is a duplicate and should not be sent.
    // Otherwise records the message and returns false.
    bool isDuplicate(uint8_t status, uint8_t param1, uint8_t param2) {
        if ((status & 0xF0) != 0xB0) return false;

        for (size_t i = 0; i < N; i++) {
            Entry &e = entries[i];
            if (e.used && e.status == status && e.param1 == param1) {
                if (e.param2 == param2) return true;
                e.param2 = param2;
                return false;
            }
        }

        // Not tracked yet: take a free slot, otherwise overwrite round-robin
        size_t slot = N;
        for (size_t i = 0; i < N; i++) {
            if (!entries[i].used) { slot = i; break; }
        }
        if (slot == N) {
            slot = nextEvict;
            nextEvict = (nextEvict + 1) % N;
        }
        entries[slot].used = true;
        entries[slot].status = status;
        entries[slot].param1 = param1;
        entries[slot].param2 = param2;
        return false;
    }

    void reset() {
        for (size_t i = 0; i < N; i++) entries[i].used = false;
        nextEvict = 0;
    }

private:
    struct Entry {
        bool used = false;
        uint8_t status = 0;
        uint8_t param1 = 0;
        uint8_t param2 = 0;
    };
    Entry entries[N];
    size_t nextEvict = 0;
};

#endif // MIDI_DUPLICATE_FILTER_H
