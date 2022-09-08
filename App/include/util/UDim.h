/* Copyright 2003-2009 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Util/G3DCore.h"

namespace RBX {

	// A utility class for holding a "Universal Dimensions", containing both a
	// relative 'scale' and an absolute 'offset'
	class UDim
	{
	public:
		float scale;
		G3D::int16 offset;

		UDim(float scale, G3D::int16 offset)
			:scale(scale)
			,offset(offset){}
		UDim()
			:scale(0.0f)
			,offset(0){}

		float transform(const float value) const;
		G3D::int16 transform(G3D::int16 value) const;
		// Assignment
		UDim& operator=(const UDim& other)
		{
			scale = other.scale;
			offset = other.offset;
			return *this;
		}
		bool operator==(const UDim& other) const {
			return scale==other.scale && offset==other.offset;
		}
		bool operator!=(const UDim& other) const {
			return scale!=other.scale || offset!=other.offset;
		}
		UDim operator*(const G3D::int16 rhs) const;
		UDim operator*(const float rhs) const;

		UDim operator+ (const UDim& v) const;
		UDim operator- (const UDim& v) const;
		UDim operator- () const;
	};

	class UDim2
	{
	public:
		UDim x;
		UDim y;

		UDim2()
			:x()
			,y()
		{}
		UDim2(UDim x, UDim y)
			:x(x)
			,y(y){}
		UDim2(float scaleX, int offsetX, float scaleY, int offsetY)
			:x(scaleX,offsetX)
			,y(scaleY,offsetY){}

		// Assignment
		UDim2& operator=(const UDim2& other)
		{
			x = other.x;
			y = other.y;
			return *this;
		}
		bool operator==(const UDim2& other) const {
			return x == other.x && y == other.y;
		}
		bool operator!=(const UDim2& other) const {
			return x!=other.x || y!=other.y;
		}

		G3D::Vector2int16 operator*(const G3D::Vector2int16 rhs) const;
		G3D::Vector2 operator*(const G3D::Vector2 rhs) const;
		UDim2 operator* (float v) const;
		UDim2 operator+ (const UDim2& v) const;
		UDim2 operator- (const UDim2& v) const;
		UDim2 operator- () const;


		const UDim& operator[] (int i) const {
			switch(i){
				case 1:
					return y;
				case 0: 
				default:
					return x;
			}
		}
		UDim& operator[] (int i){
			switch(i){
				case 1:
					return y;
				case 0: 
				default:
					return x;
			}
		}
	};

}  // namespace RBX
