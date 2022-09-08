#pragma once

#include "Util/G3DCore.h"

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

	class Color {
	private:
		G3D::Color3 rgb;
		Color() {}
		Color(unsigned char r, unsigned char g, unsigned char b) : rgb(static_cast<float>(r)/255.0f, static_cast<float>(g)/255.0f, static_cast<float>(b)/255.0f) {}
		const G3D::Color3& color3() {return rgb;}
	public:
		static const G3D::Color3& getColorByIndex(int i);

		inline static const G3D::Color3& black()		{return getColorByIndex(0);}
		inline static const G3D::Color3& darkGray()		{return getColorByIndex(1);}
		inline static const G3D::Color3& red()			{return getColorByIndex(2);}
		inline static const G3D::Color3& blue()			{return getColorByIndex(3);}
		inline static const G3D::Color3& green()		{return getColorByIndex(4);}
		inline static const G3D::Color3& brown()		{return getColorByIndex(5);}
		inline static const G3D::Color3& purple()		{return getColorByIndex(6);}
		inline static const G3D::Color3& lightGray()	{return getColorByIndex(7);}
		inline static const G3D::Color3& lightGreen()	{return getColorByIndex(8);}
		inline static const G3D::Color3& lightBlue()	{return getColorByIndex(9);}
		inline static const G3D::Color3& cyan()			{return getColorByIndex(10);}
		inline static const G3D::Color3& orange()		{return getColorByIndex(11);}
		inline static const G3D::Color3& yellow()		{return getColorByIndex(12);}
		inline static const G3D::Color3& tan()			{return getColorByIndex(13);}
		inline static const G3D::Color3& pink()			{return getColorByIndex(14);}
		inline static const G3D::Color3& white()		{return getColorByIndex(15);}

		static const G3D::Color3& colorFromIndex8(int index);

		static const G3D::Color3 colorFromInt(unsigned int i);

		static const G3D::Color3 colorFromString(const std::string& s);

		static const G3D::Color3 colorFromPointer(void* pointer);

		static const G3D::Color3 colorFromTemperature(float temperature);	// 0.0 = cold, 1.0 = hot

		static const G3D::Color3 colorFromError(double value);	// 0.0 == not important, 10.0 == very important;
	};
}
