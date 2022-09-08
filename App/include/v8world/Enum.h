/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

namespace RBX {

	namespace Sim {
		
		typedef enum {		ANCHORED,
							RECURSIVE_WAKE_PENDING,
							WAKE_PENDING,
							AWAKE,
							SLEEPING_CHECKING,
							SLEEPING_DEEPLY,
							REMOVING		}	AssemblyState;

		inline bool isMovingAssemblyState(AssemblyState state) {
			return ((state == AWAKE) || (state == RECURSIVE_WAKE_PENDING) || (state == WAKE_PENDING));
		}

		inline bool isSleepingAssemblyState(AssemblyState state) {
			return ((state == SLEEPING_CHECKING) || (state == SLEEPING_DEEPLY));
		}

		inline bool outOfKernelAssemblyState(AssemblyState state) {
			return (isSleepingAssemblyState(state) || (state == REMOVING));
		}

#ifdef _WIN32
		typedef enum : unsigned char {		CAN_NOT_THROTTLE = 0, 
							                CAN_THROTTLE, 
							                NUM_THROTTLE_TYPE,
							                UNDEFINED_THROTTLE	} ThrottleType;

        typedef enum : unsigned char {		UNDEFINED,
							                STEPPING,
							                SLEEPING,
							                CONTACTING,
							                CONTACTING_SLEEPING} EdgeState;
#else
		typedef enum {		                CAN_NOT_THROTTLE = 0, 
							                CAN_THROTTLE, 
							                NUM_THROTTLE_TYPE,
							                UNDEFINED_THROTTLE	} ThrottleType;

        typedef enum {		                UNDEFINED,
							                STEPPING,
							                SLEEPING,
							                CONTACTING,
							                CONTACTING_SLEEPING} EdgeState;
#endif

	} // namespace WORLD
}// namespace
