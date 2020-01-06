/*
 * FogLAMP "RMS" filter plugin.
 *
 * Copyright (c) 2019 Dianomic Systems
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Mark Riddoch
 */

#include <plugin_api.h>
#include <config_category.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string>
#include <iostream>
#include <filter_plugin.h>
#include <filter.h>
#include <reading_set.h>
#include <logger.h>
#include "rms.h"
#include <version.h>


#define FILTER_NAME "rms-trigger"
const char *default_config = QUOTE({
		"plugin" : { 
			"description" : "Calculate RMS & Peak values over a set of samples",
                       	"type" : "string",
			"default" : FILTER_NAME,
			"readonly" : "true"
			},
		"triggerAsset" : {
			"description" : "Name of asset that triggers RMS calculation.",
			"type": "string",
			"default": "",
			"order" : "1",
			"displayName" : " Trigger Asset"
			},
		"triggerDatapoint" : {
			"description" : "Name of datapoint that triggers RMS calculation.",
			"type": "string",
			"default": "",
			"order" : "2",
			"displayName" : "Trigger Datapoint"
			},
		"triggerType" : {
			"description" : "The type of trigger event.",
			"type": "enumeration",
			"options" : [ "zero crossing", "peak", "rapid edge" ],
			"default": "zero crossing",
			"order" : "3",
			"displayName" : "Trigger Type"
			},
		"triggerEdge" : {
			"description" : "The trigger edge direction.",
			"type": "enumeration",
			"options" : [ "rising", "falling" ],
			"default": "rising",
			"order" : "4",
			"displayName" : "Trigger Edge"
			},
		"assetName": {
			"description": "Name of the output asset for the RMS data",
			"type": "string",
			"default": "%a RMS",
			"order": "5",
			"displayName": "RMS Asset name"
			},
		"peak": {
			"description": "Include peak to peak values in readings",
			"type": "boolean",
			"default": "false",
			"order" : "6",
			"displayName": "Include Peak Values"
			},
		"rawData": {
			"description": "Switch to control the inclusion of the raw data in the output",
			"type": "boolean",
			"default": "false",
			"order" : "7",
			"displayName": "Include Raw Data"
			},
		"match": {
			"description": "An optional regular expression to match in the asset name",
			"type": "string",
			"default": ".*",
			"order": "8",
			"displayName": "Asset filter"
			},
		"addSampleNo": {
			"description": "Add a monotonic sample number to each RMS value and corresponding raw data",
			"type": "boolean",
			"default": "false",
			"order" : "9",
			"displayName": "Add Sample No."
			},
		"sampleName": {
			"description": "Name of distret sample number",
			"type": "string",
			"default": "partNo",
			"order" : "10",
			"displayName": "Sample Name."
			},
		"enable": {
			"description": "A switch that can be used to enable or disable execution of the RMS filter.",
			"type": "boolean",
			"displayName": "Enabled",
			"default": "false",
			"order": "11"
			}
		});

using namespace std;

/**
 * The Filter plugin interface
 */
extern "C" {

/**
 * The plugin information structure
 */
static PLUGIN_INFORMATION info = {
        FILTER_NAME,              // Name
        VERSION,                  // Version
        0,                        // Flags
        PLUGIN_TYPE_FILTER,       // Type
        "1.0.0",                  // Interface version
	default_config	          // Default plugin configuration
};

typedef struct
{
	RMSFilter	*handle;
	std::string	configCatName;
} FILTER_INFO;

/**
 * Return the information about this plugin
 */
PLUGIN_INFORMATION *plugin_info()
{
	return &info;
}

/**
 * Initialise the plugin, called to get the plugin handle and setup the
 * output handle that will be passed to the output stream. The output stream
 * is merely a function pointer that is called with the output handle and
 * the new set of readings generated by the plugin.
 *     (*output)(outHandle, readings);
 * Note that the plugin may not call the output stream if the result of
 * the filtering is that no readings are to be sent onwards in the chain.
 * This allows the plugin to discard data or to buffer it for aggregation
 * with data that follows in subsequent calls
 *
 * @param config	The configuration category for the filter
 * @param outHandle	A handle that will be passed to the output stream
 * @param output	The output stream (function pointer) to which data is passed
 * @return		An opaque handle that is used in all subsequent calls to the plugin
 */
PLUGIN_HANDLE plugin_init(ConfigCategory* config,
			  OUTPUT_HANDLE *outHandle,
			  OUTPUT_STREAM output)
{
	FILTER_INFO *info = new FILTER_INFO;
	info->handle = new RMSFilter(FILTER_NAME,
					*config,
					outHandle,
					output);
	info->configCatName = config->getName();

	return (PLUGIN_HANDLE)info;
}

/**
 * Ingest a set of readings into the plugin for processing
 *
 * @param handle	The plugin handle returned from plugin_init
 * @param readingSet	The readings to process
 */
void plugin_ingest(PLUGIN_HANDLE *handle,
		   READINGSET *readingSet)
{
	FILTER_INFO *info = (FILTER_INFO *) handle;
	RMSFilter *filter = info->handle;
	if (!filter->isEnabled())
	{
		// Current filter is not active: just pass the readings set
		filter->m_func(filter->m_data, readingSet);
		return;
	}

	vector<Reading *>out;
	filter->ingest(readingSet->getAllReadingsPtr(), out);
	delete (ReadingSet *)readingSet;

	ReadingSet *newReadingSet = new ReadingSet(&out);
	const vector<Reading *>& readings = newReadingSet->getAllReadings();
	// Iterate over the readings
	for (vector<Reading *>::const_iterator elem = readings.begin();
						      elem != readings.end();
						      ++elem)
	{
		AssetTracker::getAssetTracker()->addAssetTrackingTuple(info->configCatName, (*elem)->getAssetName(), string("Filter"));
	}
		
	filter->m_func(filter->m_data, newReadingSet);
}

/**
 * Reconfigure the plugin
 *
 * @param handle	The plugin handle
 * @param bewConfig	The new configuration
 */
void plugin_reconfigure(PLUGIN_HANDLE *handle, const string& newConfig)
{
	FILTER_INFO *info = (FILTER_INFO *)handle;
	RMSFilter* data = info->handle;
	data->reconfigure(newConfig);
}

/**
 * Call the shutdown method in the plugin
 */
void plugin_shutdown(PLUGIN_HANDLE *handle)
{
	FILTER_INFO *info = (FILTER_INFO *) handle;
	delete info->handle;
	delete info;
}

// End of extern "C"
};
