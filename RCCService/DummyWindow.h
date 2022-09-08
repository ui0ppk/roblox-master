#pragma once

class DummyWindow
{
public:
	HWND handle;
	DummyWindow(int width, int height);
	~DummyWindow(void);
};
