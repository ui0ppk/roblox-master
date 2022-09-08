#pragma once

#include "rbx/Debug.h"
#include "rbx/atomic.h"


#ifdef __RBX_NOT_RELEASE
	#define RBX_USE_CONCURRENCY_VALIDATOR(expr) (expr)
#else
	#define RBX_USE_CONCURRENCY_VALIDATOR(expr) ((void)0)
#endif



namespace RBX { 

class ConcurrencyValidator
{
private:
    rbx::atomic<int> writing;
	std::string writeLocation;
    mutable rbx::atomic<int> reading;

public:
	ConcurrencyValidator() : writing(0), reading(0)
	{
	}

	~ConcurrencyValidator() {
		RBXASSERT(writing == 0);
		RBXASSERT(reading == 0);
	}

private:
	friend class WriteValidator;
	friend class ReadOnlyValidator;

	bool preRead() const {
		++reading;

		long wasWriting = writing;
		if (wasWriting != 0) {RBXASSERT(false && "writing check failed in preRead");}
		return true;
	}

	bool postRead() const {
		long wasWriting = writing;
		if (wasWriting != 0)	{RBXASSERT(false && "wasWriting check failed in postRead");}
		if (writing != 0)		{RBXASSERT(false && "writing check failed in postRead");}

		--reading;
		return true;
	}

	bool preWrite() {
		long wasWriting = writing;
		long wasReading = reading;

		if (wasReading != 0)	{RBXASSERT(false && "wasReading check failed in preWrite");}
		if (wasWriting != 0)	{RBXASSERT(false && "wasWriting check failed in preWrite");}
		if (writing != 0)		{RBXASSERT(false && "writing check failed in preWrite");}

		long result = ++writing;

		if (result != 1)		{RBXASSERT(false && "InterlocedIncrement returned not -1 in preWrite");}
		return true;
	}

	bool preWrite(const std::string& writeWhere) {
		bool okWrite = preWrite();
		if (okWrite) {
			writeLocation = writeWhere;
		}
		return okWrite;
	}

	bool postWrite() {
		long wasWriting = writing;
		long wasReading = reading;

		if (wasWriting != 1)	{RBXASSERT(false && "wasWriting check failed in postWrite");}
		if (writing != 1)		{RBXASSERT(false && "writing check failed in postWrite");}
		if (wasReading != 0)	{RBXASSERT(false && "wasReading check failed in postWrite");}
		if (reading != 0)		{RBXASSERT(false && "reading check failed in postWrite");}

		long result = --writing;
		if (result != 0)		{RBXASSERT(false && "InterlocedIncrement returned not 0 in postWrite");}
		if (reading != 0)		{RBXASSERT(false && "reading check failed in postWrite");}
		return true;
	}
};

class ReadOnlyValidator
{
private:
	const ConcurrencyValidator& concurrencyValidator;

public:
	ReadOnlyValidator(const ConcurrencyValidator& c) : concurrencyValidator(c) {
		RBX_USE_CONCURRENCY_VALIDATOR(concurrencyValidator.preRead());
	}

	~ReadOnlyValidator() {
		RBX_USE_CONCURRENCY_VALIDATOR(concurrencyValidator.postRead());
	}
};


class WriteValidator
{
private:
	ConcurrencyValidator& concurrencyValidator;

public:
	WriteValidator(ConcurrencyValidator& c) : concurrencyValidator(c) {
		RBX_USE_CONCURRENCY_VALIDATOR(concurrencyValidator.preWrite());
	}

	WriteValidator(ConcurrencyValidator& c, const std::string& writeWhere) : concurrencyValidator(c) {
		RBX_USE_CONCURRENCY_VALIDATOR(concurrencyValidator.preWrite(writeWhere));
	}

	WriteValidator(ConcurrencyValidator& c, const char* writeWhere) : concurrencyValidator(c) {
		RBX_USE_CONCURRENCY_VALIDATOR(concurrencyValidator.preWrite(writeWhere));
	}

	~WriteValidator() {
		RBX_USE_CONCURRENCY_VALIDATOR(concurrencyValidator.postWrite());
	}
};


}	// namespace

