/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Util/Average.h"

namespace RBX {

	class Workspace;

	namespace Network {
		class Player;
	}


	class PhysicsInstructions 
	{
	private:
		Average<double> averageCyclicDt;
		Average<double> averageDt;
		Average<double> averageDutyDt;
		double timeSinceLastRadiusChange;

		double throttleTimer;
		double throttleAdjustTime;

		void changeSimulationRadius(RBX::Network::Player* dPhysPlayer, float change);
		void changeMaxSimulationRadius(RBX::Network::Player* dPhysPlayer, float change);

	public:
		double requestedDutyPercent;
		bool bandwidthExceeded;
		double networkBufferHealth;

		PhysicsInstructions();

		void setThrottles(RBX::Network::Player* dPhysPlayer, Workspace* workspace, double dt, double dutyDt);
		void setCyclicThrottles(RBX::Network::Player* dPhysPlayer, Workspace* workspace, double cyclicDt, double dt, double dutyDt);
		void setThrottlesBase( RBX::Network::Player* dPhysPlayer, Workspace* workspace, bool realTimePerfOK, bool dutyPerfOK, double avgDutyPercent, double dt); 

		static double visitSoloDutyPercent()			{return 0.50;}
		static double regularServerDutyPercent()		{return 0.25;}
		static double zeroDutyPercent()				{return 0.0;}
		static double dPhysicsServerDutyPercent();
		static double dPhysicsClientDutyPercent();
		static double dPhysicsClientEThrottleDutyPercent();
	};		// class
}		// namespace;
