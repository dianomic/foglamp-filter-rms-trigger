/*
 * FogLAMP "RMS" filter plugin.
 *
 * Copyright (c) 2019 Dianomic Systems
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Mark Riddoch
 */
#include <filter.h>
#include <reading_set.h>
#include <string>
#include <map>
#include <regex>

class RMSFilter : public FogLampFilter {
	public:
		RMSFilter(const std::string& filterName,
			ConfigCategory& filterConfig,
			OUTPUT_HANDLE *outHandle,
			OUTPUT_STREAM out);
		void	ingest(std::vector<Reading *> *in, std::vector<Reading *>& out);
		void	reconfigure(const std::string& newConfig);
	private:
		void	addValue(const std::string& asset, const std::string& dpname, long value);
		void	addValue(const std::string& asset, const std::string& dpname, double value);
		void	outputData(std::vector<Reading *>&, bool triggered);
		bool	sendRawData();
		class RMSData {
			public:
				RMSData() : samples(0), cumulative(0.0)
				{
				};
				unsigned int 	samples;
				double	      	cumulative;
				double		peak_max;
				double		peak_min;
		};
		std::string	m_triggerAsset;
		std::string	m_triggerDatapoint;
		bool		m_sendRawData;
		bool		m_sendPeak;
		std::string	m_assetName;
		std::string	m_assetFilter;
		std::map<std::pair<std::string, std::string>, RMSData *>
				m_values;
		bool		m_triggerNegative;
};
