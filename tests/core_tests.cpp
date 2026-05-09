#include <gtest/gtest.h>
#include "../src/emulator_core.hpp"
#include <fstream>

class EmulatorCoreTest : public ::testing::Test {
protected:
    EmulatorCore core;
};

TEST_F(EmulatorCoreTest, InitialStateIsClean) {
    EXPECT_FALSE(core.is_ready());
    EXPECT_EQ(core.get_av_info().width, 320);
    EXPECT_EQ(core.get_av_info().height, 224);
}

TEST_F(EmulatorCoreTest, LoadInvalidCoreFailsGracefully) {
    bool success = core.load_core("non_existent_core.dylib");
    EXPECT_FALSE(success);
    EXPECT_FALSE(core.is_ready());
}

TEST_F(EmulatorCoreTest, LoadInvalidROMFails) {
    // Attempt to load game without core
    bool success = core.load_game("non_existent_game.bin");
    EXPECT_FALSE(success);
}

TEST_F(EmulatorCoreTest, ReadMemorySafety) {
    uint8_t buffer[10];
    // Should return 0 since no core/game loaded
    uint32_t read = core.read_memory_for_ra(0x0, buffer, 10);
    EXPECT_EQ(read, 0);
}

TEST_F(EmulatorCoreTest, ResetCleansState) {
    core.reset();
    EXPECT_FALSE(core.is_ready());
}
