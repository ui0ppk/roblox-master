/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#pragma once

template<class T>
class Vector6
{
private:
	T	data[6];

public:
	Vector6() {;}										// no initialization
	Vector6(const T& setAll)
	{
		data[0] = data[1] = data[2] = data[3] = data[4] = data[5] = setAll;
	}
//	Vector6(const Vector6<T>& _other)					// use bit copy
//	Vector6& operator= (const Vector6<t>& _other);		// use bit copy
//	virtual ~Template();								// no destructor

	const T& operator[](int i) const 
	{
		return data[i];
	}

	T& operator[](int i) 
	{
	    return data[i];
	}
};
