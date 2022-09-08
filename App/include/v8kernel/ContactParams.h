/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */

#pragma once

namespace RBX {

	class ContactParams {
	public:
		float kSpring;		// spring constant			
		float kNeg;			// elastic - spring constant on bounceback
		float kFriction;	// contact only variable stored as true value * -0.5;???
        float kElasticity;

		ContactParams() 
			: kSpring(0.0)
			, kFriction(0.0)
			, kNeg(0.0)
            , kElasticity(0.0f)

		{}
	};


	enum GeoPairType {	BALL_POINT_PAIR,			// BALL to BLOCK
								BALL_EDGE_PAIR, 
								BALL_PLANE_PAIR,
															// BLOCK to BLOCK
								POINT_PLANE_PAIR,				
								EDGE_EDGE_PLANE_PAIR,		// two edges, plane needed to supply normal
								EDGE_EDGE_PAIR,				// two edges, guaranteed to be overlapping

								VERTEX_PLANE_CONNECTOR,
								EDGE_EDGE_CONNECTOR,
								EDGE_EDGE_PLANE_CONNECTOR,

								BALL_VERTEX_CONNECTOR,
								BALL_EDGE_CONNECTOR,
								BALL_PLANE_CONNECTOR,

								BULLET_SHAPE_CONNECTOR,
								BULLET_SHAPE_CELL_CONNECTOR	};


} // namespace