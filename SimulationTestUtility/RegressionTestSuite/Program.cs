using System;
using System.Diagnostics;
using System.IO;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using RobloxLib;

namespace RegressionTestSuite
{
    class Program
    {
        static void Main(string[] args)
        {
            try
            {
                Console.WriteLine("Starting Regression Testing App");
                // create a writer and open the file
                TextWriter tw = new StreamWriter("Results.txt");

                // write a line of text to the file
                tw.WriteLine("Simulation Testing Results " + System.DateTime.Now);
                tw.WriteLine("***************************************************");
                tw.WriteLine();


                string[] filePaths = Directory.GetFiles(Directory.GetCurrentDirectory(), "*.rbxl",
                                             SearchOption.AllDirectories);

                var app = new RobloxLib.AppClass();

                int numTests = filePaths.Length;
                int numRepeats = 10; // Run each file this many times
                int numFailures = 0;
                int numNotSetUp = 0;

                Stopwatch sw = new Stopwatch();
                Stopwatch fullSW = new Stopwatch();
                fullSW.Reset();
                fullSW.Start();

                for (int i = 0; i < numTests; i++)
                {
                    double actualSimTime = 0.0;
                    long testClockDelta = 0;
                  
                    Console.WriteLine();
                    Console.WriteLine(filePaths[i]);
                    tw.WriteLine();
                    tw.WriteLine(filePaths[i]);

                    string testFilePath = filePaths[i].Replace("\\", "\\\\");

                    for (int j = 0; j < numRepeats; j++)
                    {
                        var w = app.CreateGame("44340105256");
                        w.ExecScript("game:load('" + testFilePath + "')", null, null, null, null);

                        // If no stop time IntValue then run for 10 seconds
                        int runTime = 10;
                        var stopTimeExists = w.ExecScript("return game.Workspace:FindFirstChild(\"StopTime\") ~= nil", null, null, null, null);
                        if (Convert.ToBoolean(stopTimeExists[0]))
                        {
                            var res = w.ExecScript("return Workspace.StopTime.Value", null, null, null, null);

                            if (res.Length > 0)
                                runTime = Convert.ToInt16(res[0]);
                        }

                        sw.Reset();
                        sw.Start();
                        w.ExecScript("game:GetService(\"RunService\"):Run()", null, null, null, null);
                        System.Threading.Thread.Sleep(runTime * 1000 + 1000);
                        w.ExecScript("game:GetService(\"RunService\"):Pause()", null, null, null, null);
                        sw.Stop();
                        testClockDelta += sw.ElapsedMilliseconds;

                        // Test success conditions
                        var resultExists = w.ExecScript("return game.Workspace:FindFirstChild(\"Result\") ~= nil", null, null, null, null);
                        var resultActualSimTime = w.ExecScript("return time()", null, null, null, null);
                        actualSimTime += Convert.ToDouble(resultActualSimTime[0]);

                        // Intermediate results
                        tw.WriteLine(Convert.ToDouble(resultActualSimTime[0]) + "   " + sw.ElapsedMilliseconds / 1000.0);
                        /*
                        if (Convert.ToBoolean(resultExists[0]))
                        {
                            var testResult1 = w.ExecScript("return Workspace.Result.Value", null, null, null, null);
                            {
                                if (Convert.ToInt16(testResult1[0]) == 1)
                                    tw.WriteLine("Test Passed");
                                else if (Convert.ToInt16(testResult1[0]) == -1)
                                {
                                    numFailures++;
                                    tw.WriteLine("Test FAILED ********");
                                }
                                else
                                {
                                    numFailures++;
                                    tw.WriteLine("The test was not executed properly (i.e. Parts went out of scope.)");
                                }
                            }
                        }
                        else
                        {
                            numNotSetUp++;
                            tw.WriteLine("This file is not set up as a regression test.");
                        }
                        */

                        w.Close();
                    }
                    tw.WriteLine("Sim Run Time =                         " + actualSimTime / numRepeats);
                    tw.WriteLine("Stopwatch =                            " + (testClockDelta / 1000.0) / numRepeats);
                    tw.WriteLine("Simulation Time -to- Real Time Ratio = " + actualSimTime /(testClockDelta / 1000.0));
                    tw.WriteLine();
                }
                fullSW.Stop();
                tw.WriteLine();
                tw.WriteLine();
                tw.WriteLine();
                tw.WriteLine("***************");
                tw.WriteLine("Results Summary");
                tw.WriteLine("Simulation Repeats: " + numRepeats);
                tw.WriteLine("Cumulative Test Time: " + fullSW.ElapsedMilliseconds / 1000.0);
                tw.WriteLine("Number of Tests Executed: " + numTests);
                tw.WriteLine("Number of Successes: " + (numTests - numFailures - numNotSetUp));
                tw.WriteLine("Number Failing: " + numFailures);
                tw.WriteLine("Number Not Configured: " + numNotSetUp);
                Console.WriteLine("Done Testing");

                //if (numTests - numNotSetUp > 0)
                //    Console.WriteLine("Success Rate: " + 100 * (numTests - numFailures - numNotSetUp) / (numTests - numNotSetUp) + "%");
                //else
                //    Console.WriteLine("Success Rate: No tests were configured for regression.");

                // close the stream
                tw.Close();
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex.Message);
            }
        }
	}
}
