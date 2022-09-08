#include "stdafx.h"

#include "Util/Color.h"
#include "Util/Hash.h"

/* 
	see http://web.media.mit.edu/~wad/color/palette.html
	
	This is an optimal 16 color palette

Black			RGB: 0, 0, 0 
Dk. Gray		RGB: 87, 87, 87 
Red				RGB: 173, 35, 35 
Blue			RGB: 42, 75, 215 
Green			RGB: 29, 105, 20 
Brown			RGB: 129, 74, 25 
Purple			RGB: 129, 38, 192 
Lt. Gray		RGB: 160, 160, 160 
Lt. Green		RGB: 129, 197, 122 
Lt. Blue		RGB: 157, 175, 255 
Cyan			RGB: 41, 208, 208 
Orange			RGB: 255, 146, 51 
Yellow			RGB: 255, 238, 51 
Tan				RGB: 233, 222, 187 
Pink			RGB: 255, 205, 243 
White			RGB: 255, 255, 255 

*/



namespace RBX {

const G3D::Color3& Color::getColorByIndex(int i)
{
	static bool init = false;
	static Color colors[16];
	if (!init)
	{
		colors[0] = Color(0, 0, 0);
		colors[1] = Color(87,87,87);
		colors[2] = Color(173,35,35);
		colors[3] = Color(42,75,215);
		colors[4] = Color(29,105,20);
		colors[5] = Color(129,74,25);
		colors[6] = Color(129,38,192);
		colors[7] = Color(160,160,160);
		colors[8] = Color(129,197,22);
		colors[9] = Color(157,175,255);
		colors[10] = Color(41,208,208);
		colors[11] = Color(255,146,51);
		colors[12] = Color(255,238,51);
		colors[13] = Color(233,222,187);
		colors[14] = Color(255,205,243);
		colors[15] = Color(255,255,255);
		init = true;
	}
	int safeIndex = i % 16;
	return colors[safeIndex].color3();
}

const G3D::Color3& Color::colorFromIndex8(int index)
{
	switch (index)
	{
	case 0:		return Color::red();
	case 1:		return Color::blue();
	case 2:		return Color::green();
	case 3:		return Color::purple();
	case 4:		return Color::orange();
	case 5:		return Color::yellow();
	case 6:		return Color::pink();
	case 7:
	default:	return Color::tan();
	}
}

const G3D::Color3 Color::colorFromInt(unsigned int i)
{
	unsigned int colorId1 = (i % 15);
	unsigned int colorId2 = (i % 13) + 3;

	G3D::Color3 c1 = getColorByIndex(colorId1);
	G3D::Color3 c2 = getColorByIndex(colorId2);
	return c1.lerp(c2, 0.5f);
}

const G3D::Color3 Color::colorFromString(const std::string& s)
{
	if (s.length() < 8) {
		return G3D::Color3::white();
	}
	else {
		unsigned int i = Hash::hash(s);
		return colorFromInt(i);
	}
}

const G3D::Color3 Color::colorFromPointer(void* pointer)
{
	unsigned long temp = reinterpret_cast<uintptr_t>(pointer);
	return colorFromInt(static_cast<int>(temp));
}

const G3D::Color3 Color::colorFromTemperature(float value)	// 0.0 == cold, 1.0 == hot;
{
	float clamped = G3D::clamp(value, 0.0f, 1.0f) * 6.0f;
	if (clamped < 1.0)	{return Color3::black().lerp(		Color3::blue(),			clamped);		}
	if (clamped < 2.0)	{return Color3::blue().lerp(		Color3::purple(),		clamped-1);		}
	if (clamped < 3.0)	{return Color3::purple().lerp(		Color3::red(),			clamped-2);		}
	if (clamped < 4.0)	{return Color3::red().lerp(			Color3::orange(),		clamped-3);		}
	if (clamped < 5.0)	{return Color3::orange().lerp(		Color3::yellow(),		clamped-4);		}
	else				{return Color3::yellow().lerp(		Color3::white(),		clamped-5);		}

}

const G3D::Color3 Color::colorFromError(double value)	// 0.0 == very important, 10.0 == not important;
{
	//float clamped = G3D::clamp(value, 0.0, 10.0);
	//if (clamped <= 1.0)	{return Color3::black().lerp(		Color3::blue(),			clamped);		}
	//if (clamped <= 3.0)	{return Color3::blue().lerp(		Color3::purple(),		clamped - 1);		}
	//if (clamped <= 5.0)	{return Color3::purple().lerp(		Color3::red(),			clamped - 3);		}
	//if (clamped <= 7.0)	{return Color3::red().lerp(			Color3::orange(),		clamped - 5);		}
	//if (clamped <= 9.0)	{return Color3::orange().lerp(		Color3::yellow(),		clamped - 7);		}
	//else				{return Color3::yellow().lerp(		Color3::white(),		clamped - 9);		}

	return Color3::rainbowColorMap(value / 10);

}

}	// namespace
