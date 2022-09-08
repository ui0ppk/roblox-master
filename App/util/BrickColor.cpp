/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Util/BrickColor.h"
#include <map>
#include <assert.h>
#include "rbx/Debug.h"
#include <boost/functional/hash.hpp>

namespace RBX {

	class BrickColor::BrickMap
	{
	public:
		Colors allColors;
		Colors colorPalette;
		Colors renderingPalette; // supported by the current graphics engine.
		
		struct ColorInfo
		{
			bool valid;
			bool deprecated;
			G3D::Color4uint8 colorInt;
			G3D::Color4 color;
			std::string name;
			
			ColorInfo()
			 : valid(false)
			 , deprecated(true)
			{}
		};

		std::vector<ColorInfo> map;
		
		std::map<BrickColor::Number, int> paletteMap;		// maps number to index in colorPalette
		std::map<BrickColor::Number, int> renderingPaletteMap;		// maps number to index in colorPalette, limited to first "supported colors"

		static BrickMap& singleton();
	private:
		void insert(BrickColor::Number number, G3D::uint8 r, G3D::uint8 g, G3D::uint8 b, std::string name)
		{
			allColors.push_back(BrickColor(number));

			if (map.size() <= static_cast<unsigned int>(number))
				map.resize(number + 1);
				
			ColorInfo& info = map[number];
			
			info.valid = true;
			info.colorInt = G3D::Color4uint8(r, g, b, 255);
			info.color = G3D::Color4(info.colorInt);
			info.name = name;
		}	
		void insertPaletteItem(BrickColor::Number c)
		{
			colorPalette.push_back(c);
			map[c].deprecated = false;
		}

		void generatePaletteMap(std::map<BrickColor::Number, int>& pMap, Colors availablecolors, Number number)
		{
			int bestMatch = 0;
			float closestDistance = 10000;

			G3D::Color4 match = map[number].color;

			for(size_t paletteIndex = 0; paletteIndex < availablecolors.size(); ++paletteIndex)
			{
				if (availablecolors[paletteIndex]==number)
				{
					// short-cut
					bestMatch = paletteIndex;
					break;
				}
				else
				{
					G3D::Color4 color = map[availablecolors[paletteIndex].number].color;
					float distance = fabs(match.r - color.r);
					distance += fabs(match.g - color.g);
					distance += fabs(match.b - color.b);
					if (distance<closestDistance) {
						bestMatch = paletteIndex;
						if (distance==0.0f)
							break;	
						closestDistance = distance;
					}
				}
			}
			
			pMap[number] = bestMatch;

		}

		void generatePaletteMap()
		{
			paletteMap.clear();
			renderingPaletteMap.clear();

			Colors::iterator iter = allColors.begin();
			Colors::iterator end = allColors.end();
			while (iter!=end)
			{
				generatePaletteMap(paletteMap, colorPalette, iter->number);
				generatePaletteMap(renderingPaletteMap, renderingPalette, iter->number);
				++iter;
			}
		}

		BrickMap()
		{
			// note all color "green" values changed to avoid trademark issues
			// public names also changed
			insert(BrickColor::brick_1, 242, 243, 243, "White"); //	CoolGrey 1 C
			insert(BrickColor::brick_2, 161, 165, 162, "Grey"); //	422 C
			insert(BrickColor::brick_3, 249, 233, 153, "Light yellow"); //	1215 C
			insert(BrickColor::brick_5, 215, 197, 154, "Brick yellow"); //	467 C
			insert(BrickColor::brick_6, 194, 218, 184, "Light green (Mint)"); //	351 C
			insert(BrickColor::brick_9, 232, 186, 200, "Light reddish violet"); //	203 C
			insert(BrickColor::brick_11, 0x80,0xbb,0xdb, "Pastel Blue"); //	
			insert(BrickColor::brick_12, 203, 132, 66, "Light orange brown"); //	1385 C
			insert(BrickColor::brick_18, 204, 142, 105, "Nougat"); //	472 C
			insert(BrickColor::brick_21, 196, 40, 28, "Bright red"); //	032 C
			insert(BrickColor::brick_22, 196, 112, 160, "Med. reddish violet"); //	2375 C
			insert(BrickColor::brick_23, 13, 105, 172, "Bright blue"); //	293 C
			insert(BrickColor::brick_24, 245, 205, 48, "Bright yellow"); //	116 C
			insert(BrickColor::brick_25, 98, 71, 50, "Earth orange"); //	469 C
			insert(BrickColor::brick_26, 27, 42, 53, "Black"); //	Process Black C
			insert(BrickColor::brick_27, 109, 110, 108, "Dark grey"); //	418 C
			insert(BrickColor::brick_28, 40, 127, 71, "Dark green"); //	348 C
			insert(BrickColor::brick_29, 161, 196, 140, "Medium green"); //	353 C
			insert(BrickColor::brick_36, 243, 207, 155, "Lig. Yellowich orange"); //	148 C
			insert(BrickColor::brick_37, 75, 151, 75, "Bright green"); //	355 C
			insert(BrickColor::brick_38, 160, 95, 53, "Dark orange"); //	471 C
			insert(BrickColor::brick_39, 193, 202, 222, "Light bluish violet"); //	2706 C
			insert(BrickColor::brick_40, 236, 236, 236, "Transparent"); //	CoolGrey 1 C
			insert(BrickColor::brick_41, 205, 84, 75, "Tr. Red"); //	185 C
			insert(BrickColor::brick_42, 193, 223, 240, "Tr. Lg blue"); //	304 C
			insert(BrickColor::brick_43, 123, 182, 232, "Tr. Blue"); //	298 C
			insert(BrickColor::brick_44, 247, 241, 141, "Tr. Yellow"); //	393 C
			insert(BrickColor::brick_45, 180, 210, 228, "Light blue"); //	545 C
			insert(BrickColor::brick_47, 217, 133, 108, "Tr. Flu. Reddish orange"); //	165 C
			insert(BrickColor::brick_48, 132, 182, 141, "Tr. Green"); //	360 C
			insert(BrickColor::brick_49, 248, 241, 132, "Tr. Flu. Green"); //	387 C
			insert(BrickColor::brick_50, 236, 232, 222, "Phosph. White"); //	427 C
			insert(BrickColor::brick_100, 238, 196, 182, "Light red"); //	169 C
			insert(BrickColor::brick_101, 218, 134, 122, "Medium red"); //	170 C
			insert(BrickColor::brick_102, 110, 153, 202, "Medium blue"); //	284 C
			insert(BrickColor::brick_103, 199, 193, 183, "Light grey"); //	421 C
			insert(BrickColor::brick_104, 107, 50, 124, "Bright violet"); //	2592 C
			insert(BrickColor::brick_105, 226, 155, 64, "Br. yellowish orange"); //	137 C
			insert(BrickColor::brick_106, 218, 133, 65, "Bright orange"); //	151 C
			insert(BrickColor::brick_107, 0, 143, 156, "Bright bluish green"); //	327 C
			insert(BrickColor::brick_108, 104, 92, 67, "Earth yellow"); //	1405 C
			insert(BrickColor::brick_110, 67, 84, 147, "Bright bluish violet"); //	2736 C
			insert(BrickColor::brick_111, 191, 183, 177, "Tr. Brown"); //	WarmGrey 3 C
			insert(BrickColor::brick_112, 104, 116, 172, "Medium bluish violet"); //	2726 C
			insert(BrickColor::brick_113, 228, 173, 200, "Tr. Medi. reddish violet"); //	230 C
			insert(BrickColor::brick_115, 199, 210, 60, "Med. yellowish green"); //	381 C
			insert(BrickColor::brick_116, 85, 165, 175, "Med. bluish green"); //	326 C
			insert(BrickColor::brick_118, 183, 215, 213, "Light bluish green"); //	324 C
			insert(BrickColor::brick_119, 164, 189, 71, "Br. yellowish green"); //	390 C
			insert(BrickColor::brick_120, 217, 228, 167, "Lig. yellowish green"); //	365 C
			insert(BrickColor::brick_121, 231, 172, 88, "Med. yellowish orange"); //	1365 C
			insert(BrickColor::brick_123, 211, 111, 76, "Br. reddish orange"); //	165 C
			insert(BrickColor::brick_124, 146, 57, 120, "Bright reddish violet"); //	241 C
			insert(BrickColor::brick_125, 234, 184, 146, "Light orange"); //	1555 C
			insert(BrickColor::brick_126, 165, 165, 203, "Tr. Bright bluish violet"); //	271 C
			insert(BrickColor::brick_127, 220, 188, 129, "Gold"); //	156 C
			insert(BrickColor::brick_128, 174, 122, 89, "Dark nougat"); //	471 C
			insert(BrickColor::brick_131, 156, 163, 168, "Silver"); //	429 C
			insert(BrickColor::brick_133, 213, 115, 61, "Neon orange"); //	165 C
			insert(BrickColor::brick_134, 216, 221, 86, "Neon green"); //	374 C
			insert(BrickColor::brick_135, 116, 134, 157, "Sand blue"); //	5415 C
			insert(BrickColor::brick_136, 135, 124, 144, "Sand violet"); //	666 C
			insert(BrickColor::brick_137, 224, 152, 100, "Medium orange"); //	1575 C
			insert(BrickColor::brick_138, 149, 138, 115, "Sand yellow"); //	451 C
			insert(BrickColor::brick_140, 32, 58, 86, "Earth blue"); //	2955 C
			insert(BrickColor::brick_141, 39, 70, 45, "Earth green"); //	350 C
			insert(BrickColor::brick_143, 207, 226, 247, "Tr. Flu. Blue"); //	657 C
			insert(BrickColor::brick_145, 121, 136, 161, "Sand blue metallic"); //	652C
			insert(BrickColor::brick_146, 149, 142, 163, "Sand violet metallic"); //	5285 C
			insert(BrickColor::brick_147, 147, 135, 103, "Sand yellow metallic"); //	873 C
			insert(BrickColor::brick_148, 87, 88, 87, "Dark grey metallic"); //	446 C
			insert(BrickColor::brick_149, 22, 29, 50, "Black metallic"); //	2767 C
			insert(BrickColor::brick_150, 171, 173, 172, "Light grey metallic"); //	422 C
			insert(BrickColor::brick_151, 120, 144, 130, "Sand green"); //	624 C
			insert(BrickColor::brick_153, 149, 121, 119, "Sand red"); //	4995 C
			insert(BrickColor::brick_154, 123, 46, 47, "Dark red"); //	194 C
			insert(BrickColor::brick_157, 255, 246, 123, "Tr. Flu. Yellow"); //	395 C
			insert(BrickColor::brick_158, 225, 164, 194, "Tr. Flu. Red"); //	211 C
			insert(BrickColor::brick_168, 117, 108, 98, "Gun metallic"); //	WarmGrey 11 C
			insert(BrickColor::brick_176, 151, 105, 91, "Red flip/flop"); //	483 C
			insert(BrickColor::brick_178, 180, 132, 85, "Yellow flip/flop"); //	160 C
			insert(BrickColor::brick_179, 137, 135, 136, "Silver flip/flop"); //	410 C
			insert(BrickColor::brick_180, 215, 169, 75, "Curry"); //	131 C
			insert(BrickColor::brick_190, 249, 214, 46, "Fire Yellow"); //	012 C
			insert(BrickColor::brick_191, 232, 171, 45, "Flame yellowish orange"); //	137 C
			insert(BrickColor::brick_192, 105, 64, 40, "Reddish brown"); //	499 C
			insert(BrickColor::brick_193, 207, 96, 36, "Flame reddish orange"); //	173 C
			insert(BrickColor::brick_194, 163, 162, 165, "Medium stone grey"); //	429 C
			insert(BrickColor::brick_195, 70, 103, 164, "Royal blue"); //	2728 C
			insert(BrickColor::brick_196, 35, 71, 139, "Dark Royal blue"); //	286 C
			insert(BrickColor::brick_198, 142, 66, 133, "Bright reddish lilac"); //	254 C
			insert(BrickColor::brick_199, 99, 95, 98, "Dark stone grey"); //	431 C
			insert(BrickColor::brick_200, 130, 138, 93, "Lemon metalic"); //	5767 C
			insert(BrickColor::brick_208, 229, 228, 223, "Light stone grey"); //	428 C
			insert(BrickColor::brick_209, 176, 142, 68, "Dark Curry"); //	118 C
			insert(BrickColor::brick_210, 112, 149, 120, "Faded green"); //	364 C
			insert(BrickColor::brick_211, 121, 181, 181, "Turquoise"); //	3255 C
			insert(BrickColor::brick_212, 159, 195, 233, "Light Royal blue"); //	292 C
			insert(BrickColor::brick_213, 108, 129, 183, "Medium Royal blue"); //	2727 C
			insert(BrickColor::brick_216, 143, 76, 42, "Rust"); //	174 C
			insert(BrickColor::brick_217, 124, 92, 70, "Brown"); //	161 C
			insert(BrickColor::brick_218, 150, 112, 159, "Reddish lilac"); //	2573 C
			insert(BrickColor::brick_219, 107, 98, 155, "Lilac"); //	2725 C
			insert(BrickColor::brick_220, 167, 169, 206, "Light lilac"); //	2716 C
			insert(BrickColor::brick_221, 205, 98, 152, "Bright purple"); //	232 C
			insert(BrickColor::brick_222, 228, 173, 200, "Light purple"); //	236 C
			insert(BrickColor::brick_223, 220, 144, 149, "Light pink"); //	183 C
			insert(BrickColor::brick_224, 240, 213, 160, "Light brick yellow"); //	7501 C
			insert(BrickColor::brick_225, 235, 184, 127, "Warm yellowish orange"); //	713 C
			insert(BrickColor::brick_226, 253, 234, 141, "Cool yellow"); //	120 C
			insert(BrickColor::brick_232, 125, 187, 221, "Dove blue"); //	311 C
			insert(BrickColor::brick_268, 52, 43, 117, "Medium lilac"); //	2685 C

			insert(BrickColor::brick_301, 80,109,84, "Slime green");
			insert(BrickColor::brick_302, 91,93,105, "Smoky grey");
			insert(BrickColor::brick_303, 0,16,176, "Dark blue");
			insert(BrickColor::brick_304, 44,101,29, "Parsley green");
			insert(BrickColor::brick_305, 82,124,174, "Steel blue");
			insert(BrickColor::brick_306, 51,88,130, "Storm blue");
			insert(BrickColor::brick_307, 16,42,220, "Lapis");
			insert(BrickColor::brick_308, 61,21,133, "Dark indigo");
			insert(BrickColor::brick_309, 52,142,64, "Sea green");
			insert(BrickColor::brick_310, 91,154,76, "Shamrock");
			insert(BrickColor::brick_311, 159,161,172, "Fossil");
			insert(BrickColor::brick_312, 89,34,89, "Mulberry");
			insert(BrickColor::brick_313, 31,128,29, "Forest green");
			insert(BrickColor::brick_314, 159,173,192, "Cadet blue");
			insert(BrickColor::brick_315, 9,137,207, "Electric blue");
			insert(BrickColor::brick_316, 123,0,123, "Eggplant");
			insert(BrickColor::brick_317, 124,156,107, "Moss");
			insert(BrickColor::brick_318, 138,171,133, "Artichoke");
			insert(BrickColor::brick_319, 185,196,177, "Sage green");
			insert(BrickColor::brick_320, 202,203,209, "Ghost grey");
			insert(BrickColor::brick_321, 167,94,155, "Lilac");
			insert(BrickColor::brick_322, 123,47,123, "Plum");
			insert(BrickColor::brick_323, 148,190,129, "Olivine");
			insert(BrickColor::brick_324, 168,189,153, "Laurel green");
			insert(BrickColor::brick_325, 223,223,222, "Quill grey");
			//insert(BrickColor::brick_326, 218,220,225, "Iron");
			insert(BrickColor::brick_327, 151,0,0, "Crimson");
			insert(BrickColor::brick_328, 177,229,166, "Mint");
			insert(BrickColor::brick_329, 152,194,219, "Baby blue");
			insert(BrickColor::brick_330, 255,152,220, "Carnation pink");
			insert(BrickColor::brick_331, 255,89,89, "Persimmon");
			insert(BrickColor::brick_332, 117,0,0, "Maroon");
			insert(BrickColor::brick_333, 239,184,56, "Gold");
			insert(BrickColor::brick_334, 248,217,109, "Daisy orange");
			insert(BrickColor::brick_335, 231,231,236, "Pearl");
			insert(BrickColor::brick_336, 199,212,228, "Fog");
			insert(BrickColor::brick_337, 255,148,148, "Salmon");
			insert(BrickColor::brick_338, 190,104,98, "Terra Cotta");
			insert(BrickColor::brick_339, 86,36,36, "Cocoa");
			insert(BrickColor::brick_340, 241,231,199, "Wheat");
			insert(BrickColor::brick_341, 254,243,187, "Buttermilk");
			insert(BrickColor::brick_342, 224,178,208, "Mauve");
			insert(BrickColor::brick_343, 212,144,189, "Sunrise");
			insert(BrickColor::brick_344, 150,85,85, "Tawny");
			insert(BrickColor::brick_345, 143,76,42, "Rust");
			insert(BrickColor::brick_346, 211,190,150, "Cashmere");
			insert(BrickColor::brick_347, 226,220,188, "Khaki");
			insert(BrickColor::brick_348, 237,234,234, "Lily white");
			insert(BrickColor::brick_349, 233,218,218, "Seashell");
			insert(BrickColor::brick_350, 136,62,62, "Burgundy");
			insert(BrickColor::brick_351, 188,155,93, "Cork");
			insert(BrickColor::brick_352, 199,172,120, "Burlap");
			insert(BrickColor::brick_353, 202,191,163, "Beige");
			insert(BrickColor::brick_354, 187,179,178, "Oyster");
			insert(BrickColor::brick_355, 108,88,75, "Pine Cone");
			insert(BrickColor::brick_356, 160,132,79, "Fawn brown");
			insert(BrickColor::brick_357, 149,137,136, "Hurricane grey");
			insert(BrickColor::brick_358, 171,168,158, "Cloudy grey");
			insert(BrickColor::brick_359, 175,148,131, "Linen");
			insert(BrickColor::brick_360, 150,103,102, "Copper");
			insert(BrickColor::brick_361, 86,66,54, "Dirt brown");
			insert(BrickColor::brick_362, 126,104,63, "Bronze");
			insert(BrickColor::brick_363, 105,102,92, "Flint");
			insert(BrickColor::brick_364, 90,76,66, "Dark taupe");
			insert(BrickColor::brick_365, 106,57,9, "Burnt Sienna");

			insert(BrickColor::roblox_1001, 248, 248, 248, "Institutional white");
			insert(BrickColor::roblox_1002, 205, 205, 205, "Mid gray");
			insert(BrickColor::roblox_1003, 17, 17, 17, "Really black");
			insert(BrickColor::roblox_1004, 255, 0, 0, "Really red");
			insert(BrickColor::roblox_1005, 255, 175, 0, "Deep orange");
			insert(BrickColor::roblox_1006, 180, 128, 255, "Alder");
			insert(BrickColor::roblox_1007, 163, 75, 75, "Dusty Rose");
			insert(BrickColor::roblox_1008, 193, 190, 66, "Olive");
			insert(BrickColor::roblox_1009, 255, 255, 0, "New Yeller");
			insert(BrickColor::roblox_1010, 0, 0, 255, "Really blue");
			insert(BrickColor::roblox_1011, 0, 32, 96, "Navy blue");
			insert(BrickColor::roblox_1012, 33, 84, 185, "Deep blue");
			insert(BrickColor::roblox_1013, 4, 175, 236, "Cyan");
			insert(BrickColor::roblox_1014, 170, 85, 0, "CGA brown");
			insert(BrickColor::roblox_1015, 170, 0, 170, "Magenta");
			insert(BrickColor::roblox_1016, 255, 102, 204, "Pink");
			insert(BrickColor::roblox_1017, 255, 175, 0, "Deep orange");
			insert(BrickColor::roblox_1018, 18, 238, 212, "Teal");
			insert(BrickColor::roblox_1019, 0, 255, 255, "Toothpaste");
			insert(BrickColor::roblox_1020, 0, 255, 0, "Lime green");
			insert(BrickColor::roblox_1021, 58, 125, 21, "Camo");
			insert(BrickColor::roblox_1022, 127, 142, 100, "Grime");
			insert(BrickColor::roblox_1023, 140, 91, 159, "Lavender");
			insert(BrickColor::roblox_1024, 175, 221, 255, "Pastel light blue");
			insert(BrickColor::roblox_1025, 255, 201, 201, "Pastel orange");
			insert(BrickColor::roblox_1026, 177, 167, 255, "Pastel violet");
			insert(BrickColor::roblox_1027, 159, 243, 233, "Pastel blue-green");
			insert(BrickColor::roblox_1028, 204, 255, 204, "Pastel green");
			insert(BrickColor::roblox_1029, 255, 255, 204, "Pastel yellow");
			insert(BrickColor::roblox_1030, 255, 204, 153, "Pastel brown");
			insert(BrickColor::roblox_1031, 98, 37, 209, "Royal purple");
			insert(BrickColor::roblox_1032, 255, 0, 191, "Hot pink");

			updateColorPalette();
		}
	public:
		void setRenderingSupportedPaletteSize(size_t maxSupportedColors)
		{
			if(maxSupportedColors == renderingPalette.size())
				return; // nothing to do;

			renderingPalette.clear();

			renderingPalette.push_back(BrickColor::brick_141);
			renderingPalette.push_back(BrickColor::brick_301);
			renderingPalette.push_back(BrickColor::brick_107);
			renderingPalette.push_back(BrickColor::brick_302);
			renderingPalette.push_back(BrickColor::brick_26);
			renderingPalette.push_back(BrickColor::roblox_1012);
			renderingPalette.push_back(BrickColor::brick_303);
			renderingPalette.push_back(BrickColor::roblox_1011);
			renderingPalette.push_back(BrickColor::brick_304);
			renderingPalette.push_back(BrickColor::brick_28);
			renderingPalette.push_back(BrickColor::roblox_1018);
			renderingPalette.push_back(BrickColor::brick_135);
			renderingPalette.push_back(BrickColor::brick_305);
			renderingPalette.push_back(BrickColor::brick_306);
			renderingPalette.push_back(BrickColor::brick_307);
			renderingPalette.push_back(BrickColor::brick_308);
			renderingPalette.push_back(BrickColor::roblox_1021);
			renderingPalette.push_back(BrickColor::brick_309);
			renderingPalette.push_back(BrickColor::brick_310);
			renderingPalette.push_back(BrickColor::roblox_1019);
			renderingPalette.push_back(BrickColor::brick_311);
			renderingPalette.push_back(BrickColor::brick_102);
			renderingPalette.push_back(BrickColor::brick_23);
			renderingPalette.push_back(BrickColor::roblox_1010);
			renderingPalette.push_back(BrickColor::brick_312);
			renderingPalette.push_back(BrickColor::brick_313);
			renderingPalette.push_back(BrickColor::brick_37);
			renderingPalette.push_back(BrickColor::roblox_1022);
			renderingPalette.push_back(BrickColor::roblox_1020);
			renderingPalette.push_back(BrickColor::roblox_1027);
			renderingPalette.push_back(BrickColor::brick_314);
			renderingPalette.push_back(BrickColor::brick_315);
			renderingPalette.push_back(BrickColor::roblox_1023);
			renderingPalette.push_back(BrickColor::roblox_1031);
			renderingPalette.push_back(BrickColor::brick_316);
			renderingPalette.push_back(BrickColor::brick_151);
			renderingPalette.push_back(BrickColor::brick_317);
			renderingPalette.push_back(BrickColor::brick_318);
			renderingPalette.push_back(BrickColor::brick_319);
			renderingPalette.push_back(BrickColor::brick_320);
			renderingPalette.push_back(BrickColor::roblox_1024);
			renderingPalette.push_back(BrickColor::roblox_1013);
			renderingPalette.push_back(BrickColor::roblox_1006);
			renderingPalette.push_back(BrickColor::brick_321);
			renderingPalette.push_back(BrickColor::brick_322);
			renderingPalette.push_back(BrickColor::brick_104);
			renderingPalette.push_back(BrickColor::roblox_1008);
			renderingPalette.push_back(BrickColor::brick_119);
			renderingPalette.push_back(BrickColor::brick_323);
			renderingPalette.push_back(BrickColor::brick_324);
			renderingPalette.push_back(BrickColor::brick_325);
			//renderingPalette.push_back(BrickColor::brick_326);
			renderingPalette.push_back(BrickColor::brick_11);
			renderingPalette.push_back(BrickColor::roblox_1026);
			renderingPalette.push_back(BrickColor::roblox_1016);
			renderingPalette.push_back(BrickColor::roblox_1032);
			renderingPalette.push_back(BrickColor::roblox_1015);
			renderingPalette.push_back(BrickColor::brick_327);
			renderingPalette.push_back(BrickColor::roblox_1017);
			renderingPalette.push_back(BrickColor::roblox_1009);
			renderingPalette.push_back(BrickColor::brick_29);
			renderingPalette.push_back(BrickColor::brick_328);
			renderingPalette.push_back(BrickColor::roblox_1028);
			renderingPalette.push_back(BrickColor::brick_208);
			renderingPalette.push_back(BrickColor::brick_45);
			renderingPalette.push_back(BrickColor::brick_329);
			renderingPalette.push_back(BrickColor::brick_330);
			renderingPalette.push_back(BrickColor::brick_331);
			renderingPalette.push_back(BrickColor::roblox_1004);
			renderingPalette.push_back(BrickColor::brick_21);
			renderingPalette.push_back(BrickColor::brick_332);
			renderingPalette.push_back(BrickColor::brick_333);
			renderingPalette.push_back(BrickColor::brick_24);
			renderingPalette.push_back(BrickColor::brick_334);
			renderingPalette.push_back(BrickColor::brick_226);
			renderingPalette.push_back(BrickColor::roblox_1029);
			renderingPalette.push_back(BrickColor::brick_335);
			renderingPalette.push_back(BrickColor::brick_336);
			renderingPalette.push_back(BrickColor::roblox_1025);
			renderingPalette.push_back(BrickColor::brick_337);
			renderingPalette.push_back(BrickColor::brick_338);
			renderingPalette.push_back(BrickColor::roblox_1007);
			renderingPalette.push_back(BrickColor::brick_339);
			//renderingPalette.push_back(BrickColor::roblox_1005);
			renderingPalette.push_back(BrickColor::brick_133);
			renderingPalette.push_back(BrickColor::brick_106);
			renderingPalette.push_back(BrickColor::brick_40);
			renderingPalette.push_back(BrickColor::brick_341);
			renderingPalette.push_back(BrickColor::roblox_1001);
			renderingPalette.push_back(BrickColor::brick_1);
			renderingPalette.push_back(BrickColor::brick_9);
			renderingPalette.push_back(BrickColor::brick_342);
			renderingPalette.push_back(BrickColor::brick_343);
			renderingPalette.push_back(BrickColor::brick_344);
			renderingPalette.push_back(BrickColor::brick_345);
			renderingPalette.push_back(BrickColor::roblox_1014);
			renderingPalette.push_back(BrickColor::brick_105);
			renderingPalette.push_back(BrickColor::brick_346);
			renderingPalette.push_back(BrickColor::brick_347);
			renderingPalette.push_back(BrickColor::brick_348);
			renderingPalette.push_back(BrickColor::brick_349);
			renderingPalette.push_back(BrickColor::roblox_1030);
			renderingPalette.push_back(BrickColor::brick_125);
			renderingPalette.push_back(BrickColor::brick_101);
			renderingPalette.push_back(BrickColor::brick_350);
			renderingPalette.push_back(BrickColor::brick_192);
			renderingPalette.push_back(BrickColor::brick_351);
			renderingPalette.push_back(BrickColor::brick_352);
			renderingPalette.push_back(BrickColor::brick_353);
			renderingPalette.push_back(BrickColor::brick_354);
			renderingPalette.push_back(BrickColor::roblox_1002);
			renderingPalette.push_back(BrickColor::brick_5);
			renderingPalette.push_back(BrickColor::brick_18);
			renderingPalette.push_back(BrickColor::brick_217);
			renderingPalette.push_back(BrickColor::brick_355);
			renderingPalette.push_back(BrickColor::brick_356);
			renderingPalette.push_back(BrickColor::brick_153);
			renderingPalette.push_back(BrickColor::brick_357);
			renderingPalette.push_back(BrickColor::brick_358);
			renderingPalette.push_back(BrickColor::brick_359);
			renderingPalette.push_back(BrickColor::brick_360);
			renderingPalette.push_back(BrickColor::brick_38);
			renderingPalette.push_back(BrickColor::brick_361);
			renderingPalette.push_back(BrickColor::brick_362);
			renderingPalette.push_back(BrickColor::brick_199);
			renderingPalette.push_back(BrickColor::brick_194);
			renderingPalette.push_back(BrickColor::brick_363);
			renderingPalette.push_back(BrickColor::brick_364);
			renderingPalette.push_back(BrickColor::brick_365);
			renderingPalette.push_back(BrickColor::roblox_1003);

			// truncate to fit.
			renderingPalette.resize(std::min(maxSupportedColors, renderingPalette.size()));

			generatePaletteMap();
		}

		void updateColorPalette()
		{
			size_t expectedPaletteSize = 128;
			if (expectedPaletteSize == colorPalette.size())
				return;
			
			colorPalette.clear();
		
			insertPaletteItem(BrickColor::brick_141);
			insertPaletteItem(BrickColor::brick_301);
			insertPaletteItem(BrickColor::brick_107);
			insertPaletteItem(BrickColor::brick_26);
			insertPaletteItem(BrickColor::roblox_1012);
			insertPaletteItem(BrickColor::brick_303);
			insertPaletteItem(BrickColor::roblox_1011);
			insertPaletteItem(BrickColor::brick_304);
			insertPaletteItem(BrickColor::brick_28);
			insertPaletteItem(BrickColor::roblox_1018);
			insertPaletteItem(BrickColor::brick_302);
			insertPaletteItem(BrickColor::brick_305);
			insertPaletteItem(BrickColor::brick_306);
			insertPaletteItem(BrickColor::brick_307);
			insertPaletteItem(BrickColor::brick_308);
			insertPaletteItem(BrickColor::roblox_1021);
			insertPaletteItem(BrickColor::brick_309);
			insertPaletteItem(BrickColor::brick_310);
			insertPaletteItem(BrickColor::roblox_1019);
			insertPaletteItem(BrickColor::brick_135);
			insertPaletteItem(BrickColor::brick_102);
			insertPaletteItem(BrickColor::brick_23);
			insertPaletteItem(BrickColor::roblox_1010);
			insertPaletteItem(BrickColor::brick_312);
			insertPaletteItem(BrickColor::brick_313);
			insertPaletteItem(BrickColor::brick_37);
			insertPaletteItem(BrickColor::roblox_1022);
			insertPaletteItem(BrickColor::roblox_1020);
			insertPaletteItem(BrickColor::roblox_1027);
			insertPaletteItem(BrickColor::brick_311);
			insertPaletteItem(BrickColor::brick_315);
			insertPaletteItem(BrickColor::roblox_1023);
			insertPaletteItem(BrickColor::roblox_1031);
			insertPaletteItem(BrickColor::brick_316);
			insertPaletteItem(BrickColor::brick_151);
			insertPaletteItem(BrickColor::brick_317);
			insertPaletteItem(BrickColor::brick_318);
			insertPaletteItem(BrickColor::brick_319);
			insertPaletteItem(BrickColor::roblox_1024);				
			insertPaletteItem(BrickColor::brick_314);				
			insertPaletteItem(BrickColor::roblox_1013);
			insertPaletteItem(BrickColor::roblox_1006);
			insertPaletteItem(BrickColor::brick_321);
			insertPaletteItem(BrickColor::brick_322);
			insertPaletteItem(BrickColor::brick_104);
			insertPaletteItem(BrickColor::roblox_1008);
			insertPaletteItem(BrickColor::brick_119);
			insertPaletteItem(BrickColor::brick_323);
			insertPaletteItem(BrickColor::brick_324);
			insertPaletteItem(BrickColor::brick_325);
			insertPaletteItem(BrickColor::brick_320);
			insertPaletteItem(BrickColor::brick_11);
			insertPaletteItem(BrickColor::roblox_1026);
			insertPaletteItem(BrickColor::roblox_1016);
			insertPaletteItem(BrickColor::roblox_1032);
			insertPaletteItem(BrickColor::roblox_1015);
			insertPaletteItem(BrickColor::brick_327);
			insertPaletteItem(BrickColor::roblox_1017);
			insertPaletteItem(BrickColor::roblox_1009);
			insertPaletteItem(BrickColor::brick_29);
			insertPaletteItem(BrickColor::brick_328);
			insertPaletteItem(BrickColor::roblox_1028);
			insertPaletteItem(BrickColor::brick_208);
			insertPaletteItem(BrickColor::brick_45);
			insertPaletteItem(BrickColor::brick_329);
			insertPaletteItem(BrickColor::brick_330);
			insertPaletteItem(BrickColor::brick_331);
			insertPaletteItem(BrickColor::roblox_1004);
			insertPaletteItem(BrickColor::brick_21);
			insertPaletteItem(BrickColor::brick_332);
			insertPaletteItem(BrickColor::brick_333);
			insertPaletteItem(BrickColor::brick_24);
			insertPaletteItem(BrickColor::brick_334);
			insertPaletteItem(BrickColor::brick_226);
			insertPaletteItem(BrickColor::roblox_1029);
			insertPaletteItem(BrickColor::brick_335);
			insertPaletteItem(BrickColor::brick_336);
			insertPaletteItem(BrickColor::brick_342);
			insertPaletteItem(BrickColor::brick_343);
			insertPaletteItem(BrickColor::brick_338);
			insertPaletteItem(BrickColor::roblox_1007);
			insertPaletteItem(BrickColor::brick_339);
			//insertPaletteItem(BrickColor::roblox_1005);
			insertPaletteItem(BrickColor::brick_133);
			insertPaletteItem(BrickColor::brick_106);
			insertPaletteItem(BrickColor::brick_340);
			insertPaletteItem(BrickColor::brick_341);
			insertPaletteItem(BrickColor::roblox_1001);
			insertPaletteItem(BrickColor::brick_1);
			insertPaletteItem(BrickColor::brick_9);
			insertPaletteItem(BrickColor::roblox_1025);
			insertPaletteItem(BrickColor::brick_337);
			insertPaletteItem(BrickColor::brick_344);
			insertPaletteItem(BrickColor::brick_345);
			insertPaletteItem(BrickColor::roblox_1014);
			insertPaletteItem(BrickColor::brick_105);
			insertPaletteItem(BrickColor::brick_346);
			insertPaletteItem(BrickColor::brick_347);
			insertPaletteItem(BrickColor::brick_348);
			insertPaletteItem(BrickColor::brick_349);
			insertPaletteItem(BrickColor::roblox_1030);
			insertPaletteItem(BrickColor::brick_125);
			insertPaletteItem(BrickColor::brick_101);
			insertPaletteItem(BrickColor::brick_350);
			insertPaletteItem(BrickColor::brick_192);
			insertPaletteItem(BrickColor::brick_351);
			insertPaletteItem(BrickColor::brick_352);
			insertPaletteItem(BrickColor::brick_353);
			insertPaletteItem(BrickColor::brick_354);
			insertPaletteItem(BrickColor::roblox_1002);
			insertPaletteItem(BrickColor::brick_5);
			insertPaletteItem(BrickColor::brick_18);
			insertPaletteItem(BrickColor::brick_217);
			insertPaletteItem(BrickColor::brick_355);
			insertPaletteItem(BrickColor::brick_356);
			insertPaletteItem(BrickColor::brick_153);
			insertPaletteItem(BrickColor::brick_357);
			insertPaletteItem(BrickColor::brick_358);
			insertPaletteItem(BrickColor::brick_359);
			insertPaletteItem(BrickColor::brick_360);
			insertPaletteItem(BrickColor::brick_38);
			insertPaletteItem(BrickColor::brick_361);
			insertPaletteItem(BrickColor::brick_362);
			insertPaletteItem(BrickColor::brick_199);
			insertPaletteItem(BrickColor::brick_194);
			insertPaletteItem(BrickColor::brick_363);
			insertPaletteItem(BrickColor::brick_364);
			insertPaletteItem(BrickColor::brick_365);
			insertPaletteItem(BrickColor::roblox_1003);

			setRenderingSupportedPaletteSize(colorPalette.size()); // by default, support all the colors in the palette.
		}

	};

	void BrickColor::setRenderingSupportedPaletteSize(size_t maxSupportedColors)
	{
		BrickMap::singleton().setRenderingSupportedPaletteSize(maxSupportedColors);
	}

	BrickColor::BrickMap& BrickColor::BrickMap::singleton()
	{
		static BrickMap s;
		return s;
	}

	const BrickColor::Colors& BrickColor::allColors()
	{
		return BrickMap::singleton().allColors;
	}

	const BrickColor::Colors& BrickColor::colorPalette()
	{
		return BrickMap::singleton().colorPalette;
	}

	const BrickColor::Colors& BrickColor::renderingPalette()
	{
		return BrickMap::singleton().renderingPalette;
	}

	size_t BrickColor::getClosestPaletteIndex() const {
		return BrickMap::singleton().paletteMap[number];
	}

	size_t BrickColor::getClosestRenderingPaletteIndex() const {
		return BrickMap::singleton().renderingPaletteMap[number];
	}


	// TODO: case-insensitive search???
	BrickColor BrickColor::parse(const char* name) {
		const std::vector<BrickMap::ColorInfo>& colors = BrickMap::singleton().map;

		bool isDeprecated = true;
		int brickNumber = -1;

		for (size_t i = 0; i < colors.size() && isDeprecated; ++i)
		{					
			if (colors[i].valid && colors[i].name == name)
			{
				isDeprecated = colors[i].deprecated;
				brickNumber = i;
			}
		}

		if (brickNumber != -1)
			return static_cast<BrickColor::Number>(brickNumber);
				
		return BrickColor::defaultColor();
	}

	BrickColor BrickColor::closest(G3D::Color3uint8 color) {
		G3D::Color4uint8 c4;
		c4.r = color.r;
		c4.g = color.g;
		c4.b = color.b;
		return closest(c4);
	}


	BrickColor BrickColor::closest(G3D::Color4uint8 color)
	{
		BrickColor::Number bestMatch = BrickColor::defaultColor().number;
		int closestDistance = 10000;

		const std::vector<BrickMap::ColorInfo>& colors = BrickMap::singleton().map;
		
		for (size_t i = 0; i < colors.size(); ++i)
		{
			if (!colors[i].valid)
				continue;
				
			G3D::Color4uint8 match = colors[i].colorInt;
			
			int distance = std::abs(match.r - color.r);
			distance += std::abs(match.g - color.g);
			distance += std::abs(match.b - color.b);
			if (distance<closestDistance) {
				bestMatch = static_cast<BrickColor::Number>(i);
				if (distance==0)
					break;
				closestDistance = distance;
			}
		}

		return bestMatch;
	}

	BrickColor BrickColor::random() {
		size_t index = G3D::iRandom(0, BrickMap::singleton().colorPalette.size()-1);
		return BrickMap::singleton().colorPalette[index]; 
	}

	BrickColor BrickColor::closest(G3D::Color3 color) {
		G3D::Color4 c4;
		c4.r = color.r;
		c4.g = color.g;
		c4.b = color.b;
		return closest(c4);
	}

	BrickColor BrickColor::closest(G3D::Color4 color)
	{
		BrickColor::Number bestMatch = BrickColor::defaultColor().number;
		float closestDistance = 10000;
		
		const std::vector<BrickMap::ColorInfo>& colors = BrickMap::singleton().map;
		
		for (size_t i = 0; i < colors.size(); ++i)
		{
			if (!colors[i].valid)
				continue;
				
			G3D::Color4 match = colors[i].color;
				
			float distance = fabs(match.r - color.r);
			distance += fabs(match.g - color.g);
			distance += fabs(match.b - color.b);
			if (distance<closestDistance) {
				bestMatch = static_cast<BrickColor::Number>(i);
				if (distance==0.0f)
					break;	
				closestDistance = distance;
			}
		}

		return bestMatch;
	}

	// Assignment
	BrickColor::BrickColor(int number)
	{
		const std::vector<BrickMap::ColorInfo>& colors = BrickMap::singleton().map;
		
		if (static_cast<unsigned int>(number) < colors.size() && colors[number].valid)
			this->number = static_cast<BrickColor::Number>(number);
		else
			this->number = defaultColor().number;
	}

	G3D::Color4uint8 BrickColor::color4uint8() const {
		const std::vector<BrickMap::ColorInfo>& colors = BrickMap::singleton().map;
		RBXASSERT(static_cast<unsigned int>(number) < colors.size() && colors[number].valid);
		return colors[number].colorInt;
	}

	G3D::Color3uint8 BrickColor::color3uint8() const {
		G3D::Color4uint8 temp = color4uint8();
		return G3D::Color3uint8(temp.r, temp.g, temp.b);
	}

	const std::string& BrickColor::name() const {
		const std::vector<BrickMap::ColorInfo>& colors = BrickMap::singleton().map;
		RBXASSERT(static_cast<unsigned int>(number) < colors.size() && colors[number].valid);
		return colors[number].name;
	}

	G3D::Color4 BrickColor::color4() const {
		const std::vector<BrickMap::ColorInfo>& colors = BrickMap::singleton().map;
		RBXASSERT(static_cast<unsigned int>(number) < colors.size() && colors[number].valid);
		return colors[number].color;
	}

	G3D::Color3 BrickColor::color3() const {
		G3D::Color4 temp = BrickColor::color4();
		return G3D::Color3(temp.r, temp.g, temp.b);
	}

	std::size_t hash_value(const BrickColor& c)
	{
		boost::hash<int> hasher;
		return hasher(c.asInt());
	}
}

