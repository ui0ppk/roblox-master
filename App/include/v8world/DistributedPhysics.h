/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */

#pragma once

namespace RBX { 
	namespace Network {
		class DistributedPhysics
		{
		public:
			static const float MIN_CLIENT_SIMULATION_DISTANCE()			{return 10.0f;}
			static const float MAX_CLIENT_SIMULATION_DISTANCE()			{return 1000.0f;}

			static const float CLIENT_SLOP()							{return 1.05f;}	// 105%  how far out of the region before client stops simulating
			static const float SERVER_SLOP()							{return 1.00f;}	// server switches simulation to someone else as soon as the object leaves the region
		};
	}
}