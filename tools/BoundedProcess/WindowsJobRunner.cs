// Win32 job confinement for diagnostic tools. Not a filesystem/security sandbox.
using System;
using System.ComponentModel;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;

namespace Voyage.Tools {
    public sealed class BoundedResult {
        public uint ProcessId;
        public uint ExitCode;
        public bool TimedOut;
        public long ElapsedMilliseconds;
        public ulong PeakJobMemoryBytes;
    }

    public static class WindowsJobRunner {
        const uint Suspended = 4, NoWindow = 0x08000000, ExtendedStartup = 0x00080000;
        const uint StartUseStdHandles = 0x100, KillOnClose = 0x2000, JobMemory = 0x200, ProcessMemory = 0x100;
        const uint WaitTimeout = 258, TimeoutExit = 1460;
        const int ExtendedLimits = 9;
        static readonly IntPtr InvalidHandle = new IntPtr(-1);

        [StructLayout(LayoutKind.Sequential)] struct Security {
            public int Length; public IntPtr Descriptor; public int Inherit;
        }
        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)] struct Startup {
            public int Size; public string Reserved, Desktop, Title;
            public uint X, Y, XSize, YSize, XCount, YCount, Fill, Flags;
            public ushort Show, ReservedSize; public IntPtr ReservedBytes, Input, Output, Error;
        }
        [StructLayout(LayoutKind.Sequential)] struct StartupEx { public Startup Startup; public IntPtr Attributes; }
        [StructLayout(LayoutKind.Sequential)] struct ProcessInfo { public IntPtr Process, Thread; public uint Id, ThreadId; }
        [StructLayout(LayoutKind.Sequential)] struct BasicLimits {
            public long ProcessTime, JobTime; public uint Flags;
            public UIntPtr MinWorkingSet, MaxWorkingSet; public uint ActiveLimit;
            public UIntPtr Affinity; public uint Priority, Scheduling;
        }
        [StructLayout(LayoutKind.Sequential)] struct IoCounters { public ulong Read, Write, Other, ReadBytes, WriteBytes, OtherBytes; }
        [StructLayout(LayoutKind.Sequential)] struct Limits {
            public BasicLimits Basic; public IoCounters Io;
            public UIntPtr ProcessMemory, JobMemory, PeakProcessMemory, PeakJobMemory;
        }
        [DllImport("kernel32.dll", SetLastError=true, CharSet=CharSet.Unicode)] static extern IntPtr CreateJobObject(IntPtr security, string name);
        [DllImport("kernel32.dll", SetLastError=true)] static extern bool SetInformationJobObject(IntPtr job, int type, ref Limits value, uint size);
        [DllImport("kernel32.dll", SetLastError=true)] static extern bool QueryInformationJobObject(IntPtr job, int type, out Limits value, uint size, IntPtr returned);
        [DllImport("kernel32.dll", SetLastError=true)] static extern bool AssignProcessToJobObject(IntPtr job, IntPtr process);
        [DllImport("kernel32.dll", SetLastError=true)] static extern bool TerminateJobObject(IntPtr job, uint code);
        [DllImport("kernel32.dll", SetLastError=true)] static extern bool TerminateProcess(IntPtr process, uint code);
        [DllImport("kernel32.dll", SetLastError=true)] static extern uint ResumeThread(IntPtr thread);
        [DllImport("kernel32.dll", SetLastError=true)] static extern uint WaitForSingleObject(IntPtr handle, uint milliseconds);
        [DllImport("kernel32.dll", SetLastError=true)] static extern bool GetExitCodeProcess(IntPtr process, out uint code);
        [DllImport("kernel32.dll", SetLastError=true)] static extern bool CloseHandle(IntPtr handle);
        [DllImport("kernel32.dll", SetLastError=true, CharSet=CharSet.Unicode)] static extern IntPtr CreateFile(string path, uint access, uint sharing, ref Security security, uint creation, uint flags, IntPtr template);
        [DllImport("kernel32.dll", SetLastError=true)] static extern bool InitializeProcThreadAttributeList(IntPtr list, int count, uint flags, ref IntPtr size);
        [DllImport("kernel32.dll", SetLastError=true)] static extern bool UpdateProcThreadAttribute(IntPtr list, uint flags, IntPtr attribute, IntPtr value, IntPtr size, IntPtr previous, IntPtr returned);
        [DllImport("kernel32.dll")] static extern void DeleteProcThreadAttributeList(IntPtr list);
        [DllImport("kernel32.dll", SetLastError=true, CharSet=CharSet.Unicode)] static extern bool CreateProcess(string application, StringBuilder command, IntPtr processSecurity, IntPtr threadSecurity, bool inherit, uint flags, IntPtr environment, string directory, ref StartupEx startup, out ProcessInfo process);

        static void Check(bool ok, string operation) {
            if (!ok) throw new Win32Exception(Marshal.GetLastWin32Error(), operation);
        }
        static void Close(IntPtr handle) { if (handle != IntPtr.Zero && handle != InvalidHandle) CloseHandle(handle); }
        // Windows CRT argument encoding. Never invoke a command shell implicitly.
        static string Quote(string value) {
            if (value == null || value.IndexOf('\0') >= 0) throw new ArgumentException("Null argument or embedded NUL");
            var s = new StringBuilder("\""); int slashes = 0;
            foreach (char ch in value) {
                if (ch == '\\') { slashes++; continue; }
                s.Append('\\', ch == '"' ? slashes * 2 + 1 : slashes); slashes = 0; s.Append(ch);
            }
            s.Append('\\', slashes * 2); s.Append('"'); return s.ToString();
        }

        public static BoundedResult Run(string executable, string[] arguments, string directory,
            string stdout, string stderr, ulong memoryBytes, uint timeoutSeconds) {
            if (IntPtr.Size != 8) throw new PlatformNotSupportedException("Use 64-bit PowerShell.");
            if (memoryBytes == 0 || timeoutSeconds == 0) throw new ArgumentException("Positive limits required.");
            var command = new StringBuilder(Quote(executable));
            foreach (string arg in arguments) command.Append(' ').Append(Quote(arg));
            IntPtr job = IntPtr.Zero, output = IntPtr.Zero, error = IntPtr.Zero, input = IntPtr.Zero;
            IntPtr attributes = IntPtr.Zero, handles = IntPtr.Zero;
            bool attributesReady = false, finished = false;
            var process = new ProcessInfo(); var clock = Stopwatch.StartNew();
            try {
                job = CreateJobObject(IntPtr.Zero, null); Check(job != IntPtr.Zero, "CreateJobObject");
                var limits = new Limits(); limits.Basic.Flags = KillOnClose | JobMemory | ProcessMemory;
                limits.JobMemory = new UIntPtr(memoryBytes);
                limits.ProcessMemory = new UIntPtr(memoryBytes);
                Check(SetInformationJobObject(job, ExtendedLimits, ref limits, (uint)Marshal.SizeOf(typeof(Limits))), "Set job memory limit");
                var security = new Security { Length = Marshal.SizeOf(typeof(Security)), Inherit = 1 };
                output = CreateFile(stdout, 0x40000000, 1, ref security, 1, 0x80, IntPtr.Zero); Check(output != InvalidHandle, "Create stdout");
                error = CreateFile(stderr, 0x40000000, 1, ref security, 1, 0x80, IntPtr.Zero); Check(error != InvalidHandle, "Create stderr");
                input = CreateFile("NUL", 0x80000000, 3, ref security, 3, 0x80, IntPtr.Zero); Check(input != InvalidHandle, "Open NUL stdin");
                IntPtr bytes = IntPtr.Zero;
                InitializeProcThreadAttributeList(IntPtr.Zero, 1, 0, ref bytes);
                attributes = Marshal.AllocHGlobal(bytes);
                Check(InitializeProcThreadAttributeList(attributes, 1, 0, ref bytes), "Initialize handle list"); attributesReady = true;
                handles = Marshal.AllocHGlobal(IntPtr.Size * 3);
                Marshal.WriteIntPtr(handles, 0, input); Marshal.WriteIntPtr(handles, IntPtr.Size, output); Marshal.WriteIntPtr(handles, IntPtr.Size * 2, error);
                Check(UpdateProcThreadAttribute(attributes, 0, new IntPtr(0x20002), handles, new IntPtr(IntPtr.Size * 3), IntPtr.Zero, IntPtr.Zero), "Restrict inherited handles");
                var startup = new StartupEx(); startup.Startup.Size = Marshal.SizeOf(typeof(StartupEx));
                startup.Startup.Flags = StartUseStdHandles; startup.Startup.Input = input; startup.Startup.Output = output; startup.Startup.Error = error; startup.Attributes = attributes;
                Check(CreateProcess(executable, command, IntPtr.Zero, IntPtr.Zero, true, Suspended | NoWindow | ExtendedStartup,
                    IntPtr.Zero, directory, ref startup, out process), "Create suspended tool");
                Check(AssignProcessToJobObject(job, process.Process), "Assign job before executing tool");
                Check(ResumeThread(process.Thread) != uint.MaxValue, "Resume tool");
                bool timedOut = false;
                while (true) {
                    uint wait = WaitForSingleObject(process.Process, 100);
                    if (wait == 0) break;
                    Check(wait == WaitTimeout, "Wait for tool");
                    if (clock.Elapsed.TotalSeconds >= timeoutSeconds) { timedOut = true; break; }
                }
                if (timedOut) {
                    Check(TerminateJobObject(job, TimeoutExit), "Terminate timed-out job");
                    Check(WaitForSingleObject(process.Process, 5000) == 0, "Confirm tool termination");
                }
                uint exit; Check(GetExitCodeProcess(process.Process, out exit), "Get tool exit code");
                Limits finalLimits;
                Check(QueryInformationJobObject(job, ExtendedLimits, out finalLimits, (uint)Marshal.SizeOf(typeof(Limits)), IntPtr.Zero), "Read peak job memory");
                // A CLI invocation cannot leave background descendants running after its root returns.
                Check(TerminateJobObject(job, TimeoutExit), "Terminate remaining descendants");
                finished = true;
                return new BoundedResult { ProcessId = process.Id, ExitCode = exit, TimedOut = timedOut,
                    ElapsedMilliseconds = clock.ElapsedMilliseconds, PeakJobMemoryBytes = finalLimits.PeakJobMemory.ToUInt64() };
            }
            finally {
                if (!finished && process.Process != IntPtr.Zero) TerminateProcess(process.Process, TimeoutExit);
                Close(job); // Kill-on-close also covers descendants on exceptions/host termination.
                Close(process.Thread); Close(process.Process); Close(input); Close(output); Close(error);
                if (attributesReady) DeleteProcThreadAttributeList(attributes);
                if (attributes != IntPtr.Zero) Marshal.FreeHGlobal(attributes);
                if (handles != IntPtr.Zero) Marshal.FreeHGlobal(handles);
            }
        }
    }
}
