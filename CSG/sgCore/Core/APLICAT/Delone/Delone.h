#ifndef  __DELONE__
#define  __DELONE__

class CTriangulator
{
public:
	CTriangulator();
	CTriangulator(unsigned int points_count);
	CTriangulator(unsigned int points_count, const sgFloat* all_points);
	~CTriangulator();

	bool         ClearAll();

	bool         ClearNotAll();

	bool         SetPointsCount(unsigned int points_count);
	bool         SetPoint(unsigned int point_index, sgFloat pX, sgFloat pY,
									sgFloat point_attr_1=0.0, 
									sgFloat point_attr_2=0.0, 
									sgFloat point_attr_3=0.0,  
									int point_mark=0);
	bool         SetAllPoints(unsigned int points_count, const sgFloat* all_points,
									const sgFloat* points_attrs=NULL, 
									const int* points_marks=NULL);
	
	void         CorrectPointsCollisions();

	bool         SetSegmentsCount(unsigned int segments_count);
	bool         SetSegment(unsigned int segment_index, int start_ind, int end_ind,
									int segment_mark=0);
	bool         SetAllSegments(unsigned int segments_count, const int* all_segments,
									const int* segments_marks=NULL);


	bool         SetHolesCount(unsigned int holes_count);
	bool         SetHole(unsigned int hole_index, sgFloat pX, sgFloat pY);
	bool         SetAllHoles(unsigned int holes_count, const sgFloat* all_holes);

	bool         StartTriangulate();

	bool         GetPointsCount(unsigned int& points_count) const;
	bool         GetPoint(unsigned int point_index, sgFloat& pX, sgFloat& pY) const;
	const sgFloat* GetPointsAttributes() const;
	const int*    GetPointsMarks() const;


	bool         GetTrianglesCount(unsigned int& tr_count) const;
	bool         GetTriangle(unsigned int tr_index, int& ver1, int& ver2, int& ver3) const;


private:
	void*        m_in;
	void*        m_out;
};

#endif
