/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "rbx/Debug.h"
#include <set>
#include "rbx/Boost.hpp"
#include "rbx/rbxTime.h"

namespace RBX {

	class IMovingManager;
    class MovementHistory;
    class Velocity;
    class Primitive;

	class RBXBaseClass IMoving
	{
		friend class IMovingManager;

	private:
        IMovingManager* iMovingManager;

        int stepsToSleep;

        scoped_ptr<CoordinateFrame> lastCFrame;
        Time lastUpdateTime;
        scoped_ptr<MovementHistory> movementHistory;

		void makeMoving();

	protected:
		virtual void onSleepingChanged(bool sleeping) = 0;

		void setMovingManager(IMovingManager* _iMovingManager);

		bool checkSleep();

	public:
		IMoving();

		~IMoving();

		void notifyMoved();		// done in PartInstance::setCoordinateFrame, InterpolatedCFrame, and by the World after every step

		virtual bool reportTouches() const = 0;

		virtual void onClumpChanged() = 0;						// callback to PartInstance from Primitive 

		virtual void onNetworkIsSleepingChanged(Time now) = 0;			// callback to PartInstance from Primitive 

		virtual void onBuoyancyChanged( bool value ) = 0;		// callback to PartInstance from Primitive
        virtual bool isInContinousMotion() = 0;

        virtual const Primitive* getConstPartPrimitiveVirtual() const {return NULL;}

		bool getSleeping() const {
			return (stepsToSleep == 0);
		}

		void forceSleep();

        const MovementHistory& getMovementHistory() const;
        void clearMovementHistory();
        void addMovementNode(const CoordinateFrame& cFrame, const Velocity& velocity, const Time& timeStamp);
        void setLastCFrame(const CoordinateFrame& cFrame);
        const CoordinateFrame& getLastCFrame(const CoordinateFrame& defaultCFrame) const;
        bool hasLastCFrame() {return lastCFrame != NULL;}
        void setLastUpdateTime(const Time& time);
        const Time& getLastUpdateTime() const;
	};

	class RBXBaseClass IMovingManager 
	{
		friend class IMoving;
	private:
		typedef std::set<IMoving*> MovingSet;
		MovingSet moving;
		MovingSet::iterator current;

	protected:
		void remove(IMoving* iMoving);
		void moved(IMoving* iMoving);

	public:
		IMovingManager();

		virtual ~IMovingManager();

		void onMovingHeartbeat();		// put parts to sleep here if not moving for a long time, notify

		int getNumberMoving() const {return static_cast<int>(moving.size());}

        void updateHistory();
	};

} // namespace