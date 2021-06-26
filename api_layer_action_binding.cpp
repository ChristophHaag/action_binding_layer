#include <openxr/openxr.h>
#include "external/loader_interfaces.h"

#include <string.h>

#include <iostream>

extern "C" {

static const char *_layerName = NULL;;
static PFN_xrGetInstanceProcAddr _cachedNextXrGetInstanceProcAddr = NULL;

static XRAPI_ATTR XrResult XRAPI_CALL
_xrGetInstanceProcAddr(
	XrInstance                                  instance,
	const char*                                 name,
	PFN_xrVoidFunction*                         function)
{
	// std::cout << _layerName << ": " << name << std::endl;

	_cachedNextXrGetInstanceProcAddr(instance, name, function);

	return XR_SUCCESS;
}

static XrResult XRAPI_PTR
_xrCreateApiLayerInstance(const XrInstanceCreateInfo *info,
	const XrApiLayerCreateInfo *apiLayerInfo, XrInstance *instance)
{
	PFN_xrGetInstanceProcAddr nextXrGetInstanceProcAddr = apiLayerInfo->nextInfo->nextGetInstanceProcAddr;
	PFN_xrCreateApiLayerInstance nextXrCreateApiLayerInstance = apiLayerInfo->nextInfo->nextCreateApiLayerInstance;


	_cachedNextXrGetInstanceProcAddr = nextXrGetInstanceProcAddr;

	std::cout << _layerName << ": Created api layer instance" << std::endl;

	return nextXrCreateApiLayerInstance(info, apiLayerInfo, instance);
}

XrResult
xrNegotiateLoaderApiLayerInterface(
	const XrNegotiateLoaderInfo *loaderInfo,
	const char *layerName,
	XrNegotiateApiLayerRequest *apiLayerRequest)
{
	_layerName = strdup(layerName);

	std::cout << layerName << ": Using API layer: " << layerName << std::endl;

	std::cout << layerName << ": loader API version min: " <<
		XR_VERSION_MAJOR(loaderInfo->minApiVersion) << "." <<
		XR_VERSION_MINOR(loaderInfo->minApiVersion) << "." <<
		XR_VERSION_PATCH(loaderInfo->minApiVersion) << "." <<
		" max: " <<
		XR_VERSION_MAJOR(loaderInfo->maxApiVersion) << "." <<
		XR_VERSION_MINOR(loaderInfo->maxApiVersion) << "." <<
		XR_VERSION_PATCH(loaderInfo->maxApiVersion) << "." << std::endl;

	std::cout << layerName << ": loader interface version min: " <<
	XR_VERSION_MAJOR(loaderInfo->minInterfaceVersion) << "." <<
		XR_VERSION_MINOR(loaderInfo->minInterfaceVersion) << "." <<
		XR_VERSION_PATCH(loaderInfo->minInterfaceVersion) << "." <<
		" max: " <<
		XR_VERSION_MAJOR(loaderInfo->maxInterfaceVersion) << "." <<
		XR_VERSION_MINOR(loaderInfo->maxInterfaceVersion) << "." <<
		XR_VERSION_PATCH(loaderInfo->maxInterfaceVersion) << "." << std::endl;



	// TODO: proper version check
	apiLayerRequest->layerInterfaceVersion = loaderInfo->maxInterfaceVersion;
	apiLayerRequest->layerApiVersion = loaderInfo->maxApiVersion;
	apiLayerRequest->getInstanceProcAddr = _xrGetInstanceProcAddr;
	apiLayerRequest->createApiLayerInstance = _xrCreateApiLayerInstance;

	return XR_SUCCESS;
}

}
