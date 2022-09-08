/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "rbx/Debug.h"

// Simple class for returning metric values - used for graphics reporting

namespace RBX {

	class RBXInterface IMetric
	{
	public:
		IMetric() {}
		virtual ~IMetric() {}

		virtual std::string getMetric(const std::string& metric) const = 0;
		virtual double getMetricValue(const std::string& metric) const = 0;
	};
} // namespace
