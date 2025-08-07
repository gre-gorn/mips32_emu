#include "mips32_api.h"
#include "mips32_wrapper.h"
#include <new>
#include <cstring>

// Global error state for cases where we don't have a CPU instance
static MIPS32_Error g_last_error = MIPS32_ERROR_NONE;

// Helper function to cast opaque pointer
static MIPS32_Wrapper *GetWrapper(MIPS32_CPU *cpu)
{
    return reinterpret_cast<MIPS32_Wrapper *>(cpu);
}

// Core CPU management
extern "C" MIPS32_CPU *mips32_create(uint32_t memory_size_kb)
{
    if (memory_size_kb == 0 || memory_size_kb > 1024 * 1024)
    { // Max 1GB
        g_last_error = MIPS32_ERROR_INVALID_PARAMETER;
        return nullptr;
    }

    try
    {
        MIPS32_Wrapper *wrapper = new (std::nothrow) MIPS32_Wrapper(memory_size_kb);
        if (!wrapper)
        {
            g_last_error = MIPS32_ERROR_OUT_OF_MEMORY;
            return nullptr;
        }

        // Check if initialization succeeded
        if (wrapper->GetLastError() != MIPS32_ERROR_NONE)
        {
            MIPS32_Error error = wrapper->GetLastError();
            delete wrapper;
            g_last_error = error;
            return nullptr;
        }

        g_last_error = MIPS32_ERROR_NONE;
        return reinterpret_cast<MIPS32_CPU *>(wrapper);
    }
    catch (...)
    {
        g_last_error = MIPS32_ERROR_OUT_OF_MEMORY;
        return nullptr;
    }
}

extern "C" void mips32_destroy(MIPS32_CPU *cpu)
{
    if (cpu)
    {
        delete GetWrapper(cpu);
    }
}

// Memory Management
extern "C" bool mips32_load_program(MIPS32_CPU *cpu, const uint32_t *program, uint32_t program_size, uint32_t load_address)
{
    if (!cpu || !program)
    {
        g_last_error = MIPS32_ERROR_INVALID_PARAMETER;
        return false;
    }

    return GetWrapper(cpu)->LoadProgram(program, program_size, load_address);
}

extern "C" bool mips32_load_data(MIPS32_CPU *cpu, const uint8_t *data, uint32_t data_size, uint32_t load_address)
{
    if (!cpu || !data)
    {
        g_last_error = MIPS32_ERROR_INVALID_PARAMETER;
        return false;
    }

    return GetWrapper(cpu)->LoadData(data, data_size, load_address);
}

extern "C" uint32_t mips32_read_memory(MIPS32_CPU *cpu, uint32_t address)
{
    if (!cpu)
    {
        g_last_error = MIPS32_ERROR_INVALID_PARAMETER;
        return 0;
    }

    return GetWrapper(cpu)->ReadMemory(address);
}

extern "C" void mips32_write_memory(MIPS32_CPU *cpu, uint32_t address, uint8_t data)
{
    if (!cpu)
    {
        g_last_error = MIPS32_ERROR_INVALID_PARAMETER;
        return;
    }

    GetWrapper(cpu)->WriteMemory(address, data);
}

// Execution Control
extern "C" void mips32_reset(MIPS32_CPU *cpu)
{
    if (cpu)
    {
        GetWrapper(cpu)->Reset();
    }
}

extern "C" void mips32_set_pc(MIPS32_CPU *cpu, uint32_t address)
{
    if (cpu)
    {
        GetWrapper(cpu)->SetPC(address);
    }
}

extern "C" uint32_t mips32_get_pc(MIPS32_CPU *cpu)
{
    if (!cpu)
    {
        g_last_error = MIPS32_ERROR_INVALID_PARAMETER;
        return 0;
    }

    return GetWrapper(cpu)->GetPC();
}

extern "C" bool mips32_step(MIPS32_CPU *cpu)
{
    if (!cpu)
    {
        g_last_error = MIPS32_ERROR_INVALID_PARAMETER;
        return false;
    }

    return GetWrapper(cpu)->Step();
}

extern "C" bool mips32_run(MIPS32_CPU *cpu, uint32_t max_cycles)
{
    if (!cpu)
    {
        g_last_error = MIPS32_ERROR_INVALID_PARAMETER;
        return false;
    }

    return GetWrapper(cpu)->Run(max_cycles);
}

// Register Access
extern "C" uint32_t mips32_get_register(MIPS32_CPU *cpu, uint32_t reg_index)
{
    if (!cpu || reg_index >= 32)
    {
        g_last_error = MIPS32_ERROR_INVALID_PARAMETER;
        return 0;
    }

    return GetWrapper(cpu)->GetRegister(reg_index);
}

extern "C" void mips32_set_register(MIPS32_CPU *cpu, uint32_t reg_index, uint32_t value)
{
    if (!cpu || reg_index >= 32)
    {
        g_last_error = MIPS32_ERROR_INVALID_PARAMETER;
        return;
    }

    GetWrapper(cpu)->SetRegister(reg_index, value);
}

extern "C" uint32_t mips32_get_hi(MIPS32_CPU *cpu)
{
    if (!cpu)
    {
        g_last_error = MIPS32_ERROR_INVALID_PARAMETER;
        return 0;
    }

    return GetWrapper(cpu)->GetHI();
}

extern "C" uint32_t mips32_get_lo(MIPS32_CPU *cpu)
{
    if (!cpu)
    {
        g_last_error = MIPS32_ERROR_INVALID_PARAMETER;
        return 0;
    }

    return GetWrapper(cpu)->GetLO();
}

extern "C" void mips32_set_hi(MIPS32_CPU *cpu, uint32_t value)
{
    if (cpu)
    {
        GetWrapper(cpu)->SetHI(value);
    }
}

extern "C" void mips32_set_lo(MIPS32_CPU *cpu, uint32_t value)
{
    if (cpu)
    {
        GetWrapper(cpu)->SetLO(value);
    }
}

// State Management
extern "C" uint32_t mips32_get_cycle_count(MIPS32_CPU *cpu)
{
    if (!cpu)
    {
        g_last_error = MIPS32_ERROR_INVALID_PARAMETER;
        return 0;
    }

    return GetWrapper(cpu)->GetCycleCount();
}

extern "C" bool mips32_is_halted(MIPS32_CPU *cpu)
{
    if (!cpu)
    {
        g_last_error = MIPS32_ERROR_INVALID_PARAMETER;
        return true;
    }

    return GetWrapper(cpu)->IsHalted();
}

extern "C" void mips32_set_break_flag(MIPS32_CPU *cpu, bool should_break)
{
    if (cpu)
    {
        GetWrapper(cpu)->SetBreakFlag(should_break);
    }
}

// Debugging
extern "C" void mips32_enable_logging(MIPS32_CPU *cpu, bool enable)
{
    if (cpu)
    {
        GetWrapper(cpu)->EnableLogging(enable);
    }
}

extern "C" uint32_t mips32_get_last_instruction(MIPS32_CPU *cpu)
{
    if (!cpu)
    {
        g_last_error = MIPS32_ERROR_INVALID_PARAMETER;
        return 0;
    }

    return GetWrapper(cpu)->GetLastInstruction();
}

extern "C" const char *mips32_get_last_mnemonic(MIPS32_CPU *cpu)
{
    if (!cpu)
    {
        g_last_error = MIPS32_ERROR_INVALID_PARAMETER;
        return "unknown";
    }

    return GetWrapper(cpu)->GetLastMnemonic();
}

// Callback Registration
extern "C" void mips32_set_output_callback(MIPS32_CPU *cpu, MIPS32_OutputCallback callback)
{
    if (cpu)
    {
        GetWrapper(cpu)->SetOutputCallback(callback);
    }
}

extern "C" void mips32_set_input_callback(MIPS32_CPU *cpu, MIPS32_InputCallback callback)
{
    if (cpu)
    {
        GetWrapper(cpu)->SetInputCallback(callback);
    }
}

extern "C" void mips32_set_write_callback(MIPS32_CPU *cpu, MIPS32_WriteCallback callback)
{
    if (cpu)
    {
        GetWrapper(cpu)->SetWriteCallback(callback);
    }
}

// Utility Functions
extern "C" const char *mips32_get_version(void)
{
    return "MIPS32 Emulator v1.0.0 - Unity Compatible";
}

extern "C" const char *mips32_get_register_name(uint32_t reg_index)
{
    static const char *reg_names[32] = {
        "$zero", "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3",
        "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7",
        "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",
        "$t8", "$t9", "$k0", "$k1", "$gp", "$sp", "$fp", "$ra"};

    if (reg_index >= 32)
    {
        g_last_error = MIPS32_ERROR_INVALID_PARAMETER;
        return "unknown";
    }

    return reg_names[reg_index];
}

extern "C" bool mips32_save_state(MIPS32_CPU *cpu, uint8_t *buffer, uint32_t buffer_size)
{
    if (!cpu || !buffer)
    {
        g_last_error = MIPS32_ERROR_INVALID_PARAMETER;
        return false;
    }

    return GetWrapper(cpu)->SaveState(buffer, buffer_size);
}

extern "C" bool mips32_load_state(MIPS32_CPU *cpu, const uint8_t *buffer, uint32_t buffer_size)
{
    if (!cpu || !buffer)
    {
        g_last_error = MIPS32_ERROR_INVALID_PARAMETER;
        return false;
    }

    return GetWrapper(cpu)->LoadState(buffer, buffer_size);
}

// Error Handling
extern "C" MIPS32_Error mips32_get_last_error(MIPS32_CPU *cpu)
{
    if (cpu)
    {
        return GetWrapper(cpu)->GetLastError();
    }
    return g_last_error;
}

extern "C" const char *mips32_get_error_string(MIPS32_Error error)
{
    switch (error)
    {
    case MIPS32_ERROR_NONE:
        return "No error";
    case MIPS32_ERROR_INVALID_PARAMETER:
        return "Invalid parameter";
    case MIPS32_ERROR_OUT_OF_MEMORY:
        return "Out of memory";
    case MIPS32_ERROR_INVALID_ADDRESS:
        return "Invalid address";
    case MIPS32_ERROR_BUS_ERROR:
        return "Bus error";
    case MIPS32_ERROR_EXECUTION_FAILED:
        return "Execution failed";
    case MIPS32_ERROR_BUFFER_TOO_SMALL:
        return "Buffer too small";
    case MIPS32_ERROR_UNKNOWN:
        return "Unknown error";
    default:
        return "Invalid error code";
    }
}
