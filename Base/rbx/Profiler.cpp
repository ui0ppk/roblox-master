#include "rbx/Profiler.h"

#include "rbx/Debug.h"

FASTFLAGVARIABLE(OnScreenProfiler, false)

#ifdef RBXPROFILER
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4244 4995 4996)
#endif

#define NOMINMAX

#ifdef __APPLE__
#include <boost/thread.hpp>
#include <boost/function.hpp>

namespace std
{
    using boost::thread;
    using boost::recursive_mutex;
    using boost::lock_guard;
    using boost::function;

    enum memory_order
    {
        memory_order_relaxed,
        memory_order_consume,
        memory_order_acquire,
        memory_order_release,
        memory_order_acq_rel,
        memory_order_seq_cst
    };

    template <typename T>
    class atomic
    {
    public:
        atomic()
        {
        }

        T load(memory_order order = memory_order_seq_cst) const
        {
            T result = value;
            if (order != memory_order_relaxed) __sync_synchronize();
            return result;
        }

        void store(T desired, memory_order order = memory_order_seq_cst)
        {
            if (order != memory_order_relaxed) __sync_synchronize();
            value = desired;
        }

        T fetch_add(T arg, memory_order order = std::memory_order_seq_cst)
        {
            return __sync_fetch_and_add(&value, arg);
        }

    private:
        template <int N> struct __attribute__((aligned(N))) pad_t {};

        union { volatile T value; pad_t<sizeof(T)> pad; };
    };
}

#define MICROPROFILE_NOCXX11
#endif

#ifdef __ANDROID__
#include <android/log.h>

#define MICROPROFILE_PRINTF(...) __android_log_print(ANDROID_LOG_DEBUG, "MicroProfile", __VA_ARGS__)
#endif

#ifdef _WIN32
static void MicroProfileDebugPrintf(const char* format, ...)
{
    char message[256];

    va_list args;
    va_start(args, format);
    vsprintf_s(message, format, args);
    va_end(args);

    OutputDebugStringA(message);
}

#define MICROPROFILE_PRINTF(...) MicroProfileDebugPrintf(__VA_ARGS__)
#endif

#define MP_ASSERT(e) RBXASSERT(e)
#define MICROPROFILE_WEBSERVER 0

#ifdef RBX_PLATFORM_DURANGO
#define MICROPROFILE_WEBSERVER_PORT 4600
#define MICROPROFILE_CONTEXT_SWITCH_TRACE 0
#define getenv(name) NULL
#endif

#if defined(_WIN32)
#define MICROPROFILE_GPU_TIMERS_D3D11 1
#elif defined(__APPLE__) && !defined(RBX_PLATFORM_IOS)
#include <OpenGL/gl3.h>
#define MICROPROFILE_GPU_TIMERS_GL 0
#endif

#if defined(RBX_PLATFORM_DURANGO)
#include <d3d11_x.h>
#endif

#define MICROPROFILE_IMPL
#include "microprofile/microprofile.h"

#define MICROPROFILE_DRAWCURSOR 1

#define MICROPROFILEUI_IMPL
#include "microprofile/microprofileui.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

struct InputState
{
	int mouseX, mouseY, mouseWheel;
	bool mouseButton[4];
};

RBX::Profiler::Renderer* gProfileRenderer = 0;

InputState gInputState;

void MicroProfileDrawText(int nX, int nY, uint32_t nColor, const char* pText, uint32_t nNumCharacters)
{
	gProfileRenderer->drawText(nX, nY, nColor, pText, nNumCharacters, MICROPROFILE_TEXT_WIDTH, MICROPROFILE_TEXT_HEIGHT);
}

void MicroProfileDrawBox(int nX, int nY, int nX1, int nY1, uint32_t nColor, MicroProfileBoxType type)
{
	if (type == MicroProfileBoxTypeBar)
	{
		uint32_t r = 0xff & (nColor>>16);
		uint32_t g = 0xff & (nColor>>8);
		uint32_t b = 0xff & nColor;

		uint32_t nMax = MicroProfileMax(MicroProfileMax(MicroProfileMax(r, g), b), 30u);
		uint32_t nMin = MicroProfileMin(MicroProfileMin(MicroProfileMin(r, g), b), 180u);

		uint32_t r0 = 0xff & ((r + nMax)/2);
		uint32_t g0 = 0xff & ((g + nMax)/2);
		uint32_t b0 = 0xff & ((b + nMax)/2);

		uint32_t r1 = 0xff & ((r+nMin)/2);
		uint32_t g1 = 0xff & ((g+nMin)/2);
		uint32_t b1 = 0xff & ((b+nMin)/2);

		uint32_t nColor0 = (r0<<16)|(g0<<8)|(b0<<0)|(0xff000000&nColor);
		uint32_t nColor1 = (r1<<16)|(g1<<8)|(b1<<0)|(0xff000000&nColor);

		gProfileRenderer->drawBox(nX, nY, nX1, nY1, nColor0, nColor1);
	}
	else
	{
		gProfileRenderer->drawBox(nX, nY, nX1, nY1, nColor, nColor);
	}
}

void MicroProfileDrawLine2D(uint32_t nVertices, float* pVertices, uint32_t nColor)
{
	gProfileRenderer->drawLine(nVertices, const_cast<const float*>(pVertices), nColor);
}

namespace RBX
{
	namespace Profiler
	{
		MicroProfileTokenType getTokenType(const char* group)
		{
			return strcmp(group, "GPU") == 0 ? MicroProfileTokenTypeGpu : MicroProfileTokenTypeCpu;
		}

		Token getToken(const char* group, const char* name, int color)
		{
			return MicroProfileGetToken(group, name, color, getTokenType(group));
		}

		Token getLabelToken(const char* group)
		{
			return MicroProfileGetLabelToken(group, getTokenType(group));
		}

		Token getCounterToken(const char* name)
		{
			return MicroProfileGetCounterToken(name);
		}

		uint64_t enterRegion(Token token)
		{
			if (!FFlag::OnScreenProfiler) return MICROPROFILE_INVALID_TICK;

			return MicroProfileEnter(token);
		}

		void addLabel(Token token, const char* name)
		{
			if (!FFlag::OnScreenProfiler) return;

			MicroProfileLabel(token, name);
		}

		void addLabelFormat(Token token, const char* name, ...)
		{
			if (!FFlag::OnScreenProfiler) return;

			va_list args;
			va_start(args, name);
			MicroProfileLabelFormatV(token, name, args);
			va_end(args);
		}

		void leaveRegion(Token token, uint64_t enterTimestamp)
		{
			if (!FFlag::OnScreenProfiler) return;

			MicroProfileLeave(token, enterTimestamp);
		}

        void counterAdd(Token token, long long count)
        {
			if (!FFlag::OnScreenProfiler) return;
            
            MicroProfileCounterAdd(token, count);
        }
        
        void counterSet(Token token, long long count)
        {
			if (!FFlag::OnScreenProfiler) return;
            
            MicroProfileCounterSet(token, count);
        }

		void onThreadCreate(const char* name)
		{
			if (!FFlag::OnScreenProfiler) return;

			MicroProfileOnThreadCreate(name);
		}

		void onThreadExit()
		{
			if (!FFlag::OnScreenProfiler) return;

			MicroProfileOnThreadExit();
		}

		void onFrame()
		{
			if (!FFlag::OnScreenProfiler) return;

            if (g_MicroProfile.nWebServerDataSent)
            {
                // Set the defaults for web serving after the first connect
                MicroProfileSetEnableAllGroups(true);
            }

			MicroProfileFlip();
		}

		void gpuInit(void* context)
		{
			if (!FFlag::OnScreenProfiler) return;

			MicroProfileGpuInit(context);
		}

		void gpuShutdown()
		{
			if (!FFlag::OnScreenProfiler) return;

			MicroProfileGpuShutdown();
		}

		bool isCapturingMouseInput()
		{
			if (!FFlag::OnScreenProfiler) return false;

			return (MicroProfileIsDrawing() && !g_MicroProfile.nRunning);
		}

		bool handleMouse(unsigned int flags, int mouseX, int mouseY, int mouseWheel, int mouseButton)
		{
			if (!FFlag::OnScreenProfiler) return false;

			if (flags & Flag_MouseMove)
			{
				gInputState.mouseX = mouseX;
				gInputState.mouseY = mouseY;
			}

			if (flags & Flag_MouseWheel)
				gInputState.mouseWheel = mouseWheel;

			if (flags & Flag_MouseDown)
				gInputState.mouseButton[mouseButton] = true;

			if (flags & Flag_MouseUp)
				gInputState.mouseButton[mouseButton] = false;


			MicroProfileMousePosition(std::max(gInputState.mouseX, 0), std::max(gInputState.mouseY, 0), gInputState.mouseWheel);
			MicroProfileMouseButton(gInputState.mouseButton[0], gInputState.mouseButton[1]);

			return isCapturingMouseInput();
		}

		bool toggleVisible()
		{
			if (!FFlag::OnScreenProfiler) return false;

			if (MicroProfileIsDrawing())
			{
				MicroProfileSetDisplayMode(MP_DRAW_OFF);

				if (!g_MicroProfile.nRunning)
					g_MicroProfile.nToggleRunning = 1;
			}
			else
			{
				MicroProfileSetDisplayMode(MP_DRAW_FRAME);
			}

			return true;
		}

		bool togglePause()
		{
			if (!FFlag::OnScreenProfiler) return false;

			if (MicroProfileIsDrawing())
			{
				if (g_MicroProfile.nRunning && g_MicroProfile.nDisplay == MP_DRAW_FRAME)
					MicroProfileSetDisplayMode(MP_DRAW_DETAILED);
				else if (!g_MicroProfile.nRunning && g_MicroProfile.nDisplay == MP_DRAW_DETAILED)
					MicroProfileSetDisplayMode(MP_DRAW_FRAME);

				MicroProfileTogglePause();

				return true;
			}

			return false;
		}

		bool isVisible()
		{
			if (!FFlag::OnScreenProfiler) return false;

			return MicroProfileIsDrawing();
		}

		void render(Renderer* renderer, unsigned int width, unsigned int height)
		{
			if (!FFlag::OnScreenProfiler) return;

			static bool initialized = false;

			if (!initialized)
			{
				MicroProfileInitUI();

				g_MicroProfileUI.nOpacityBackground = 0x40<<24;
			}

			gProfileRenderer = renderer;
			MicroProfileDraw(width, height);
			gProfileRenderer = 0;

			if (!initialized)
			{
				// MicroProfileDraw loads the preset; after it has loaded it we can check if we need to set up the defaults to enable all groups
				if (g_MicroProfile.nActiveGroupWanted == 0)
					MicroProfileSetEnableAllGroups(true);

				initialized = true;
			}

			gInputState.mouseWheel = 0;
		}
	}
}
#else
namespace RBX
{
	namespace Profiler
	{
		Token getToken(const char* group, const char* name, int color)
		{
			return 0;
		}

		uint64_t enterRegion(Token token)
		{
			return 0;
		}

		void addLabel(Token token, const char* name)
		{
		}

		void leaveRegion(Token token, uint64_t enterTimestamp)
		{
		}

		void onThreadCreate(const char* name)
		{
		}

		void onThreadExit()
		{
		}

		void onFrame()
		{
		}

		void gpuInit(void* context)
		{
		}

		void gpuShutdown()
		{
		}

		bool isCapturingMouseInput()
		{
			return false;
		}

		bool handleMouse(unsigned int flags, int mouseX, int mouseY, int mouseWheel, int mouseButton)
		{
			return false;
		}

		bool toggleVisible()
		{
			return false;
		}

		bool togglePause()
		{
			return false;
		}

		bool isVisible()
		{
			return false;
		}

		void render(Renderer* renderer, unsigned int width, unsigned int height)
		{
		}
	}
}
#endif
