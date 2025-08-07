using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UnityEngine;

namespace MIPS32Unity
{
    /// <summary>
    /// Unity wrapper for MIPS32 emulator
    /// Provides high-level interface for running MIPS32 programs in Unity
    /// </summary>
    public class MIPS32Emulator : MonoBehaviour
    {
        #region Native Library Imports

        // Platform-specific library names
#if UNITY_EDITOR_WIN || UNITY_STANDALONE_WIN
            private const string LIBRARY_NAME = "mips32_emu";
#elif UNITY_EDITOR_OSX || UNITY_STANDALONE_OSX
            private const string LIBRARY_NAME = "libmips32_emu";
#elif UNITY_EDITOR_LINUX || UNITY_STANDALONE_LINUX
            private const string LIBRARY_NAME = "libmips32_emu";
#else
        private const string LIBRARY_NAME = "mips32_emu";
#endif

        // Error codes
        public enum ErrorCode
        {
            None = 0,
            InvalidParameter = 1,
            OutOfMemory = 2,
            InvalidAddress = 3,
            BusError = 4,
            ExecutionFailed = 5,
            BufferTooSmall = 6,
            Unknown = 7
        }

        // Callback delegates
        public delegate void OutputCallback(byte data);
        public delegate byte InputCallback();
        public delegate void WriteCallback(uint address, byte data);

        // Core CPU management
        [DllImport(LIBRARY_NAME, CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr MIPS32_Create(uint memory_size_kb);

        [DllImport(LIBRARY_NAME, CallingConvention = CallingConvention.Cdecl)]
        private static extern void MIPS32_Destroy(IntPtr cpu);

        [DllImport(LIBRARY_NAME, CallingConvention = CallingConvention.Cdecl)]
        private static extern void MIPS32_Reset(IntPtr cpu);

        // Memory management
        [DllImport(LIBRARY_NAME, CallingConvention = CallingConvention.Cdecl)]
        private static extern int MIPS32_LoadProgram(IntPtr cpu, uint[] program, uint program_size, uint load_address);

        [DllImport(LIBRARY_NAME, CallingConvention = CallingConvention.Cdecl)]
        private static extern int MIPS32_LoadData(IntPtr cpu, byte[] data, uint data_size, uint load_address);

        [DllImport(LIBRARY_NAME, CallingConvention = CallingConvention.Cdecl)]
        private static extern uint MIPS32_ReadMemory(IntPtr cpu, uint address);

        [DllImport(LIBRARY_NAME, CallingConvention = CallingConvention.Cdecl)]
        private static extern void MIPS32_WriteMemory(IntPtr cpu, uint address, byte data);

        // Execution control
        [DllImport(LIBRARY_NAME, CallingConvention = CallingConvention.Cdecl)]
        private static extern void MIPS32_SetPC(IntPtr cpu, uint address);

        [DllImport(LIBRARY_NAME, CallingConvention = CallingConvention.Cdecl)]
        private static extern uint MIPS32_GetPC(IntPtr cpu);

        [DllImport(LIBRARY_NAME, CallingConvention = CallingConvention.Cdecl)]
        private static extern int MIPS32_Step(IntPtr cpu);

        [DllImport(LIBRARY_NAME, CallingConvention = CallingConvention.Cdecl)]
        private static extern int MIPS32_Run(IntPtr cpu, uint max_cycles);

        // Register access
        [DllImport(LIBRARY_NAME, CallingConvention = CallingConvention.Cdecl)]
        private static extern uint MIPS32_GetRegister(IntPtr cpu, uint reg_index);

        [DllImport(LIBRARY_NAME, CallingConvention = CallingConvention.Cdecl)]
        private static extern void MIPS32_SetRegister(IntPtr cpu, uint reg_index, uint value);

        // State management
        [DllImport(LIBRARY_NAME, CallingConvention = CallingConvention.Cdecl)]
        private static extern uint MIPS32_GetCycleCount(IntPtr cpu);

        [DllImport(LIBRARY_NAME, CallingConvention = CallingConvention.Cdecl)]
        private static extern int MIPS32_IsHalted(IntPtr cpu);

        [DllImport(LIBRARY_NAME, CallingConvention = CallingConvention.Cdecl)]
        private static extern void MIPS32_SetBreakFlag(IntPtr cpu, int should_break);

        // Callbacks
        [DllImport(LIBRARY_NAME, CallingConvention = CallingConvention.Cdecl)]
        private static extern void MIPS32_SetOutputCallback(IntPtr cpu, OutputCallback callback);

        [DllImport(LIBRARY_NAME, CallingConvention = CallingConvention.Cdecl)]
        private static extern void MIPS32_SetInputCallback(IntPtr cpu, InputCallback callback);

        [DllImport(LIBRARY_NAME, CallingConvention = CallingConvention.Cdecl)]
        private static extern void MIPS32_SetWriteCallback(IntPtr cpu, WriteCallback callback);

        // Error handling
        [DllImport(LIBRARY_NAME, CallingConvention = CallingConvention.Cdecl)]
        private static extern ErrorCode MIPS32_GetLastError(IntPtr cpu);

        [DllImport(LIBRARY_NAME, CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr MIPS32_GetErrorString(ErrorCode error);

        // Utility functions
        [DllImport(LIBRARY_NAME, CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr MIPS32_GetRegisterName(uint reg_index);

        [DllImport(LIBRARY_NAME, CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr MIPS32_GetVersion();

        #endregion

        #region Public Properties and Events

        [Header("Emulator Configuration")]
        [SerializeField] private uint memorySizeKB = 64;  // 64KB default
        [SerializeField] private bool autoStart = false;
        [SerializeField] private uint maxCyclesPerFrame = 1000;

        [Header("Program Loading")]
        [SerializeField] private TextAsset programFile;
        [SerializeField] private uint loadAddress = 0x1000;

        [Header("Debugging")]
        [SerializeField] private bool enableLogging = true;
        [SerializeField] private bool showRegisters = false;

        // Events
        public event Action<string> OnOutput;
        public event Action<uint, byte> OnMemoryWrite;
        public event Action<ErrorCode, string> OnError;
        public event Action OnHalted;

        // Public properties
        public bool IsRunning { get; private set; }
        public bool IsHalted => _cpuHandle != IntPtr.Zero && MIPS32_IsHalted(_cpuHandle) != 0;
        public uint CycleCount => _cpuHandle != IntPtr.Zero ? MIPS32_GetCycleCount(_cpuHandle) : 0;
        public uint PC => _cpuHandle != IntPtr.Zero ? MIPS32_GetPC(_cpuHandle) : 0;

        #endregion

        #region Private Fields

        private IntPtr _cpuHandle = IntPtr.Zero;
        private OutputCallback _outputCallback;
        private InputCallback _inputCallback;
        private WriteCallback _writeCallback;
        private List<string> _outputBuffer = new List<string>();
        private Queue<byte> _inputBuffer = new Queue<byte>();

        #endregion

        #region Unity Lifecycle

        void Start()
        {
            Initialize();

            if (autoStart && programFile != null)
            {
                LoadProgramFromTextAsset(programFile);
                StartExecution();
            }
        }

        void Update()
        {
            if (IsRunning && !IsHalted)
            {
                // Run emulator for a limited number of cycles per frame
                RunCycles(maxCyclesPerFrame);
            }
        }

        void OnDestroy()
        {
            Shutdown();
        }

        #endregion

        #region Public Methods

        /// <summary>
        /// Initialize the MIPS32 emulator
        /// </summary>
        public bool Initialize()
        {
            if (_cpuHandle != IntPtr.Zero)
            {
                LogWarning("Emulator already initialized");
                return true;
            }

            _cpuHandle = MIPS32_Create(memorySizeKB);
            if (_cpuHandle == IntPtr.Zero)
            {
                LogError("Failed to create MIPS32 emulator");
                return false;
            }

            // Set up callbacks
            _outputCallback = OnOutputCallback;
            _inputCallback = OnInputCallback;
            _writeCallback = OnWriteCallback;

            MIPS32_SetOutputCallback(_cpuHandle, _outputCallback);
            MIPS32_SetInputCallback(_cpuHandle, _inputCallback);
            MIPS32_SetWriteCallback(_cpuHandle, _writeCallback);

            Log($"MIPS32 Emulator initialized with {memorySizeKB}KB memory");
            Log($"Version: {GetVersion()}");

            return true;
        }

        /// <summary>
        /// Shutdown the emulator and clean up resources
        /// </summary>
        public void Shutdown()
        {
            if (_cpuHandle != IntPtr.Zero)
            {
                MIPS32_Destroy(_cpuHandle);
                _cpuHandle = IntPtr.Zero;
                IsRunning = false;
                Log("MIPS32 Emulator shutdown");
            }
        }

        /// <summary>
        /// Reset the CPU to initial state
        /// </summary>
        public void Reset()
        {
            if (_cpuHandle == IntPtr.Zero) return;

            MIPS32_Reset(_cpuHandle);
            IsRunning = false;
            _outputBuffer.Clear();
            _inputBuffer.Clear();
            Log("CPU reset");
        }

        /// <summary>
        /// Load a program from a uint array
        /// </summary>
        public bool LoadProgram(uint[] program, uint address = 0)
        {
            if (_cpuHandle == IntPtr.Zero)
            {
                LogError("Emulator not initialized");
                return false;
            }

            if (address == 0) address = loadAddress;

            int result = MIPS32_LoadProgram(_cpuHandle, program, (uint)program.Length, address);
            if (result == 0)
            {
                LogError($"Failed to load program: {GetLastErrorString()}");
                return false;
            }

            Log($"Program loaded at 0x{address:X8}, size: {program.Length} words");
            return true;
        }

        /// <summary>
        /// Load a program from a TextAsset (hex values)
        /// </summary>
        public bool LoadProgramFromTextAsset(TextAsset asset)
        {
            if (asset == null)
            {
                LogError("Program asset is null");
                return false;
            }

            try
            {
                string[] lines = asset.text.Split('\n');
                List<uint> program = new List<uint>();

                foreach (string line in lines)
                {
                    string trimmed = line.Trim();
                    if (string.IsNullOrEmpty(trimmed) || trimmed.StartsWith("//"))
                        continue;

                    // Parse hex values (with or without 0x prefix)
                    if (trimmed.StartsWith("0x"))
                        trimmed = trimmed.Substring(2);

                    if (uint.TryParse(trimmed, System.Globalization.NumberStyles.HexNumber, null, out uint value))
                    {
                        program.Add(value);
                    }
                }

                return LoadProgram(program.ToArray());
            }
            catch (Exception e)
            {
                LogError($"Failed to parse program file: {e.Message}");
                return false;
            }
        }

        /// <summary>
        /// Start execution from current PC
        /// </summary>
        public void StartExecution()
        {
            if (_cpuHandle == IntPtr.Zero)
            {
                LogError("Emulator not initialized");
                return;
            }

            IsRunning = true;
            Log("Execution started");
        }

        /// <summary>
        /// Stop execution
        /// </summary>
        public void StopExecution()
        {
            IsRunning = false;
            if (_cpuHandle != IntPtr.Zero)
            {
                MIPS32_SetBreakFlag(_cpuHandle, 1);
            }
            Log("Execution stopped");
        }

        /// <summary>
        /// Execute a single instruction
        /// </summary>
        public bool Step()
        {
            if (_cpuHandle == IntPtr.Zero) return false;

            int result = MIPS32_Step(_cpuHandle);
            if (result == 0)
            {
                LogError($"Step failed: {GetLastErrorString()}");
                return false;
            }

            return true;
        }

        /// <summary>
        /// Run for specified number of cycles
        /// </summary>
        public bool RunCycles(uint cycles)
        {
            if (_cpuHandle == IntPtr.Zero) return false;

            int result = MIPS32_Run(_cpuHandle, cycles);
            if (result == 0 && !IsHalted)
            {
                LogError($"Execution failed: {GetLastErrorString()}");
                return false;
            }

            if (IsHalted)
            {
                IsRunning = false;
                OnHalted?.Invoke();
                Log("CPU halted");
            }

            return true;
        }

        /// <summary>
        /// Get register value by index
        /// </summary>
        public uint GetRegister(uint index)
        {
            if (_cpuHandle == IntPtr.Zero) return 0;
            return MIPS32_GetRegister(_cpuHandle, index);
        }

        /// <summary>
        /// Set register value by index
        /// </summary>
        public void SetRegister(uint index, uint value)
        {
            if (_cpuHandle == IntPtr.Zero) return;
            MIPS32_SetRegister(_cpuHandle, index, value);
        }

        /// <summary>
        /// Get register name by index
        /// </summary>
        public string GetRegisterName(uint index)
        {
            IntPtr namePtr = MIPS32_GetRegisterName(index);
            return Marshal.PtrToStringAnsi(namePtr);
        }

        /// <summary>
        /// Read memory at address
        /// </summary>
        public uint ReadMemory(uint address)
        {
            if (_cpuHandle == IntPtr.Zero) return 0;
            return MIPS32_ReadMemory(_cpuHandle, address);
        }

        /// <summary>
        /// Write byte to memory
        /// </summary>
        public void WriteMemory(uint address, byte data)
        {
            if (_cpuHandle == IntPtr.Zero) return;
            MIPS32_WriteMemory(_cpuHandle, address, data);
        }

        /// <summary>
        /// Send input data to the emulator
        /// </summary>
        public void SendInput(string text)
        {
            foreach (char c in text)
            {
                _inputBuffer.Enqueue((byte)c);
            }
        }

        /// <summary>
        /// Get output buffer contents
        /// </summary>
        public string GetOutput()
        {
            return string.Join("", _outputBuffer);
        }

        /// <summary>
        /// Clear output buffer
        /// </summary>
        public void ClearOutput()
        {
            _outputBuffer.Clear();
        }

        #endregion

        #region Private Methods

        private void OnOutputCallback(byte data)
        {
            char c = (char)data;
            _outputBuffer.Add(c.ToString());
            OnOutput?.Invoke(c.ToString());

            if (enableLogging)
            {
                Debug.Log($"Output: '{c}' (0x{data:X2})");
            }
        }

        private byte OnInputCallback()
        {
            if (_inputBuffer.Count > 0)
            {
                byte data = _inputBuffer.Dequeue();
                if (enableLogging)
                {
                    Debug.Log($"Input: '{(char)data}' (0x{data:X2})");
                }
                return data;
            }
            return 0; // No input available
        }

        private void OnWriteCallback(uint address, byte data)
        {
            OnMemoryWrite?.Invoke(address, data);

            if (enableLogging)
            {
                Debug.Log($"Memory Write: 0x{address:X8} = 0x{data:X2}");
            }
        }

        private string GetLastErrorString()
        {
            if (_cpuHandle == IntPtr.Zero) return "Emulator not initialized";

            ErrorCode error = MIPS32_GetLastError(_cpuHandle);
            IntPtr errorPtr = MIPS32_GetErrorString(error);
            return Marshal.PtrToStringAnsi(errorPtr);
        }

        private string GetVersion()
        {
            IntPtr versionPtr = MIPS32_GetVersion();
            return Marshal.PtrToStringAnsi(versionPtr);
        }

        private void Log(string message)
        {
            if (enableLogging)
            {
                Debug.Log($"[MIPS32] {message}");
            }
        }

        private void LogWarning(string message)
        {
            if (enableLogging)
            {
                Debug.LogWarning($"[MIPS32] {message}");
            }
        }

        private void LogError(string message)
        {
            Debug.LogError($"[MIPS32] {message}");
            OnError?.Invoke(MIPS32_GetLastError(_cpuHandle), message);
        }

        #endregion

        #region GUI Debug

#if UNITY_EDITOR
        void OnGUI()
        {
            if (!showRegisters || _cpuHandle == IntPtr.Zero) return;

            GUILayout.BeginArea(new Rect(10, 10, 300, 400));
            GUILayout.BeginVertical("box");
            
            GUILayout.Label("MIPS32 Emulator Debug", GUI.skin.label);
            
            GUILayout.Label($"PC: 0x{PC:X8}");
            GUILayout.Label($"Cycles: {CycleCount}");
            GUILayout.Label($"Status: {(IsRunning ? "Running" : "Stopped")} {(IsHalted ? "(Halted)" : "")}");
            
            GUILayout.Space(10);
            GUILayout.Label("Registers:");
            
            // Show first 8 registers
            for (uint i = 0; i < 8; i++)
            {
                uint value = GetRegister(i);
                GUILayout.Label($"{GetRegisterName(i)}: 0x{value:X8} ({value})");
            }
            
            GUILayout.Space(10);
            if (GUILayout.Button("Step"))
            {
                Step();
            }
            
            if (GUILayout.Button(IsRunning ? "Stop" : "Start"))
            {
                if (IsRunning)
                    StopExecution();
                else
                    StartExecution();
            }
            
            if (GUILayout.Button("Reset"))
            {
                Reset();
            }
            
            GUILayout.EndVertical();
            GUILayout.EndArea();
        }
#endif

        #endregion
    }
}
