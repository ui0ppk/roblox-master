#include "StdAfx.h"
#include "DummyWindow.h"



WNDCLASS DefineMyClass(WNDPROC winProc, LPCTSTR className, HINSTANCE hInst) {
   WNDCLASS wndClass;

   wndClass.style         = 0;         // Windows Style (future article)
   wndClass.lpfnWndProc   = winProc;         // The Window Procedure
   wndClass.cbClsExtra    = 0;         // Extra memory for this class
   wndClass.cbWndExtra    = 0;         // Extra memory for this window
   wndClass.hInstance     = hInst;         // The definition of the instance of the application
   wndClass.hIcon         = 0;         // The icon in the upper left corner
   wndClass.hCursor       = 0;//::LoadCursor(0, IDC_ARROW);   // The cursor for this window
   wndClass.hbrBackground = 0;//(HBRUSH)(COLOR_WINDOW + 1);   // The background color for this window
   wndClass.lpszMenuName  = 0;         // The name of the class for the menu for this window
   wndClass.lpszClassName = className;      // The name of this class

   return wndClass;
}


HWND CreateMyWindow(LPCTSTR caption, LPCTSTR className, HINSTANCE hInstance, int width, int height) {
   return ::CreateWindow(
      className,      // Name of the registered window class
      caption,      // Window Caption to appear at the top of the frame
      WS_DISABLED ,   // The style of window to produce (overlapped is standard window for desktop)
      CW_USEDEFAULT,   // x position of the window
      CW_USEDEFAULT,   // y position of the window
      width,   // width of the window
      height,   // height of the window
      0,      // The handle to the parent frame (we will use this later to simulate an owned window)
      0,      // The handle to the menu for this window (we will learn to create menus in a later article)
      hInstance,    // the application instance
      0);      // window creation data
}

LRESULT CALLBACK WindowProcedure(HWND hwnd, unsigned int message, WPARAM wParam, LPARAM lParam) {
   switch(message) {
      case WM_DESTROY:
         ::PostQuitMessage(0);
         return 0;
   }
   
   return ::DefWindowProc(hwnd, message, wParam, lParam);
}

DummyWindow::DummyWindow(int width, int height) 
: handle(0)
{
	// create a dummy window.
	HINSTANCE hInst = GetModuleHandle (0);
	TCHAR className[] = _T("DummyWindow");

	WNDCLASS wndClass = DefineMyClass(WindowProcedure, className, hInst);
	::RegisterClass(&wndClass);

	handle = ::CreateMyWindow(_T(""), className, hInst, width, height);
}

DummyWindow::~DummyWindow(void)
{
	::DestroyWindow(handle);
}