using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;

internal static class Setup
{
    private static int Main()
    {
        string target = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System), "CatppuccinSaver.scr");
        Assembly assembly = Assembly.GetExecutingAssembly();
        string resourceName = assembly.GetManifestResourceNames().First(name => name.EndsWith("CatppuccinSaver.scr", StringComparison.OrdinalIgnoreCase));
        using (Stream input = assembly.GetManifestResourceStream(resourceName))
        using (FileStream output = File.Create(target))
        {
            input.CopyTo(output);
        }

        Process.Start(new ProcessStartInfo
        {
            FileName = "control.exe",
            Arguments = "desk.cpl,,@screensaver",
            UseShellExecute = true
        });
        return 0;
    }
}
