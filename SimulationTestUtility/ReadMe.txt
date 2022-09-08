Simulation Regression Test Utility

The simulation regression test utility will open, simulate and report results on *.rbxl files
that have been configured as described below.  In order to be tested properly each world must have three objects
as children of the Workspace.

1. An IntValue that must be called "StopTime" with a value equal to the number of seconds of simulation time.
2. An IntValue called "Result" that is initially set to 0
3. A script that checks the test criteria when the simulation is complete.  The  following can be used as a template
   for creating this script.



****** Start Code Fence ***************************

-- Leave this loop here so the test condition
-- gets checked at the correct time

while time() < Workspace.StopTime.Value do
wait()
end

****** End Code Fence *****************************

-- Put your test conditions here
-- Make sure the bool success gets set 
-- Here is an example

success = false
success = Workspace.Part1.Velocity.magnitude < 0.02



****** Start Code Fence ***************************

-- Don't change this

if success
then
	Workspace.Result.Value = 1
else
	Workspace.Result.Value = -1
end

****** End Code Fence *****************************

Additional notes and requirements:

- If an *.rbxl file is not decorated with these objects it will run for 10 seconds.

- Currently the test utility runs all worlds in or below the directory where the test utility exe resides.

- Results are not streamed to file yet (only written to console).

- Must have RbxTestHooks.dll in the same directory as RobloxApp.exe

- The utility will attach to the currently running RobloxApp.exe.  It will not create it's own instance.

- It will not work with the installed version of Roblox.


To Do:
- Specify root directory dialog
- Stream results to file
- Provide hook from Roblox Tools menu