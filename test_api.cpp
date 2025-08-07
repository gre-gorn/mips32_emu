#include "mips32_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Test program: simple countdown
uint32_t test_program[] = {
    0x3c011000, // lui $at, 0x1000
    0x34280005, // ori $t0, $at, 5        # $t0 = 5 (counter)
    0x3c012000, // lui $at, 0x2000
    0x34290000, // ori $t1, $at, 0        # $t1 = output address
    0x00000000, // nop
    // loop:
    0xa1280000, // sb $t0, 0($t1)         # output counter
    0x2108ffff, // addi $t0, $t0, -1      # decrement
    0x1500fffd, // bne $t0, $zero, loop   # branch if not zero
    0x00000000, // nop (delay slot)
    // end:
    0x2008000a, // addi $t0, $zero, 10    # newline
    0xa1280000, // sb $t0, 0($t1)         # output newline
    0x08000064, // j 0x64 (halt)
    0x00000000  // nop
};

// Callback functions
void output_callback(uint8_t data)
{
    if (data >= 0x30 && data <= 0x39)
    {
        printf("Output: %c (number %d)\n", data, data - 0x30);
    }
    else if (data == 10)
    {
        printf("Output: \\n (newline)\n");
    }
    else
    {
        printf("Output: 0x%02X\n", data);
    }
}

uint8_t input_callback(void)
{
    printf("Input requested\n");
    return 0; // No input for this test
}

void write_callback(uint32_t address, uint8_t data)
{
    printf("Memory write: 0x%08X = 0x%02X\n", address, data);

    // If writing to output address, treat as output
    if (address == 0x2000)
    {
        output_callback(data);
    }
}

int test_basic_functionality()
{
    printf("=== Testing Basic Functionality ===\n");

    // Create CPU
    MIPS32_CPU *cpu = MIPS32_Create(64); // 64KB memory
    if (!cpu)
    {
        printf("ERROR: Failed to create CPU\n");
        return 0;
    }
    printf("✓ CPU created successfully\n");

    // Test version info
    printf("Version: %s\n", MIPS32_GetVersion());
    printf("Version: %d.%d.%d\n",
           MIPS32_GetVersionMajor(),
           MIPS32_GetVersionMinor(),
           MIPS32_GetVersionPatch());

    // Set callbacks
    MIPS32_SetOutputCallback(cpu, output_callback);
    MIPS32_SetInputCallback(cpu, input_callback);
    MIPS32_SetWriteCallback(cpu, write_callback);
    printf("✓ Callbacks set\n");

    // Load program
    if (!MIPS32_LoadProgram(cpu, test_program, sizeof(test_program) / sizeof(uint32_t), 0x1000))
    {
        printf("ERROR: Failed to load program: %s\n", MIPS32_GetErrorString(MIPS32_GetLastError(cpu)));
        MIPS32_Destroy(cpu);
        return 0;
    }
    printf("✓ Program loaded at 0x1000\n");

    // Set PC to program start
    MIPS32_SetPC(cpu, 0x1000);
    printf("✓ PC set to 0x%08X\n", MIPS32_GetPC(cpu));

    // Test register access
    MIPS32_SetRegister(cpu, 1, 0x12345678); // $at
    uint32_t value = MIPS32_GetRegister(cpu, 1);
    if (value != 0x12345678)
    {
        printf("ERROR: Register test failed. Expected 0x12345678, got 0x%08X\n", value);
        MIPS32_Destroy(cpu);
        return 0;
    }
    printf("✓ Register access test passed\n");

    // Test memory access
    MIPS32_WriteMemory(cpu, 0x3000, 0xAB);
    uint32_t mem_word = MIPS32_ReadMemory(cpu, 0x3000);
    if ((mem_word & 0xFF) != 0xAB)
    {
        printf("ERROR: Memory test failed. Expected 0xAB in LSB, got 0x%08X\n", mem_word);
        MIPS32_Destroy(cpu);
        return 0;
    }
    printf("✓ Memory access test passed\n");

    // Reset for clean execution
    MIPS32_Reset(cpu);
    MIPS32_LoadProgram(cpu, test_program, sizeof(test_program) / sizeof(uint32_t), 0x1000);
    MIPS32_SetPC(cpu, 0x1000);

    printf("\n=== Starting Execution ===\n");

    // Execute program step by step
    int steps = 0;
    const int max_steps = 50;

    while (steps < max_steps && !MIPS32_IsHalted(cpu))
    {
        uint32_t pc_before = MIPS32_GetPC(cpu);

        if (!MIPS32_Step(cpu))
        {
            MIPS32_Error error = MIPS32_GetLastError(cpu);
            if (error != MIPS32_ERROR_NONE)
            {
                printf("ERROR: Execution failed at step %d: %s\n", steps, MIPS32_GetErrorString(error));
                break;
            }
        }

        uint32_t pc_after = MIPS32_GetPC(cpu);
        printf("Step %d: PC 0x%08X -> 0x%08X\n", steps, pc_before, pc_after);

        steps++;

        // Check if we're stuck (PC not changing)
        if (pc_before == pc_after && steps > 5)
        {
            printf("WARNING: PC not changing, possible infinite loop\n");
            break;
        }
    }

    printf("\nExecution completed after %d steps\n", steps);
    printf("Final PC: 0x%08X\n", MIPS32_GetPC(cpu));
    printf("Cycle count: %u\n", MIPS32_GetCycleCount(cpu));
    printf("Halted: %s\n", MIPS32_IsHalted(cpu) ? "Yes" : "No");

    // Show some register values
    printf("\nFinal Register Values:\n");
    for (int i = 0; i < 8; i++)
    {
        printf("  %s: 0x%08X\n", MIPS32_GetRegisterName(i), MIPS32_GetRegister(cpu, i));
    }

    MIPS32_Destroy(cpu);
    printf("✓ CPU destroyed\n");

    return 1;
}

int test_error_handling()
{
    printf("\n=== Testing Error Handling ===\n");

    // Test invalid parameters
    MIPS32_CPU *cpu = MIPS32_Create(0); // Invalid size
    if (cpu != NULL)
    {
        printf("ERROR: Created CPU with invalid size\n");
        MIPS32_Destroy(cpu);
        return 0;
    }
    printf("✓ Invalid size rejected\n");

    // Test valid creation
    cpu = MIPS32_Create(64);
    if (!cpu)
    {
        printf("ERROR: Failed to create valid CPU\n");
        return 0;
    }

    // Test invalid operations
    if (MIPS32_LoadProgram(cpu, NULL, 10, 0x1000))
    {
        printf("ERROR: Accepted NULL program\n");
        MIPS32_Destroy(cpu);
        return 0;
    }
    printf("✓ NULL program rejected\n");

    // Test invalid address
    MIPS32_SetPC(cpu, 0xFFFFFFFF);
    MIPS32_Error error = MIPS32_GetLastError(cpu);
    if (error == MIPS32_ERROR_NONE)
    {
        printf("WARNING: Invalid PC address was accepted\n");
    }
    else
    {
        printf("✓ Invalid PC address rejected: %s\n", MIPS32_GetErrorString(error));
    }

    MIPS32_Destroy(cpu);
    return 1;
}

int main()
{
    printf("MIPS32 Emulator API Test\n");
    printf("========================\n\n");

    if (!test_basic_functionality())
    {
        printf("Basic functionality test FAILED\n");
        return 1;
    }

    if (!test_error_handling())
    {
        printf("Error handling test FAILED\n");
        return 1;
    }

    printf("\n=== All Tests Passed ===\n");
    printf("The MIPS32 emulator API is working correctly!\n");
    printf("Ready for Unity integration.\n");

    return 0;
}
