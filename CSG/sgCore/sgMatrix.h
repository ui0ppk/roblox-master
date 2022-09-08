#ifndef  __sgMatrix__
#define  __sgMatrix__

class sgCore_API sgCMatrix
{
private:
	sgFloat     m_matrix[16];
	sgFloat*    m_temp_buffer;
public:
	sgCMatrix();
	sgCMatrix(const sgFloat*);
	sgCMatrix(sgCMatrix&);
	~sgCMatrix();
	sgCMatrix& operator = (const sgCMatrix& );

	bool  SetMatrix(const sgCMatrix*);

	const sgFloat*   GetData();
	const sgFloat*   GetTransparentData();

	void  Identity();

	void  Transparent();
	bool  Inverse();
	void  Multiply(const sgCMatrix& MatrB);
	void  Translate(const SG_VECTOR& transVector);
	void  Rotate(const SG_POINT& axePoint, const SG_VECTOR& axeDir, sgFloat alpha_radians);
	void  VectorToZAxe(const SG_VECTOR& vect);

	void  ApplyMatrixToVector(SG_POINT& vectBegin, SG_VECTOR& vectDir) const;
	void  ApplyMatrixToPoint(SG_POINT& pnt) const;
};

#endif