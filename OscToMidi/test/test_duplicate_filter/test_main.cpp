#include <unity.h>
#include "MidiDuplicateFilter.h"

static const uint8_t NOTE_ON = 0x90;
static const uint8_t NOTE_OFF = 0x80;
static const uint8_t CC = 0xB0;

void setUp() {}
void tearDown() {}

void test_repeated_taps_are_never_filtered() {
    MidiDuplicateFilter<16> f;
    for (int tap = 0; tap < 5; tap++) {
        TEST_ASSERT_FALSE(f.isDuplicate(NOTE_ON, 60, 100));
        TEST_ASSERT_FALSE(f.isDuplicate(NOTE_OFF, 60, 0));
    }
}

void test_identical_cc_is_filtered() {
    MidiDuplicateFilter<16> f;
    TEST_ASSERT_FALSE(f.isDuplicate(CC, 80, 64));
    TEST_ASSERT_TRUE(f.isDuplicate(CC, 80, 64));
    TEST_ASSERT_FALSE(f.isDuplicate(CC, 80, 65));
    TEST_ASSERT_TRUE(f.isDuplicate(CC, 80, 65));
}

void test_interleaved_controllers_are_tracked_independently() {
    MidiDuplicateFilter<16> f;
    // first frame: everything new
    TEST_ASSERT_FALSE(f.isDuplicate(CC, 80, 10));
    TEST_ASSERT_FALSE(f.isDuplicate(CC, 81, 20));
    TEST_ASSERT_FALSE(f.isDuplicate(CC, 82, 30));
    // second frame, unchanged: all three filtered
    TEST_ASSERT_TRUE(f.isDuplicate(CC, 80, 10));
    TEST_ASSERT_TRUE(f.isDuplicate(CC, 81, 20));
    TEST_ASSERT_TRUE(f.isDuplicate(CC, 82, 30));
    // third frame: only 81 changed
    TEST_ASSERT_TRUE(f.isDuplicate(CC, 80, 10));
    TEST_ASSERT_FALSE(f.isDuplicate(CC, 81, 21));
    TEST_ASSERT_TRUE(f.isDuplicate(CC, 82, 30));
}

void test_channels_are_tracked_independently() {
    MidiDuplicateFilter<16> f;
    TEST_ASSERT_FALSE(f.isDuplicate(CC | 0, 80, 10));
    TEST_ASSERT_FALSE(f.isDuplicate(CC | 1, 80, 10));
    TEST_ASSERT_TRUE(f.isDuplicate(CC | 0, 80, 10));
    TEST_ASSERT_TRUE(f.isDuplicate(CC | 1, 80, 10));
}

void test_eviction_when_full() {
    MidiDuplicateFilter<2> f;
    TEST_ASSERT_FALSE(f.isDuplicate(CC, 1, 1));
    TEST_ASSERT_FALSE(f.isDuplicate(CC, 2, 2));
    TEST_ASSERT_FALSE(f.isDuplicate(CC, 3, 3)); // evicts CC1
    TEST_ASSERT_TRUE(f.isDuplicate(CC, 3, 3));
    TEST_ASSERT_FALSE(f.isDuplicate(CC, 1, 1)); // forgotten, sent again
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_repeated_taps_are_never_filtered);
    RUN_TEST(test_identical_cc_is_filtered);
    RUN_TEST(test_interleaved_controllers_are_tracked_independently);
    RUN_TEST(test_channels_are_tracked_independently);
    RUN_TEST(test_eviction_when_full);
    return UNITY_END();
}
