#pragma once

// See http://graphics.stanford.edu/courses/cs468-02-fall/readings/lischinski.ps
// And Graphics Gems IV - delaunay

#include "rbx/Debug.h"
#include <limits>
#include "math.h"

namespace GEMS {

	#define GEMS_EPS 1e-6

	typedef float Real;

	class Vector2d {
	public:
		Real x, y;
		Vector2d()					{ x = 0; y = 0; }
		Vector2d(Real a, Real b)	{ x = a; y = b; }
		Real norm() const;
		void normalize();
		Vector2d operator+(const Vector2d&) const;
		Vector2d operator-(const Vector2d&) const;
		friend Vector2d operator*(Real, const Vector2d&);
		friend Real dot(const Vector2d&, const Vector2d&);
	};

	class Point2d {
	public:
		Real x, y;
		Point2d()					{ x = 0; y = 0; }
		Point2d(Real a, Real b)		{ x = a; y = b; }
		Point2d(const Point2d& p)	{ *this = p; }
		Point2d operator+(const Vector2d&) const;
		Vector2d operator-(const Point2d&) const;
		int operator==(const Point2d&) const;
	};

	class Line {
	public:
		Line()	{}
		Line(const Point2d&, const Point2d&);
		Real eval(const Point2d&) const;
		int classify(const Point2d&) const;
	private:
		Real a, b, c;
	};

	// Vector2d:

	inline Real Vector2d::norm() const
	{
		return sqrt(x * x + y * y);
	}

	inline void Vector2d::normalize()
	{
		Real len;

		if ((len = sqrt(x * x + y * y)) == 0.0) {
			RBXASSERT(0);
		}
		else {
			x /= len;
			y /= len;
		}
	}

	inline Vector2d Vector2d::operator+(const Vector2d& v) const
	{
		return Vector2d(x + v.x, y + v.y);
	}

	inline Vector2d Vector2d::operator-(const Vector2d& v) const
	{
		return Vector2d(x - v.x, y - v.y);
	}

	inline Vector2d operator*(Real c, const Vector2d& v)
	{
		return Vector2d(c * v.x, c * v.y);
	}

	inline Real dot(const Vector2d& u, const Vector2d& v)
	{
		return u.x * v.x + u.y * v.y;
	}


	// Point2d:

	inline Point2d Point2d::operator+(const Vector2d& v) const
	{
		return Point2d(x + v.x, y + v.y);
	}

	inline Vector2d Point2d::operator-(const Point2d& p) const
	{
		return Vector2d(x - p.x, y - p.y);
	}

	inline int Point2d::operator==(const Point2d& p) const
	{
		return ((*this - p).norm() < GEMS_EPS);
	}


	// Line:

	inline Line::Line(const Point2d& p, const Point2d& q)
	// Computes the normalized line equation through the
	// points p and q.
	{
		Vector2d t = q - p;
		Real len = t.norm();
		a =   t.y / len;
		b = - t.x / len;
		c = -(a*p.x + b*p.y);
	}

	inline Real Line::eval(const Point2d& p) const
	// Plugs point p into the line equation.
	{
		return (a * p.x + b* p.y + c);
	}

	inline int Line::classify(const Point2d& p) const
	// Returns -1, 0, or 1, if p is to the left of, on,
	// or right of the line, respectively.
	{
		Real d = eval(p);
		return (d < -GEMS_EPS) ? -1 : (d > GEMS_EPS ? 1 : 0);
	}

	class QuadEdge;
	
	class Edge {
		friend class QuadEdge;
		friend void Splice(Edge*, Edge*);
	  private:
		int num;
		Edge *next;
		Point2d *data;
	  public:
		Edge()			{ data = 0; }
		Edge* Rot();
		Edge* invRot();
		Edge* Sym();
		Edge* Onext();
		Edge* Oprev();
		Edge* Dnext();
		Edge* Dprev();
		Edge* Lnext();
		Edge* Lprev();
		Edge* Rnext();
		Edge* Rprev();
		Point2d* Org();
		Point2d* Dest();
		const Point2d& Org2d() const;
		const Point2d& Dest2d() const;
		void  EndPoints(Point2d*, Point2d*);
		QuadEdge* Qedge()				{ return (QuadEdge *)(this - num); }

		template<class Func>
		void visit(Func func);

	};

	class QuadEdge {
		friend Edge *MakeEdge();
	  private:
		Edge e[4];
		static unsigned int globalVisitId;
		unsigned int myVisitId;

	  public:
		QuadEdge() : myVisitId(0) {
			e[0].num = 0, e[1].num = 1, e[2].num = 2, e[3].num = 3;
			e[0].next = &(e[0]); e[1].next = &(e[3]);
			e[2].next = &(e[2]); e[3].next = &(e[1]);
		}

		static void incrementVisitId() {
			globalVisitId++;
		}

		bool visited() {
			if (myVisitId != globalVisitId) {
				myVisitId = globalVisitId;
				return false;
			} else
				return true;
		}
	};

	class Subdivision {
	  private:
		Edge *startingEdge;
		Edge *Locate(const Point2d&);
	  public:
		Subdivision(const Point2d&, const Point2d&, const Point2d&);

		void InsertSite(const Point2d&);

		template<class Func>
		inline void visit(Func func) 
		{
			QuadEdge::incrementVisitId();
			startingEdge->visit<Func>(func);
		}
	};



	/************************* Edge Algebra *************************************/

	inline Edge* Edge::Rot()
	// Return the dual of the current edge, directed from its right to its left.
	{
		return (num < 3) ? this + 1 : this - 3;
	}

	inline Edge* Edge::invRot()
	// Return the dual of the current edge, directed from its left to its right.
	{
		return (num > 0) ? this - 1 : this + 3;
	}

	inline Edge* Edge::Sym()
	// Return the edge from the destination to the origin of the current edge.
	{
		return (num < 2) ? this + 2 : this - 2;
	}

	inline Edge* Edge::Onext()
	// Return the next ccw edge around (from) the origin of the current edge.
	{
		return next;
	}

	inline Edge* Edge::Oprev()
	// Return the next cw edge around (from) the origin of the current edge.
	{
		return Rot()->Onext()->Rot();
	}

	inline Edge* Edge::Dnext()
	// Return the next ccw edge around (into) the destination of the current edge.
	{
		return Sym()->Onext()->Sym();
	}

	inline Edge* Edge::Dprev()
	// Return the next cw edge around (into) the destination of the current edge.
	{
		return invRot()->Onext()->invRot();
	}

	inline Edge* Edge::Lnext()
	// Return the ccw edge around the left face following the current edge.
	{
		return invRot()->Onext()->Rot();
	}

	inline Edge* Edge::Lprev()
	// Return the ccw edge around the left face before the current edge.
	{
		return Onext()->Sym();
	}

	inline Edge* Edge::Rnext()
	// Return the edge around the right face ccw following the current edge.
	{
		return Rot()->Onext()->invRot();
	}

	inline Edge* Edge::Rprev()
	// Return the edge around the right face ccw before the current edge.
	{
		return Sym()->Onext();
	}

	/************** Access to data pointers *************************************/

	inline Point2d* Edge::Org()
	{
		return data;
	}

	inline Point2d* Edge::Dest()
	{
		return Sym()->data;
	}

	inline const Point2d& Edge::Org2d() const
	{
		return *data;
	}

	inline const Point2d& Edge::Dest2d() const
	{
		return (num < 2) ? *((this + 2)->data) : *((this - 2)->data);
	}

	inline void Edge::EndPoints(Point2d* por, Point2d* de)
	{
		data = por;
		Sym()->data = de;
	}
	
	template<class Func>
	void Edge::visit(Func func)
	{
		if (!(Qedge()->visited())) {
			func(this);
			Onext()->visit<Func>(func);
			Oprev()->visit<Func>(func);
			Dnext()->visit<Func>(func);
			Dprev()->visit<Func>(func);
		}
	}

} // namespace

