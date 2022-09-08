#include <boost/test/unit_test.hpp>

#include "rbx/debug.h"
#include "RbxFormat.h"
#include <iostream>

#include "boost/pool/poolfwd.hpp"
#include "v8datamodel/Filters.h"
#include "v8world/SpatialHashMultiRes.h"

#include "boost/intrusive/list.hpp"
#include <boost/unordered_map.hpp>

using namespace boost::intrusive;


using namespace RBX;

class Prim;

class DummyContact : public std::pair<Prim* const, Prim*>
{
public:
	DummyContact(Prim* const p0, Prim* p1) 
		: std::pair<Prim* const, Prim*>(p0,p1)
	{};
	Prim* otherPrimitive(Prim* p) 
	{
		return p == first ? second : (p == second ? (Prim*)first : NULL);
	}

};

class ContactMan;

typedef std::multimap<Prim*, Prim*> ContactMap;


// contact manager stub.
class ContactMan
{
public:
	ContactMap contacts;
	/////////////////////////////////////////////
	// From the collision engine
	//
	virtual void onNewPair(Prim* p0, Prim* p1) 
	{
		contacts.insert(std::make_pair(p0, p1));
		contacts.insert(std::make_pair(p1, p0));
	};	
	virtual void releasePair(Prim* p0, Prim* p1);
    virtual bool primitiveIsExcludedFromSpatialHash(Prim* p) {return false;}
    virtual void checkTerrainContact(Prim* p) {}
};

// primitive stub.
class Prim : public BasicSpatialHashPrimitive
{
public:
	Prim(ContactMan* c, const Extents& ext)
		:fuzzyExtents(ext)
	{
		contactManager = c;
	}
	
	DummyContact* getFirstContact() {
		
		ContactMap::iterator it = contactManager->contacts.find(this);
		if(it != contactManager->contacts.end())
		{
			return static_cast<DummyContact*>(&(*it));
		}
		else
		{
			return NULL;
		}
	};
	
	static const bool hasGetFirstContact = true;

	DummyContact* getNextContact(DummyContact* p) 
	{
		ContactMap::iterator it = getContactIterator(p->first, p->second);
		++it;
		if(it != contactManager->contacts.end() && it->first == this)
		{
			return static_cast<DummyContact*>(&(*it));
		}
		return 0;
	};

	int getNumContacts()
	{
		return(contactManager->contacts.count(this));
	};

	Prim* getContactOther(int id)
	{
		ContactMap::iterator it = contactManager->contacts.find(this);
		int count = 0;
		while (it != contactManager->contacts.end() && it->first == this && count < id)
		{
			++it;
			++count;
		}
		if (it != contactManager->contacts.end() && it->first == this)
		{
			return static_cast<DummyContact*>(&(*it))->otherPrimitive(this);
		}
		return 0;
	};

	// For fuzzyExtents - when updated
	Extents					fuzzyExtents;
	const Extents& getFastFuzzyExtents() { return fuzzyExtents; };

	bool requestFixed() const { return false; }

	static ContactMap::iterator getContactIterator(Prim* p0, Prim* p1) 
	{  
		ContactMap::iterator it = contactManager->contacts.find(p0);
		while(it != contactManager->contacts.end() && it->first == p0)
		{
			if(it->second == p1)
			{
				return it;
			}
			++it;
		}
		return contactManager->contacts.end();
	};
	static void* getContact(Prim* p0, Prim* p1) 
	{
		if(getContactIterator(p0, p1) != contactManager->contacts.end())
		{
			// just return non null pointer if we found something.
			// this is only used as an existence check by the spatial hash.
			return p0;
		}
		else
		{
			return 0;
		}
	}

	static ContactMan* contactManager;
};

ContactMan* Prim::contactManager = 0;

void ContactMan::releasePair(Prim* p0, Prim* p1) 
{
	contacts.erase(Prim::getContactIterator(p0, p1));
	contacts.erase(Prim::getContactIterator(p1, p0));
};	

typedef RBX::SpatialHash<Prim, DummyContact, ContactMan, 4> SH;

struct PrimitiveCounter : public SH::SpaceFilter
{
	int count;
	PrimitiveCounter()
		: count(0) {};
	void reset() { count = 0; };
	virtual IntersectResult Intersects(const Extents& extents) { return irFull; };
	// return false to break iteration.
	virtual bool onPrimitive(Prim *p, IntersectResult intersectResult, float d) { count++; return true; };
};

struct PrimitiveCoverageCounter : public SH::SpaceFilter
{
	typedef boost::unordered_map<Prim *, int> PrimCountType;
	PrimCountType counts;

	void reset() { counts.clear(); };
	virtual IntersectResult Intersects(const Extents& extents) { return irFull; };
	// return false to break iteration.
	virtual bool onPrimitive(Prim *p, IntersectResult intersectResult, float d) 
	{
		counts[p] = counts[p] + 1;
		return true;
	};
};


BOOST_AUTO_TEST_SUITE( SpatialHash )

const int SH_TEST_MAX_CELLS_PER_PRIMITIVE = 100;		// this is the current number used by the contactManager, - the graphics version uses less

BOOST_AUTO_TEST_CASE( AddRemove )
{
    ContactMan cm;
    SH sh(NULL, &cm, SH_TEST_MAX_CELLS_PER_PRIMITIVE);
	
    Prim prims[] = { Prim(&cm,Extents(Vector3(0,0,0), Vector3(1,1,1))) };
    PrimitiveCounter counter;
	
    sh.onPrimitiveAdded(&prims[0]);
    sh.visitPrimitivesInSpace(&counter);
    BOOST_CHECK_EQUAL(counter.count, 1);
	
    sh.onPrimitiveRemoved(&prims[0]);
    counter.reset();
    sh.visitPrimitivesInSpace(&counter);
    BOOST_CHECK_EQUAL(counter.count, 0);
}


BOOST_AUTO_TEST_CASE( CoverageTest )
{
	// assumes grid size 8x8x8
	ContactMan cm;
	SH sh(NULL, &cm, SH_TEST_MAX_CELLS_PER_PRIMITIVE);
	
	Prim prims[] = {
		Prim(&cm,Extents(Vector3(0,0,0), Vector3(2,2,2))),
		Prim(&cm,Extents(Vector3(0,0,0), Vector3(7,7,7))),
		Prim(&cm,Extents(Vector3(0,0,0), Vector3(15,15,15))),
	};
	PrimitiveCoverageCounter counter;
	
	sh.onPrimitiveAdded(&prims[0]);
	sh.onPrimitiveAdded(&prims[1]);
	sh.onPrimitiveAdded(&prims[2]);
	sh.visitPrimitivesInSpace(&counter);
	BOOST_CHECK_EQUAL(counter.counts.size(), 3);
	BOOST_CHECK_EQUAL(counter.counts[&prims[0]], 1);
	BOOST_CHECK_EQUAL(counter.counts[&prims[1]], 1);
	BOOST_CHECK_EQUAL(counter.counts[&prims[2]], 8);
	
	sh.onPrimitiveRemoved(&prims[0]);
	sh.onPrimitiveRemoved(&prims[1]);
	sh.onPrimitiveRemoved(&prims[2]);
	counter.reset();
	sh.visitPrimitivesInSpace(&counter);
	BOOST_CHECK_EQUAL(counter.counts.size(), 0);
}


BOOST_AUTO_TEST_CASE( TestDegenerate )
{
	bool bAssertsEnabled = false;
	RBXASSERT((bAssertsEnabled = true) != false);

	if(bAssertsEnabled)
	{
		// abort this test, not meant to be run with asserts on.
		BOOST_MESSAGE("Skipping this test because assertions are on");
		BOOST_CHECK(true);
		return;
	}
	// assumes grid size 8x8x8
	ContactMan cm;
	SH sh(NULL, &cm, SH_TEST_MAX_CELLS_PER_PRIMITIVE);

	int maxint = 0x7FFFFFFF;
	Vector3 qnan(std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::quiet_NaN());

	Prim prims[] = { 
		Prim(&cm,Extents(Vector3(1,1,1), Vector3(-1,-1,-1))),
		Prim(&cm,Extents(Vector3(1,1,1), Vector3(1,-7,1))),
		Prim(&cm,Extents(Vector3((float)(maxint-1),(float)(maxint-1),(float)(maxint-1)), Vector3((float)maxint,(float)maxint,(float)maxint))),
		Prim(&cm,Extents(qnan, qnan)),
		Prim(&cm,Extents(Vector3(0,0,0), Vector3(0,0,0))),
		};
	PrimitiveCoverageCounter counter;

	sh.onPrimitiveAdded(&prims[0]);
	sh.onPrimitiveAdded(&prims[1]);
	sh.onPrimitiveAdded(&prims[2]);
	sh.onPrimitiveAdded(&prims[3]);
	sh.onPrimitiveAdded(&prims[4]);
	sh.visitPrimitivesInSpace(&counter);
	BOOST_CHECK_EQUAL(counter.counts.size(), 3);
	BOOST_CHECK_EQUAL(counter.counts[&prims[0]], 0);
	BOOST_CHECK_EQUAL(counter.counts[&prims[1]], 0);
	BOOST_CHECK_EQUAL(counter.counts[&prims[2]], 1);
	BOOST_CHECK_EQUAL(counter.counts[&prims[3]], 1);
	BOOST_CHECK_EQUAL(counter.counts[&prims[4]], 1);


	prims[0].fuzzyExtents = Extents(Vector3(1,1,1),Vector3(1,1,1)); // make extents valid
	prims[1].fuzzyExtents = Extents(Vector3(1,1,1),Vector3(1,-15,1)); // keep extents invalid
	prims[2].fuzzyExtents = Extents(Vector3((float)(maxint-1),(float)(maxint-1),(float)(maxint-1)),Vector3(0,0,0)); // make extents invalid
	prims[3].fuzzyExtents = Extents(Vector3(2,2,2),Vector3(2,2,2)); // make extents valid
	prims[4].fuzzyExtents = Extents(Vector3(2,2,2),qnan); // make extents invalid
	
	sh.onPrimitiveExtentsChanged(&prims[0]);
	sh.onPrimitiveExtentsChanged(&prims[1]);
	sh.onPrimitiveExtentsChanged(&prims[2]);
	sh.onPrimitiveExtentsChanged(&prims[3]);
	sh.onPrimitiveExtentsChanged(&prims[4]);
	counter.reset();
	sh.visitPrimitivesInSpace(&counter);
	BOOST_WARN_EQUAL(counter.counts.size(), 2);
	BOOST_CHECK_EQUAL(counter.counts[&prims[0]], 1); // extents valid again.
	BOOST_CHECK_EQUAL(counter.counts[&prims[1]], 0);
	BOOST_CHECK_EQUAL(counter.counts[&prims[2]], 0);
	BOOST_CHECK_EQUAL(counter.counts[&prims[3]], 1); // qnans are less invalid than inverted bounds. becomes valid again.
	BOOST_WARN_EQUAL(counter.counts[&prims[4]], 0);

	sh.onPrimitiveRemoved(&prims[0]);
	sh.onPrimitiveRemoved(&prims[1]);
	sh.onPrimitiveRemoved(&prims[2]);
	sh.onPrimitiveRemoved(&prims[3]);
	counter.reset();
	sh.visitPrimitivesInSpace(&counter);
	BOOST_WARN_EQUAL(counter.counts.size(), 0);
}

float frand(double min, double max)
{
	double lerp = rand() * (1.0/RAND_MAX);
	return (float)(min * lerp + max * (1-lerp));
}

Extents RandomExtent(int maxsize, int minbound, int maxbound)
{
	Vector3 hsize(frand(0, maxsize/2), frand(0, maxsize/2), frand(0, maxsize/2));
	Vector3 pos(frand(minbound, maxbound), frand(minbound, maxbound), frand(minbound, maxbound));
	Vector3 min = pos-hsize;
	Vector3 max = pos+hsize;

	// clamp to bounds.
	min.x = std::max((float)minbound, min.x);
	min.y = std::max((float)minbound, min.y);
	min.z = std::max((float)minbound, min.z);
	max.x = std::min((float)maxbound, max.x);
	max.y = std::min((float)maxbound, max.y);
	max.z = std::min((float)maxbound, max.z);

	return Extents(min, max);
}


BOOST_AUTO_TEST_CASE( RandomBlast )
{
	int maxsize = 100;
	int minbound = -500;
	int maxbound = 500;
	size_t numprims = 100;

	ContactMan cm;
	SH sh(NULL, &cm, SH_TEST_MAX_CELLS_PER_PRIMITIVE);

	std::vector<Prim> prims;
	prims.reserve(numprims); //critical. we don't want re-allocs.
	PrimitiveCounter counter;
	PrimitiveCoverageCounter counterUnique;

	for(size_t i = 0; i < numprims; ++i)
	{
		prims.push_back(Prim(&cm,RandomExtent(maxsize, minbound, maxbound)));
		sh.onPrimitiveAdded(&prims[i]);
#ifdef _RBX_DEBUGGING_SPATIAL_HASH
		BOOST_CHECK(sh._validate(&prims[i]));
#endif
	}
	
	counterUnique.reset();
	sh.visitPrimitivesInSpace(&counterUnique);
	BOOST_CHECK_EQUAL(counterUnique.counts.size(), numprims);

	for(size_t i = 0; i < numprims; ++i)
	{
		int randi = rand() % prims.size();
		prims[randi].fuzzyExtents = RandomExtent(maxsize, minbound, maxbound);
		sh.onPrimitiveExtentsChanged(&prims[randi]);
#ifdef _RBX_DEBUGGING_SPATIAL_HASH
		BOOST_CHECK(sh._validate(&prims[randi]));
#endif
	}

	Vector3int32 grid(0x80000000, 0x80000000, 0x80000000);
	G3D::Array<Prim*> primitives;
	sh.getPrimitivesInGrid(grid, primitives);

	counterUnique.reset();
	sh.visitPrimitivesInSpace(&counterUnique);
	BOOST_CHECK_EQUAL(counterUnique.counts.size(), numprims);

	for(size_t i = 0; i < numprims; ++i)
	{
#ifdef _RBX_DEBUGGING_SPATIAL_HASH
		BOOST_CHECK(sh._validate(&prims[i]));
#endif
		sh.onPrimitiveRemoved(&prims[i]);
	}

	counter.reset();
	sh.visitPrimitivesInSpace(&counter);
	BOOST_CHECK_EQUAL(counter.count, 0);

}

BOOST_AUTO_TEST_SUITE_END()
