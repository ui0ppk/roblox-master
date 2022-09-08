using System;
using System.Runtime.InteropServices;
namespace Roblox.Test
{
	/// <summary>
	/// This class shall keep the User32 APIs being used in 
	/// our program.
	/// </summary>
	public class PlatformInvokeUSER32
	{

		#region Class Variables
		public  const int SM_CXSCREEN=0;
		public  const int SM_CYSCREEN=1;
        public  const uint WM_COMMAND = 0x0111;
        public const uint WM_ACTIVATE = 0x06;
        public const uint WA_ACTIVE = 1;

        /*
         * ShowWindow() Commands
         */
        public const int SW_HIDE = 0;
        public const int SW_SHOW = 5;
        /*#define SW_SHOWNORMAL       1
        #define SW_NORMAL           1
        #define SW_SHOWMINIMIZED    2
        #define SW_SHOWMAXIMIZED    3
        #define SW_MAXIMIZE         3
        #define SW_SHOWNOACTIVATE   4
        #define SW_MINIMIZE         6
        #define SW_SHOWMINNOACTIVE  7
        #define SW_SHOWNA           8
        #define SW_RESTORE          9
        #define SW_SHOWDEFAULT      10
        #define SW_FORCEMINIMIZE    11
        #define SW_MAX              11*/

        #endregion		
		
		#region Class Functions
		[DllImport("user32.dll", EntryPoint="GetDesktopWindow")]
		public static extern IntPtr GetDesktopWindow();

		[DllImport("user32.dll",EntryPoint="GetDC")]
		public static extern IntPtr GetDC(IntPtr ptr);

		[DllImport("user32.dll",EntryPoint="GetSystemMetrics")]
		public static extern int GetSystemMetrics(int abc);

		[DllImport("user32.dll",EntryPoint="GetWindowDC")]
		public static extern IntPtr GetWindowDC(Int32 ptr);

		[DllImport("user32.dll",EntryPoint="ReleaseDC")]
		public static extern IntPtr ReleaseDC(IntPtr hWnd,IntPtr hDc);


        [return: MarshalAs(UnmanagedType.Bool)]
        [DllImport("user32.dll", SetLastError = true)]
        public static extern bool PostMessage(HandleRef hWnd, uint Msg, IntPtr wParam,
           IntPtr lParam);

        [return: MarshalAs(UnmanagedType.Bool)]
        [DllImport("user32.dll", SetLastError = true)]
        public static extern bool ShowWindow(HandleRef hWnd, int nCmdShow);
		
		#endregion

		#region Public Constructor
		public PlatformInvokeUSER32()
		{
			// 
			// TODO: Add constructor logic here
			//
		}
		#endregion
	}

	//This structure shall be used to keep the size of the screen.
	public struct SIZE
	{
		public int cx;
		public int cy;
	}
}
