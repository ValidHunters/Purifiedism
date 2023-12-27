using System.Reflection;
using Serilog;

#pragma warning disable CS8603

namespace Matsuoka;
public class Seiko
{
    private static Assembly? RC = null;
    private static Assembly? RS = null;
    private static Assembly? CC = null;
    private static Assembly? CS = null;
    
    private static void Capture()
    {
        while (RC == null && RS == null && CC == null && CS == null)
        {
            Assembly[] assemblies = AppDomain.CurrentDomain.GetAssemblies();
            foreach(Assembly e in assemblies)
            {
                if (e.FullName == null) continue;
                
                if (e.FullName.Contains("Robust.Client,"))
                {
                    RC = e;
                }

                if (e.FullName.Contains("Robust.Shared,"))
                {
                    RS = e;
                }
                
                if (e.FullName.Contains("Content.Client,"))
                {
                    CC = e;
                }
                
                if (e.FullName.Contains("Content.Shared,"))
                {
                    CS = e;
                }
            }
            Thread.Sleep(125);
        }
        
        Console.WriteLine("BSOD Bypassed");
    }
    
    private static void Main()
    {
        Capture();
    }

    public static int Kickstart(IntPtr arg, int argLength)
    {
        Thread t = new Thread(new ThreadStart(Main));
        t.Start();

        return 0;
    }
    
}