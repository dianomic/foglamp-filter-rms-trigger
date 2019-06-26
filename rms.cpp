/*
 * FogLAMP "RMS" filter plugin.
 *
 * Copyright (c) 2019 Dianomic Systems
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Mark Riddoch
 */
#include <reading.h>
#include <reading_set.h>
#include <utility>
#include <logger.h>
#include "rms.h"
#include <regex>

using namespace std;

/**
 * Constructor for the RMS Filter class
 *
 * The filter is called with each number data point/reading and
 * will calculate the RMS (Root Mean Squared) value of each data point
 * in the reading over a specified number of samples.
 *
 * The RMS value is calculated by taking a cumulative squared value from
 * each reading up to the point that enough readings have been taken.
 * Once enough readings have been taken the cumulative value is divided
 * by the sample count to get the RMS value. This RMS value is passed up
 * the filter chain as a new reading. The cumulative value is reset to
 * zero and the process starts again for the next RMS value.
 *
 * The filter can also optionally pass the raw data along the filter
 * pipeline if it has been configured to send raw data.
 */
RMSFilter::RMSFilter(const std::string& filterName,
		     ConfigCategory& filterConfig,
		     OUTPUT_HANDLE *outHandle,
		     OUTPUT_STREAM out) :
				FogLampFilter(filterName, filterConfig,
						outHandle, out)
{
	m_sampleNo = 0;
	if (filterConfig.itemExists("assetName"))
	{
		m_assetName = filterConfig.getValue("assetName");
	}
	else
	{
		m_assetName = "RMS";
	}
	if (filterConfig.itemExists("match"))
	{
		m_assetFilter = filterConfig.getValue("match");
	}
	else
	{
		m_assetFilter = ".*";
	}
	if (filterConfig.itemExists("triggerAsset"))
	{
		m_triggerAsset = filterConfig.getValue("triggerAsset");
	}
	else
	{
		m_triggerAsset = "";
	}
	if (filterConfig.itemExists("triggerDatapoint"))
	{
		m_triggerDatapoint = filterConfig.getValue("triggerDatapoint");
	}
	else
	{
		m_triggerDatapoint = "";
	}
	if (filterConfig.itemExists("triggerType"))
	{
		string type = m_config.getValue("triggerType");
		if (type.compare("zero crossing") == 0)
		{
			m_triggerZC = true;
		}
		else
		{
			m_triggerZC = false;
		}
		if (type.compare("rapid edge") == 0)
		{
			m_triggerRapid = true;
		}
		else
		{
			m_triggerRapid = false;
		}
	}
	if (filterConfig.itemExists("triggerEdge"))
	{
		string type = m_config.getValue("triggerEdge");
		if (type.compare("rising") == 0)
		{
			m_triggerRise = true;
		}
		else
		{
			m_triggerRise = false;
		}
	}
	if (filterConfig.itemExists("addSampleNo"))
	{
		m_addSampleNo = filterConfig.getValue("addSampleNo").compare("true") == 0 ? true : false;
	}
	else
	{
		m_addSampleNo = false;
	}
	if (filterConfig.itemExists("sampleName"))
	{
		m_sampleName = filterConfig.getValue("sampleName");
	}
	else
	{
		m_sampleName = "partNo";
	}
	if (filterConfig.itemExists("rawData"))
	{
		m_sendRawData = filterConfig.getValue("rawData").compare("true") == 0 ? true : false;
	}
	else
	{
		m_sendRawData = false;
	}
	if (filterConfig.itemExists("peak"))
	{
		m_sendPeak = filterConfig.getValue("peak").compare("true") == 0 ? true : false;
	}
	else
	{
		m_sendPeak = false;
	}
}

/**
 * Ingest data into the plugin and write the processed data to the out vector
 *
 * @param readings	The readings to process
 * @param out		The output readings vector
 */
void
RMSFilter::ingest(vector<Reading *> *readings, vector<Reading *>& out)
{
regex	*re = 0;

	if (m_assetFilter.compare(".*"))
	{
		re = new regex(m_assetFilter);
	}
	// Iterate over the readings
	for (vector<Reading *>::const_iterator elem = readings->begin();
						      elem != readings->end();
						      ++elem)
	{
		bool triggered = false;
		const string& asset = (*elem)->getAssetName();
		if (re && regex_match(asset, *re) == false)
		{
			// Does not match, pass through
			out.push_back(*elem);
		}
		else
		{
			// Iterate over the datapoints
			const vector<Datapoint *>& dataPoints = (*elem)->getReadingData();
			for (vector<Datapoint *>::const_iterator it = dataPoints.cbegin(); it != dataPoints.cend(); ++it)
			{
				// Get the reference to a DataPointValue
				DatapointValue& value = (*it)->getData();

				// If INTEGER or FLOAT do the change
				if (value.getType() == DatapointValue::T_INTEGER)
				{
					addValue(asset, (*it)->getName(), value.toInt());
					if ((*it)->getName().compare(m_triggerDatapoint) == 0 &&
							asset.compare(m_triggerAsset) == 0)
					{
						if (hasTriggered(value))
						{
							triggered = true;
						}
					}
				}
				else if (value.getType() == DatapointValue::T_FLOAT)
				{
					addValue(asset, (*it)->getName(), value.toDouble());
					if ((*it)->getName().compare(m_triggerDatapoint) == 0 &&
							asset.compare(m_triggerAsset) == 0)
					{
						if (hasTriggered(value))
						{
							triggered = true;
						}
					}
				}
				else
				{
					// do nothing
				}
			}
			if (sendRawData())
			{
				DatapointValue  sampleNo(m_sampleNo);
				(*elem)->addDatapoint(new Datapoint(m_sampleName, sampleNo));
				out.push_back(*elem);
			}
			else
			{
				delete *elem;
			}
		}
		outputData(out, triggered);
	}
	readings->clear();	// Prevent double deletes

	if (re)
		delete re;
}

/**
 * Add a sample value to the RMS cumulative values
 * @param name	The name of the value, i.e. the datapoint
 * @param dpname The name of the data point
 * @param value	The value to add
 */
void
RMSFilter::addValue(const string& asset, const string& dpname, long value)
{
double	dvalue = (double)value;

	addValue(asset, dpname, dvalue);
}

/**
 * Add a sample value to the RMS cumulative values
 * @param name	The name of the value, i.e. the datapoint
 * @param dpname The name of the data point
 * @param value	The value to add
 */
void
RMSFilter::addValue(const string& asset, const string& dpname, double value)
{
map<pair<string, string>, RMSData *>::iterator it;
pair<string, string>	key = make_pair(asset, dpname);

	if ((it = m_values.find(key)) == m_values.end())
	{
		m_values.insert(pair<pair<string, string>, RMSData *>(key, new RMSData));
		it = m_values.find(key);
		it->second->peak_max = value;
		it->second->peak_min = value;
	}
	it->second->cumulative += (value * value);
	it->second->samples++;
	if (it->second->samples == 0 || it->second->peak_max < value)
		it->second->peak_max = value;
	if (it->second->samples == 0 || it->second->peak_min > value)
		it->second->peak_min = value;
}

/**
 * Called to output any RMS values that can be sent.
 * We only output values when we get the number defined as
 * the sample size.
 *
 * The output mechanism involves appending RMS values to a
 * ReadingSet instance. Not every call will result in new RMS
 * values being appended to the ReadingSet.
 *
 * @param readingSet	A reading set to which any RMS values are appened
 */
void
RMSFilter::outputData(vector<Reading *>& out, bool triggered)
{
vector<Datapoint *>	dataPoints;
map<string, Reading *>	readings;

	for (auto it = m_values.cbegin(); it != m_values.cend(); it++)
	{
		if (triggered)
		{
			double value = it->second->cumulative / it->second->samples;
			value = sqrt(value);
			it->second->cumulative = 0.0;
			it->second->samples = 0;
			DatapointValue	dpv(value);
			DatapointValue  peak(it->second->peak_max - it->second->peak_min);
			DatapointValue  sampleNo(m_sampleNo++);

			string assetName = m_assetName;
			/*
			 * Check for a %a in the new name and substitute with
			 * the asset name
			 */
			if (assetName.find("%a") != string::npos)
			{
				assetName.replace(assetName.find("%a"), 2, it->first.first);
			}

			map<string, Reading *>::iterator ait = readings.find(it->first.first);
			if (ait != readings.end())
			{
				ait->second->addDatapoint(new Datapoint(it->first.second, dpv));
				if (m_sendPeak)
				{
					ait->second->addDatapoint(new Datapoint(it->first.second + "peak", peak));
				}
			}
			else
			{
				Reading *tmpReading = new Reading(assetName, new Datapoint(it->first.second, dpv));
				if (m_sendPeak)
				{
					tmpReading->addDatapoint(new Datapoint(it->first.second + "peak", peak));
				}
				if (m_addSampleNo)
				{
					tmpReading->addDatapoint(new Datapoint(m_sampleName, sampleNo));
				}
				readings.insert(pair<string, Reading *>(it->first.first, tmpReading));
			}
		}
	}

	// Move all the new RMS values into the output
	for (auto it = readings.cbegin(); it != readings.cend(); it++)
	{
		out.push_back(it->second);
	}
}

/**
 * Indicates if the raw inut data should be output in
 * addition to the RMS values.
 */
bool
RMSFilter::sendRawData()
{
	return m_sendRawData;
}

/**
 * Reconfigure the RMS filter
 *
 * @param newConfig	The new configuration
 */
void
RMSFilter::reconfigure(const string& newConfig)
{
	setConfig(newConfig);
	if (m_config.itemExists("assetName"))
	{
		m_assetName = m_config.getValue("assetName");
	}
	else
	{
		m_assetName = "RMS";
	}
	if (m_config.itemExists("match"))
	{
		m_assetFilter = m_config.getValue("match");
	}
	else
	{
		m_assetFilter = ".*";
	}
	if (m_config.itemExists("triggerAsset"))
	{
		m_triggerAsset = m_config.getValue("triggerAsset");
	}
	else
	{
		m_triggerAsset = "";
	}
	if (m_config.itemExists("triggerDatapoint"))
	{
		m_triggerDatapoint = m_config.getValue("triggerDatapoint");
	}
	else
	{
		m_triggerDatapoint = "";
	}
	if (m_config.itemExists("triggerType"))
	{
		string type = m_config.getValue("triggerType");
		if (type.compare("zero crossing") == 0)
		{
			m_triggerZC = true;
		}
		else
		{
			m_triggerZC = false;
		}
		if (type.compare("rapid edge") == 0)
		{
			m_triggerRapid = true;
		}
		else
		{
			m_triggerRapid = false;
		}
	}
	if (m_config.itemExists("triggerEdge"))
	{
		string type = m_config.getValue("triggerEdge");
		if (type.compare("rising") == 0)
		{
			m_triggerRise = true;
		}
		else
		{
			m_triggerRise = false;
		}
	}
	if (m_config.itemExists("addSampleNo"))
	{
		m_addSampleNo = m_config.getValue("addSampleNo").compare("true") == 0 ? true : false;
	}
	else
	{
		m_addSampleNo = false;
	}
	if (m_config.itemExists("sampleName"))
	{
		m_sampleName = m_config.getValue("sampleName");
	}
	else
	{
		m_sampleName = "partNo";
	}
	if (m_config.itemExists("rawData"))
	{
		m_sendRawData = m_config.getValue("rawData").compare("true") == 0 ? true : false;
	}
	else
	{
		m_sendRawData = false;
	}
	if (m_config.itemExists("peak"))
	{
		m_sendPeak = m_config.getValue("peak").compare("true") == 0 ? true : false;
	}
	else
	{
		m_sendPeak = false;
	}
}

/**
 * Determine if the trigger condition has been met.
 *
 * @param value	The datapoint value of the trigger datapoint
 * @param true if the trigger condition has been met
 */
bool RMSFilter::hasTriggered(DatapointValue& value)
{
bool triggered = false;
double val;

	if (value.getType() == DatapointValue::T_INTEGER)
	{
		val = value.toInt();
	}
	else if (value.getType() == DatapointValue::T_FLOAT)
	{
		val = value.toDouble();
	}
	if (m_triggerZC)
	{
		if (m_triggerRise && m_triggerNegative && val >= 0)
		{
			triggered = true;
		}
		if (m_triggerRise == false && m_triggerNegative == false && val <= 0)
		{
			triggered = true;
		}
		m_triggerNegative = (val < 0);
	}
	else if (m_triggerRapid)
	{
		if (m_triggerRise && val - m_lastTrigger > 1000)
		{
			triggered = true;
		}
		else if (m_triggerRise == false && m_lastTrigger - val > 1000)
		{
			triggered = true;
		}
		m_lastTrigger = val;
	}
	else
	{
		if (m_triggerRise && m_triggerDecreasing == false && val < m_lastTrigger)
		{
			triggered = true;
		}
		if (m_triggerRise == false && m_triggerDecreasing && val > m_lastTrigger)
		{
			triggered = true;
		}
		m_triggerDecreasing = (val < m_lastTrigger);
		m_lastTrigger = val;
	}
	return triggered;
}
