#include <openxr/openxr.h>
#include "external/loader_interfaces.h"

#include <iostream>

extern "C" {
XrResult
xrNegotiateLoaderApiLayerInterface(
	const XrNegotiateLoaderInfo *loaderInfo,
	const char *layerName,
	XrNegotiateApiLayerRequest *apiLayerRequest)
{
	std::cout << layerName << ": Using API layer: " << layerName << std::endl;
	return XR_SUCCESS;
}

}
