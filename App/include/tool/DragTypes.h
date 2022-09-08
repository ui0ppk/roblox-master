/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

namespace RBX {
	namespace DRAG {

		typedef enum {NO_UNJOIN_NO_JOIN, UNJOIN_JOIN, UNJOIN_NO_JOIN} JoinType;

		typedef enum {MOVE_DROP, MOVE_NO_DROP} MoveType;

		typedef enum {WEAK_MANUAL_JOINT, STRONG_MANUAL_JOINT, INFINITE_MANUAL_JOINT} ManualJointType;

		typedef enum {ONE_STUD, QUARTER_STUD, OFF} DraggerGridMode;

	}
} // namespace