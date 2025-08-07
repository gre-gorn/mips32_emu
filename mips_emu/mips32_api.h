#ifndef MIPS32_API_H
#define MIPS32_API_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>

// Export macros for different platforms
#ifdef _WIN32
#ifdef MIPS32_EXPORTS
#define MIPS32_API __declspec(dllexport)
#else
#define MIPS32_API __declspec(dllimport)
#endif
#else
#define MIPS32_API __attribute__((visibility("default")))
#endif

    // Forward declarations
    typedef struct MIPS32_CPU MIPS32_CPU;

    // Callback function types for Unity integration
    typedef void (*MIPS32_OutputCallback)(int syscall_type, const char *data);
    typedef uint8_t (*MIPS32_InputCallback)(uint32_t address);
    typedef void (*MIPS32_WriteCallback)(uint32_t address, uint8_t data);

    // CPU Creation and Destruction
    MIPS32_API MIPS32_CPU *mips32_create(uint32_t memory_size_kb);
    MIPS32_API void mips32_destroy(MIPS32_CPU *cpu);

    // Memory Management
    MIPS32_API bool mips32_load_program(MIPS32_CPU *cpu, const uint32_t *program, uint32_t program_size, uint32_t load_address);
    MIPS32_API bool mips32_load_data(MIPS32_CPU *cpu, const uint8_t *data, uint32_t data_size, uint32_t load_address);
    MIPS32_API uint32_t mips32_read_memory(MIPS32_CPU *cpu, uint32_t address);
    MIPS32_API void mips32_write_memory(MIPS32_CPU *cpu, uint32_t address, uint8_t data);

    // Execution Control
    MIPS32_API void mips32_reset(MIPS32_CPU *cpu);
    MIPS32_API void mips32_set_pc(MIPS32_CPU *cpu, uint32_t address);
    MIPS32_API uint32_t mips32_get_pc(MIPS32_CPU *cpu);
    MIPS32_API bool mips32_step(MIPS32_CPU *cpu);                     // Returns true if execution should continue
    MIPS32_API bool mips32_run(MIPS32_CPU *cpu, uint32_t max_cycles); // Run for specified cycles

    // Register Access
    MIPS32_API uint32_t mips32_get_register(MIPS32_CPU *cpu, uint32_t reg_index);
    MIPS32_API void mips32_set_register(MIPS32_CPU *cpu, uint32_t reg_index, uint32_t value);
    MIPS32_API uint32_t mips32_get_hi(MIPS32_CPU *cpu);
    MIPS32_API uint32_t mips32_get_lo(MIPS32_CPU *cpu);
    MIPS32_API void mips32_set_hi(MIPS32_CPU *cpu, uint32_t value);
    MIPS32_API void mips32_set_lo(MIPS32_CPU *cpu, uint32_t value);

    // State Management
    MIPS32_API uint32_t mips32_get_cycle_count(MIPS32_CPU *cpu);
    MIPS32_API bool mips32_is_halted(MIPS32_CPU *cpu);
    MIPS32_API void mips32_set_break_flag(MIPS32_CPU *cpu, bool should_break);

    // Debugging
    MIPS32_API void mips32_enable_logging(MIPS32_CPU *cpu, bool enable);
    MIPS32_API uint32_t mips32_get_last_instruction(MIPS32_CPU *cpu);
    MIPS32_API const char *mips32_get_last_mnemonic(MIPS32_CPU *cpu);

    // Callback Registration
    MIPS32_API void mips32_set_output_callback(MIPS32_CPU *cpu, MIPS32_OutputCallback callback);
    MIPS32_API void mips32_set_input_callback(MIPS32_CPU *cpu, MIPS32_InputCallback callback);
    MIPS32_API void mips32_set_write_callback(MIPS32_CPU *cpu, MIPS32_WriteCallback callback);

    // Utility Functions
    MIPS32_API const char *mips32_get_version(void);
    MIPS32_API const char *mips32_get_register_name(uint32_t reg_index);
    MIPS32_API bool mips32_save_state(MIPS32_CPU *cpu, uint8_t *buffer, uint32_t buffer_size);
    MIPS32_API bool mips32_load_state(MIPS32_CPU *cpu, const uint8_t *buffer, uint32_t buffer_size);

    // Error Handling
    typedef enum
    {
        MIPS32_ERROR_NONE = 0,
        MIPS32_ERROR_INVALID_PARAMETER = 1,
        MIPS32_ERROR_OUT_OF_MEMORY = 2,
        MIPS32_ERROR_INVALID_ADDRESS = 3,
        MIPS32_ERROR_BUS_ERROR = 4,
        MIPS32_ERROR_EXECUTION_FAILED = 5,
        MIPS32_ERROR_BUFFER_TOO_SMALL = 6,
        MIPS32_ERROR_UNKNOWN = 7
    } MIPS32_Error;

    MIPS32_API MIPS32_Error mips32_get_last_error(MIPS32_CPU *cpu);
    MIPS32_API const char *mips32_get_error_string(MIPS32_Error error);

#ifdef __cplusplus
}
#endif

#endif // MIPS32_API_H
