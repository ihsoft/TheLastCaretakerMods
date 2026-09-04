// Synthetic regression executable; built only by Test-VoyageBoundedTool.ps1.
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Threading;
public static class RunnerProbe {
    [DllImport("kernel32.dll", SetLastError=true)] static extern IntPtr VirtualAlloc(IntPtr address, UIntPtr size, uint allocation, uint protection);
    public static int Main(string[] args) {
        if (args[0] == "echo") {
            for (int i = 1; i < args.Length; i++) Console.WriteLine(Convert.ToBase64String(System.Text.Encoding.UTF8.GetBytes(args[i])));
            Console.Error.WriteLine("probe-stderr"); return 0;
        }
        if (args[0] == "fail") return 7;
        if (args[0] == "hold") { Thread.Sleep(10000); return 0; }
        if (args[0] == "occupy") {
            var block = VirtualAlloc(IntPtr.Zero, new UIntPtr(64 * 1024 * 1024), 0x3000, 4);
            if (block == IntPtr.Zero) return 93;
            Marshal.WriteByte(block, 1); Console.WriteLine("ready"); Console.Out.Flush();
            Thread.Sleep(10000); return 0;
        }
        if (args[0] == "aggregate") {
            var child = Process.Start(new ProcessStartInfo {
                FileName = System.Reflection.Assembly.GetExecutingAssembly().Location,
                Arguments = "occupy", UseShellExecute = false, CreateNoWindow = true, RedirectStandardOutput = true
            });
            if (child.StandardOutput.ReadLine() != "ready") return 94;
            // Each allocation separately fits 128 MiB; together the job must reject it.
            var block = VirtualAlloc(IntPtr.Zero, new UIntPtr(64 * 1024 * 1024), 0x3000, 4);
            Console.WriteLine(child.Id); Console.Out.Flush();
            return block == IntPtr.Zero ? 91 : 92;
        }
        if (args[0] == "child") {
            var child = Process.Start(new ProcessStartInfo {
                FileName = System.Reflection.Assembly.GetExecutingAssembly().Location,
                Arguments = "hold", UseShellExecute = false, CreateNoWindow = true
            });
            Console.WriteLine(child.Id); Console.Out.Flush();
            if (args.Length > 1) Thread.Sleep(10000);
            return 0;
        }
        if (args[0] == "memory") {
            // Request at most 512 MiB even if job confinement is broken.
            for (int i = 0; i < 32; i++) {
                var block = VirtualAlloc(IntPtr.Zero, new UIntPtr(16 * 1024 * 1024), 0x3000, 4);
                if (block == IntPtr.Zero) { Console.WriteLine("allocation-denied"); return 91; }
                Marshal.WriteByte(block, 1);
            }
            Console.WriteLine("limit-not-enforced"); return 92;
        }
        return 3;
    }
}
