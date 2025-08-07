#include "mips32_wrapper.h"
#include <cstring>
#include <algorithm>

MIPS32_Wrapper::MIPS32_Wrapper(uint32_t memory_size_kb)
    : m_last_error(MIPS32_ERROR_NONE), m_memory_size(memory_size_kb * 1024), m_output_callback(nullptr), m_input_callback(nullptr), m_write_callback(nullptr), m_last_instruction(0), m_halted(false), m_cycle_count(0)
{
    try
    {
        m_bus.reset(new Bus(memory_size_kb));
        m_cpu.reset(new MIPS32(m_bus.get()));
    }
    catch (...)
    {
        m_last_error = MIPS32_ERROR_OUT_OF_MEMORY;
    }
}

MIPS32_Wrapper::~MIPS32_Wrapper()
{
    // Smart pointers handle cleanup automatically
}

bool MIPS32_Wrapper::LoadProgram(const uint32_t *program, uint32_t program_size, uint32_t load_address)
{
    if (!program || program_size == 0)
    {
        m_last_error = MIPS32_ERROR_INVALID_PARAMETER;
        return false;
    }

    if (load_address + (program_size * sizeof(uint32_t)) > m_memory_size)
    {
        m_last_error = MIPS32_ERROR_INVALID_ADDRESS;
        return false;
    }

    try
    {
        // Copy program to memory (convert from host endianness if needed)
        for (uint32_t i = 0; i < program_size; ++i)
        {
            uint32_t addr = load_address + (i * sizeof(uint32_t));
            uint32_t instruction = program[i];

            // Write as bytes to handle endianness
            m_bus->Write(addr, (instruction >> 24) & 0xFF);
            m_bus->Write(addr + 1, (instruction >> 16) & 0xFF);
            m_bus->Write(addr + 2, (instruction >> 8) & 0xFF);
            m_bus->Write(addr + 3, instruction & 0xFF);
        }

        m_last_error = MIPS32_ERROR_NONE;
        return true;
    }
    catch (...)
    {
        m_last_error = MIPS32_ERROR_BUS_ERROR;
        return false;
    }
}

bool MIPS32_Wrapper::LoadData(const uint8_t *data, uint32_t data_size, uint32_t load_address)
{
    if (!data || data_size == 0)
    {
        m_last_error = MIPS32_ERROR_INVALID_PARAMETER;
        return false;
    }

    if (load_address + data_size > m_memory_size)
    {
        m_last_error = MIPS32_ERROR_INVALID_ADDRESS;
        return false;
    }

    try
    {
        for (uint32_t i = 0; i < data_size; ++i)
        {
            m_bus->Write(load_address + i, data[i]);
        }

        m_last_error = MIPS32_ERROR_NONE;
        return true;
    }
    catch (...)
    {
        m_last_error = MIPS32_ERROR_BUS_ERROR;
        return false;
    }
}

uint32_t MIPS32_Wrapper::ReadMemory(uint32_t address)
{
    if (!IsValidAddress(address))
    {
        m_last_error = MIPS32_ERROR_INVALID_ADDRESS;
        return 0;
    }

    try
    {
        return m_bus->Read(address);
    }
    catch (...)
    {
        m_last_error = MIPS32_ERROR_BUS_ERROR;
        return 0;
    }
}

void MIPS32_Wrapper::WriteMemory(uint32_t address, uint8_t data)
{
    if (!IsValidAddress(address))
    {
        m_last_error = MIPS32_ERROR_INVALID_ADDRESS;
        return;
    }

    try
    {
        m_bus->Write(address, data);

        // Trigger write callback if set
        if (m_write_callback)
        {
            m_write_callback(address, data);
        }

        m_last_error = MIPS32_ERROR_NONE;
    }
    catch (...)
    {
        m_last_error = MIPS32_ERROR_BUS_ERROR;
    }
}

void MIPS32_Wrapper::Reset()
{
    try
    {
        m_cpu->Reset();
        m_halted = false;
        m_cycle_count = 0;
        m_last_instruction = 0;
        m_last_mnemonic.clear();
        m_last_error = MIPS32_ERROR_NONE;
    }
    catch (...)
    {
        m_last_error = MIPS32_ERROR_UNKNOWN;
    }
}

void MIPS32_Wrapper::SetPC(uint32_t address)
{
    if (!IsValidAddress(address))
    {
        m_last_error = MIPS32_ERROR_INVALID_ADDRESS;
        return;
    }

    try
    {
        m_cpu->SetPC(address);
        m_last_error = MIPS32_ERROR_NONE;
    }
    catch (...)
    {
        m_last_error = MIPS32_ERROR_UNKNOWN;
    }
}

uint32_t MIPS32_Wrapper::GetPC()
{
    try
    {
        return m_cpu->GetPC();
    }
    catch (...)
    {
        m_last_error = MIPS32_ERROR_UNKNOWN;
        return 0;
    }
}

bool MIPS32_Wrapper::Step()
{
    if (m_halted)
    {
        return false;
    }

    try
    {
        m_cpu->Tick();
        m_cycle_count++;
        UpdateLastInstruction();
        m_last_error = MIPS32_ERROR_NONE;
        return true;
    }
    catch (...)
    {
        m_last_error = MIPS32_ERROR_EXECUTION_FAILED;
        m_halted = true;
        return false;
    }
}

bool MIPS32_Wrapper::Run(uint32_t max_cycles)
{
    if (m_halted)
    {
        return false;
    }

    try
    {
        for (uint32_t i = 0; i < max_cycles && !m_halted; ++i)
        {
            if (!Step())
            {
                return false;
            }
        }
        return true;
    }
    catch (...)
    {
        m_last_error = MIPS32_ERROR_EXECUTION_FAILED;
        m_halted = true;
        return false;
    }
}

uint32_t MIPS32_Wrapper::GetRegister(uint32_t reg_index)
{
    if (!IsValidRegister(reg_index))
    {
        m_last_error = MIPS32_ERROR_INVALID_PARAMETER;
        return 0;
    }

    try
    {
        return m_cpu->GetRegister(reg_index);
    }
    catch (...)
    {
        m_last_error = MIPS32_ERROR_UNKNOWN;
        return 0;
    }
}

void MIPS32_Wrapper::SetRegister(uint32_t reg_index, uint32_t value)
{
    if (!IsValidRegister(reg_index))
    {
        m_last_error = MIPS32_ERROR_INVALID_PARAMETER;
        return;
    }

    try
    {
        m_cpu->SetRegister(reg_index, value);
        m_last_error = MIPS32_ERROR_NONE;
    }
    catch (...)
    {
        m_last_error = MIPS32_ERROR_UNKNOWN;
    }
}

uint32_t MIPS32_Wrapper::GetHI()
{
    try
    {
        return m_cpu->GetRegister(32); // HI register index
    }
    catch (...)
    {
        m_last_error = MIPS32_ERROR_UNKNOWN;
        return 0;
    }
}

uint32_t MIPS32_Wrapper::GetLO()
{
    try
    {
        return m_cpu->GetRegister(33); // LO register index
    }
    catch (...)
    {
        m_last_error = MIPS32_ERROR_UNKNOWN;
        return 0;
    }
}

void MIPS32_Wrapper::SetHI(uint32_t value)
{
    try
    {
        m_cpu->SetRegister(32, value); // HI register index
        m_last_error = MIPS32_ERROR_NONE;
    }
    catch (...)
    {
        m_last_error = MIPS32_ERROR_UNKNOWN;
    }
}

void MIPS32_Wrapper::SetLO(uint32_t value)
{
    try
    {
        m_cpu->SetRegister(33, value); // LO register index
        m_last_error = MIPS32_ERROR_NONE;
    }
    catch (...)
    {
        m_last_error = MIPS32_ERROR_UNKNOWN;
    }
}

uint32_t MIPS32_Wrapper::GetCycleCount()
{
    return m_cycle_count;
}

bool MIPS32_Wrapper::IsHalted()
{
    return m_halted;
}

void MIPS32_Wrapper::SetBreakFlag(bool should_break)
{
    m_halted = should_break;
}

void MIPS32_Wrapper::EnableLogging(bool enable)
{
    try
    {
        m_cpu->EnableLog(enable);
        m_last_error = MIPS32_ERROR_NONE;
    }
    catch (...)
    {
        m_last_error = MIPS32_ERROR_UNKNOWN;
    }
}

uint32_t MIPS32_Wrapper::GetLastInstruction()
{
    return m_last_instruction;
}

const char *MIPS32_Wrapper::GetLastMnemonic()
{
    return m_last_mnemonic.c_str();
}

void MIPS32_Wrapper::SetOutputCallback(MIPS32_OutputCallback callback)
{
    m_output_callback = callback;
}

void MIPS32_Wrapper::SetInputCallback(MIPS32_InputCallback callback)
{
    m_input_callback = callback;
}

void MIPS32_Wrapper::SetWriteCallback(MIPS32_WriteCallback callback)
{
    m_write_callback = callback;
}

MIPS32_Error MIPS32_Wrapper::GetLastError()
{
    return m_last_error;
}

void MIPS32_Wrapper::SetLastError(MIPS32_Error error)
{
    m_last_error = error;
}

bool MIPS32_Wrapper::SaveState(uint8_t *buffer, uint32_t buffer_size)
{
    if (!buffer)
    {
        m_last_error = MIPS32_ERROR_INVALID_PARAMETER;
        return false;
    }

    // Calculate required size: registers + memory + metadata
    uint32_t required_size = (34 * sizeof(uint32_t)) + m_memory_size + (4 * sizeof(uint32_t));

    if (buffer_size < required_size)
    {
        m_last_error = MIPS32_ERROR_BUFFER_TOO_SMALL;
        return false;
    }

    try
    {
        uint8_t *ptr = buffer;

        // Save registers (32 general + PC + HI + LO)
        for (int i = 0; i < 34; ++i)
        {
            uint32_t value = m_cpu->GetRegister(i);
            *reinterpret_cast<uint32_t *>(ptr) = value;
            ptr += sizeof(uint32_t);
        }

        // Save metadata
        *reinterpret_cast<uint32_t *>(ptr) = m_cycle_count;
        ptr += sizeof(uint32_t);
        *reinterpret_cast<uint32_t *>(ptr) = m_halted ? 1 : 0;
        ptr += sizeof(uint32_t);
        *reinterpret_cast<uint32_t *>(ptr) = m_last_instruction;
        ptr += sizeof(uint32_t);
        *reinterpret_cast<uint32_t *>(ptr) = static_cast<uint32_t>(m_last_error);
        ptr += sizeof(uint32_t);

        // Save memory (read byte by byte)
        for (uint32_t i = 0; i < m_memory_size; ++i)
        {
            if (i < m_bus->Size())
            {
                // Read single byte by reading a word and extracting byte
                uint32_t word_addr = i & ~3U;  // Align to word boundary
                uint32_t byte_offset = i & 3U; // Get byte offset within word
                uint32_t word = m_bus->Read(word_addr);
                *ptr++ = (word >> (24 - (byte_offset * 8))) & 0xFF;
            }
            else
            {
                *ptr++ = 0;
            }
        }

        m_last_error = MIPS32_ERROR_NONE;
        return true;
    }
    catch (...)
    {
        m_last_error = MIPS32_ERROR_UNKNOWN;
        return false;
    }
}

bool MIPS32_Wrapper::LoadState(const uint8_t *buffer, uint32_t buffer_size)
{
    if (!buffer)
    {
        m_last_error = MIPS32_ERROR_INVALID_PARAMETER;
        return false;
    }

    uint32_t required_size = (34 * sizeof(uint32_t)) + m_memory_size + (4 * sizeof(uint32_t));

    if (buffer_size < required_size)
    {
        m_last_error = MIPS32_ERROR_BUFFER_TOO_SMALL;
        return false;
    }

    try
    {
        const uint8_t *ptr = buffer;

        // Load registers
        for (int i = 0; i < 34; ++i)
        {
            uint32_t value = *reinterpret_cast<const uint32_t *>(ptr);
            m_cpu->SetRegister(i, value);
            ptr += sizeof(uint32_t);
        }

        // Load metadata
        m_cycle_count = *reinterpret_cast<const uint32_t *>(ptr);
        ptr += sizeof(uint32_t);
        m_halted = (*reinterpret_cast<const uint32_t *>(ptr) != 0);
        ptr += sizeof(uint32_t);
        m_last_instruction = *reinterpret_cast<const uint32_t *>(ptr);
        ptr += sizeof(uint32_t);
        m_last_error = static_cast<MIPS32_Error>(*reinterpret_cast<const uint32_t *>(ptr));
        ptr += sizeof(uint32_t);

        // Load memory
        for (uint32_t i = 0; i < m_memory_size; ++i)
        {
            m_bus->Write(i, *ptr++);
        }

        m_last_error = MIPS32_ERROR_NONE;
        return true;
    }
    catch (...)
    {
        m_last_error = MIPS32_ERROR_UNKNOWN;
        return false;
    }
}

// Helper methods
bool MIPS32_Wrapper::IsValidRegister(uint32_t reg_index)
{
    return reg_index < 34; // 32 general + PC + HI + LO
}

bool MIPS32_Wrapper::IsValidAddress(uint32_t address)
{
    return address < m_memory_size;
}

void MIPS32_Wrapper::UpdateLastInstruction()
{
    try
    {
        uint32_t pc = m_cpu->GetPC();
        if (pc >= 4)
        { // Get previous instruction
            m_last_instruction = m_bus->Read(pc - 4);
            // Here you could add instruction disassembly to get mnemonic
            m_last_mnemonic = "unknown"; // Placeholder
        }
    }
    catch (...)
    {
        // Ignore errors in debug info
    }
}
