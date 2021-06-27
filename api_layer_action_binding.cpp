#include <openxr/openxr.h>
#include "external/loader_interfaces.h"

#include <string.h>

#include <iostream>

extern "C" {

// for logging purposes
static const char *_layerName = NULL;
;

// load next function pointers in _xrCreateApiLayerInstance
static PFN_xrGetInstanceProcAddr _nextXrGetInstanceProcAddr = NULL;
static PFN_xrCreateActionSet _nextXrCreateActionSet = NULL;

static XRAPI_ATTR XrResult XRAPI_CALL
_xrCreateActionSet(XrInstance instance, const XrActionSetCreateInfo *createInfo, XrActionSet *actionSet)
{

	std::cout << _layerName << ": xrCreateActionSet " << createInfo->actionSetName << std::endl;

	return _nextXrCreateActionSet(instance, createInfo, actionSet);
}

static XRAPI_ATTR XrResult XRAPI_CALL
_xrGetInstanceProcAddr(XrInstance instance, const char *name, PFN_xrVoidFunction *function)
{
	// std::cout << _layerName << ": " << name << std::endl;

	std::string func_name = name;

	if (func_name == "xrCreateActionSet") {
		*function = (PFN_xrVoidFunction)_xrCreateActionSet;
		return XR_SUCCESS;
	}

	return _nextXrGetInstanceProcAddr(instance, name, function);
}

static XrResult XRAPI_PTR
_xrCreateApiLayerInstance(const XrInstanceCreateInfo *info,
                          const XrApiLayerCreateInfo *apiLayerInfo,
                          XrInstance *instance)
{
	_nextXrGetInstanceProcAddr = apiLayerInfo->nextInfo->nextGetInstanceProcAddr;
	XrResult result;

	// first let the instance be created
	result = apiLayerInfo->nextInfo->nextCreateApiLayerInstance(info, apiLayerInfo, instance);
	if (XR_FAILED(result)) {
		std::cout << "Failed to load xrCreateActionSet" << std::endl;
		return result;
	}

	// then use the created instance to load next function pointers
	result =
	    _nextXrGetInstanceProcAddr(*instance, "xrCreateActionSet", (PFN_xrVoidFunction *)&_nextXrCreateActionSet);
	if (XR_FAILED(result)) {
		std::cout << "Failed to load xrCreateActionSet" << std::endl;
		return result;
	}

	std::cout << _layerName << ": Created api layer instance" << std::endl;

	return result;
}

XrResult
xrNegotiateLoaderApiLayerInterface(const XrNegotiateLoaderInfo *loaderInfo,
                                   const char *layerName,
                                   XrNegotiateApiLayerRequest *apiLayerRequest)
{
	_layerName = strdup(layerName);

	std::cout << layerName << ": Using API layer: " << layerName << std::endl;

	std::cout << layerName << ": loader API version min: " << XR_VERSION_MAJOR(loaderInfo->minApiVersion) << "."
	          << XR_VERSION_MINOR(loaderInfo->minApiVersion) << "." << XR_VERSION_PATCH(loaderInfo->minApiVersion)
	          << "."
	          << " max: " << XR_VERSION_MAJOR(loaderInfo->maxApiVersion) << "."
	          << XR_VERSION_MINOR(loaderInfo->maxApiVersion) << "." << XR_VERSION_PATCH(loaderInfo->maxApiVersion)
	          << "." << std::endl;

	std::cout << layerName
	          << ": loader interface version min: " << XR_VERSION_MAJOR(loaderInfo->minInterfaceVersion) << "."
	          << XR_VERSION_MINOR(loaderInfo->minInterfaceVersion) << "."
	          << XR_VERSION_PATCH(loaderInfo->minInterfaceVersion) << "."
	          << " max: " << XR_VERSION_MAJOR(loaderInfo->maxInterfaceVersion) << "."
	          << XR_VERSION_MINOR(loaderInfo->maxInterfaceVersion) << "."
	          << XR_VERSION_PATCH(loaderInfo->maxInterfaceVersion) << "." << std::endl;



	// TODO: proper version check
	apiLayerRequest->layerInterfaceVersion = loaderInfo->maxInterfaceVersion;
	apiLayerRequest->layerApiVersion = loaderInfo->maxApiVersion;
	apiLayerRequest->getInstanceProcAddr = _xrGetInstanceProcAddr;
	apiLayerRequest->createApiLayerInstance = _xrCreateApiLayerInstance;

	return XR_SUCCESS;
}
}
