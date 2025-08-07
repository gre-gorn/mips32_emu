#ifndef MIPS32_WRAPPER_H
#define MIPS32_WRAPPER_H

#include "mips32_api.h"
#include "mips32.h"
#include "bus.h"
#include <string>
#include <memory>

class MIPS32_Wrapper
{
public:
    MIPS32_Wrapper(uint32_t memory_size_kb);
    ~MIPS32_Wrapper();

    // Core functionality
    bool LoadProgram(const uint32_t *program, uint32_t program_size, uint32_t load_address);
    bool LoadData(const uint8_t *data, uint32_t data_size, uint32_t load_address);
    uint32_t ReadMemory(uint32_t address);
    void WriteMemory(uint32_t address, uint8_t data);

    // Execution control
    void Reset();
    void SetPC(uint32_t address);
    uint32_t GetPC();
    bool Step();
    bool Run(uint32_t max_cycles);

    // Register access
    uint32_t GetRegister(uint32_t reg_index);
    void SetRegister(uint32_t reg_index, uint32_t value);
    uint32_t GetHI();
    uint32_t GetLO();
    void SetHI(uint32_t value);
    void SetLO(uint32_t value);

    // State management
    uint32_t GetCycleCount();
    bool IsHalted();
    void SetBreakFlag(bool should_break);

    // Debugging
    void EnableLogging(bool enable);
    uint32_t GetLastInstruction();
    const char *GetLastMnemonic();

    // Callbacks
    void SetOutputCallback(MIPS32_OutputCallback callback);
    void SetInputCallback(MIPS32_InputCallback callback);
    void SetWriteCallback(MIPS32_WriteCallback callback);

    // Error handling
    MIPS32_Error GetLastError();
    void SetLastError(MIPS32_Error error);

    // State serialization
    bool SaveState(uint8_t *buffer, uint32_t buffer_size);
    bool LoadState(const uint8_t *buffer, uint32_t buffer_size);

private:
    std::unique_ptr<MIPS32> m_cpu;
    std::unique_ptr<Bus> m_bus;
    MIPS32_Error m_last_error;
    uint32_t m_memory_size;

    // Callbacks
    MIPS32_OutputCallback m_output_callback;
    MIPS32_InputCallback m_input_callback;
    MIPS32_WriteCallback m_write_callback;

    // State tracking
    uint32_t m_last_instruction;
    std::string m_last_mnemonic;
    bool m_halted;
    uint32_t m_cycle_count;

    // Helper methods
    bool IsValidRegister(uint32_t reg_index);
    bool IsValidAddress(uint32_t address);
    void UpdateLastInstruction();
};

#endif // MIPS32_WRAPPER_H
