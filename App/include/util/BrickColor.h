/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "G3D/Color3.h"
#include "G3D/Color4.h"
#include "G3D/Color3uint8.h"
#include "G3D/Color4uint8.h"

#include <vector>

namespace RBX {

	// A collection of official ROBLOX colors
	class BrickColor
	{
		class BrickMap;
	public:
		enum Number {
			brick_1 = 1,
			brick_2 = 2,
			brick_3 = 3,
			brick_5 = 5,
			brick_6 = 6,
			brick_9 = 9,
			brick_11 = 11,
			brick_12 = 12,
			brick_18 = 18,
			brick_21 = 21,
			brick_22 = 22,
			brick_23 = 23,
			brick_24 = 24,
			brick_25 = 25,
			brick_26 = 26,
			brick_27 = 27,
			brick_28 = 28,
			brick_29 = 29,
			brick_36 = 36,
			brick_37 = 37,
			brick_38 = 38,
			brick_39 = 39,
			brick_40 = 40,
			brick_41 = 41,
			brick_42 = 42,
			brick_43 = 43,
			brick_44 = 44,
			brick_45 = 45,
			brick_47 = 47,
			brick_48 = 48,
			brick_49 = 49,
			brick_50 = 50,
			brick_100 = 100,
			brick_101 = 101,
			brick_102 = 102,
			brick_103 = 103,
			brick_104 = 104,
			brick_105 = 105,
			brick_106 = 106,
			brick_107 = 107,
			brick_108 = 108,
			brick_110 = 110,
			brick_111 = 111,
			brick_112 = 112,
			brick_113 = 113,
			brick_115 = 115,
			brick_116 = 116,
			brick_118 = 118,
			brick_119 = 119,
			brick_120 = 120,
			brick_121 = 121,
			brick_123 = 123,
			brick_124 = 124,
			brick_125 = 125,
			brick_126 = 126,
			brick_127 = 127,
			brick_128 = 128,
			brick_131 = 131,
			brick_133 = 133,
			brick_134 = 134,
			brick_135 = 135,
			brick_136 = 136,
			brick_137 = 137,
			brick_138 = 138,
			brick_140 = 140,
			brick_141 = 141,
			brick_143 = 143,
			brick_145 = 145,
			brick_146 = 146,
			brick_147 = 147,
			brick_148 = 148,
			brick_149 = 149,
			brick_150 = 150,
			brick_151 = 151,
			brick_153 = 153,
			brick_154 = 154,
			brick_157 = 157,
			brick_158 = 158,
			brick_168 = 168,
			brick_176 = 176,
			brick_178 = 178,
			brick_179 = 179,
			brick_180 = 180,
			brick_190 = 190,
			brick_191 = 191,
			brick_192 = 192,
			brick_193 = 193,
			brick_194 = 194,
			brick_195 = 195,
			brick_196 = 196,
			brick_198 = 198,
			brick_199 = 199,
			brick_200 = 200,
			brick_208 = 208,
			brick_209 = 209,
			brick_210 = 210,
			brick_211 = 211,
			brick_212 = 212,
			brick_213 = 213,
			brick_216 = 216,
			brick_217 = 217,
			brick_218 = 218,
			brick_219 = 219,
			brick_220 = 220,
			brick_221 = 221,
			brick_222 = 222,
			brick_223 = 223,
			brick_224 = 224,
			brick_225 = 225,
			brick_226 = 226,
			brick_232 = 232,
			brick_268 = 268,
			brick_301 = 301,
			brick_302 = 302,
			brick_303 = 303,
			brick_304 = 304,
			brick_305 = 305,
			brick_306 = 306,
			brick_307 = 307,
			brick_308 = 308,
			brick_309 = 309,
			brick_310 = 310,
			brick_311 = 311,
			brick_312 = 312,
			brick_313 = 313,
			brick_314 = 314,
			brick_315 = 315,
			brick_316 = 316,
			brick_317 = 317,
			brick_318 = 318,
			brick_319 = 319,
			brick_320 = 320,
			brick_321 = 321,
			brick_322 = 322,
			brick_323 = 323,
			brick_324 = 324,
			brick_325 = 325,
			//brick_326 = 326,
			brick_327 = 327,
			brick_328 = 328,
			brick_329 = 329,
			brick_330 = 330,
			brick_331 = 331,
			brick_332 = 332,
			brick_333 = 333,
			brick_334 = 334,
			brick_335 = 335,
			brick_336 = 336,
			brick_337 = 337,
			brick_338 = 338,
			brick_339 = 339,
			brick_340 = 340,
			brick_341 = 341,
			brick_342 = 342,
			brick_343 = 343,
			brick_344 = 344,
			brick_345 = 345,
			brick_346 = 346,
			brick_347 = 347,
			brick_348 = 348,
			brick_349 = 349,
			brick_350 = 350,
			brick_351 = 351,
			brick_352 = 352,
			brick_353 = 353,
			brick_354 = 354,
			brick_355 = 355,
			brick_356 = 356,
			brick_357 = 357,
			brick_358 = 358,
			brick_359 = 359,
			brick_360 = 360,
			brick_361 = 361,
			brick_362 = 362,
			brick_363 = 363,
			brick_364 = 364,
			brick_365 = 365,
			roblox_1001 = 1001,
			roblox_1002 = 1002,
			roblox_1003 = 1003,
			roblox_1004 = 1004,
			roblox_1005 = 1005,
			roblox_1006 = 1006,
			roblox_1007 = 1007,
			roblox_1008 = 1008,
			roblox_1009 = 1009,
			roblox_1010 = 1010,
			roblox_1011 = 1011,
			roblox_1012 = 1012,
			roblox_1013 = 1013,
			roblox_1014 = 1014,
			roblox_1015 = 1015,
			roblox_1016 = 1016,
			roblox_1017 = 1017,
			roblox_1018 = 1018,
			roblox_1019 = 1019,
			roblox_1020 = 1020,
			roblox_1021 = 1021,
			roblox_1022 = 1022,
			roblox_1023 = 1023,
			roblox_1024 = 1024,
			roblox_1025 = 1025,
			roblox_1026 = 1026,
			roblox_1027 = 1027,
			roblox_1028 = 1028,
			roblox_1029 = 1029,
			roblox_1030 = 1030,
			roblox_1031 = 1031,
			roblox_1032 = 1032
		};
		Number number;
		typedef std::vector< BrickColor > Colors;
		static const Colors& colorPalette();		// colors shown in UI
		static const Colors& renderingPalette();	// colors supported by renderer
		static const Colors& allColors();			// all known colors

		// returns the 0-based index of the color
		size_t getClosestRenderingPaletteIndex() const; // closest "supported" palette index by the GFX engine.
		size_t getClosestPaletteIndex() const; // closest palette index from the _whole_ palette list.
		static const size_t paletteSize = 128; // supported by UI and data model.
		static const size_t paletteSizeMSB = 7;	// == log2(paletteSize)

		static void setRenderingSupportedPaletteSize(size_t maxSupportedColors);

		// Constructor/Factory
		BrickColor(Number number):number(number) {
		}
		BrickColor():number(brick_194) {
		}
		explicit BrickColor(int number);
		static BrickColor closest(G3D::Color3uint8 color);
		static BrickColor closest(G3D::Color4uint8 color);
		static BrickColor closest(G3D::Color3 color);
		static BrickColor closest(G3D::Color4 color);
		static BrickColor parse(const char* name);
		static BrickColor random();
		inline static BrickColor brickWhite()
		{
			return brick_1;
		}
		inline static BrickColor brickGray()
		{
			return brick_194;
		}
		inline static BrickColor brickDarkGray()
		{
			return brick_199;
		}
		inline static BrickColor brickBlack()
		{
			return brick_26;
		}
		inline static BrickColor brickRed()
		{
			return brick_21;
		}
		inline static BrickColor brickYellow()
		{
			return brick_24;
		}
		inline static BrickColor brickGreen()
		{
			return brick_28;
		}
		inline static BrickColor baseplateGreen()
		{
			return brick_37;
		}
		inline static BrickColor brickBlue()
		{
			return brick_23;
		}
		inline static BrickColor defaultColor()
		{
			return BrickColor();
		}

		// Assignment
		BrickColor& operator=(const BrickColor& other)
		{
			number = other.number;
			return *this;
		}

		//Query
		G3D::Color4uint8 color4uint8() const;
		G3D::Color3uint8 color3uint8() const;
		G3D::Color4 color4() const;
		G3D::Color3 color3() const;
		const std::string& name() const;

		// returns the number as an int, not an ARGB
		int asInt() const { return number; }

		// Comparison
		bool operator==(const BrickColor& other) const {
			return number==other.number;
		}
		bool operator!=(const BrickColor& other) const {
			return number!=other.number;
		}
		bool operator>(const BrickColor& other) const {
			return number>other.number;
		}
		bool operator<(const BrickColor& other) const {
			return number<other.number;
		}


	};

	std::size_t hash_value(const BrickColor& c);



} // namespace