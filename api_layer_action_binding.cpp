#include <openxr/openxr.h>
#include "external/loader_interfaces.h"

#include <string.h>

#include <map>
#include <iostream>

extern "C" {

// for logging purposes
static const char *_layerName = NULL;

// load next function pointers in _xrCreateApiLayerInstance
static PFN_xrGetInstanceProcAddr _nextXrGetInstanceProcAddr = NULL;
static PFN_xrCreateActionSet _nextXrCreateActionSet = NULL;
static PFN_xrCreateAction _nextXrCreateAction = NULL;

// cache create infos
static std::map<XrActionSet, XrActionSetCreateInfo> actionSetInfos;
static std::map<XrAction, XrActionCreateInfo> actionInfos;


static XRAPI_ATTR XrResult XRAPI_CALL
_xrCreateActionSet(XrInstance instance, const XrActionSetCreateInfo *createInfo, XrActionSet *actionSet)
{


	XrResult res = _nextXrCreateActionSet(instance, createInfo, actionSet);
	if (XR_SUCCEEDED(res)) {
		std::cout << _layerName << ": created action set " << createInfo->actionSetName << "|" << *actionSet
		          << std::endl;
		actionSetInfos[*actionSet] = *createInfo;
	} else {
		std::cout << _layerName << ": xrCreateActionSet failed for " << createInfo->actionSetName << std::endl;
	}
	return res;
}

static XRAPI_ATTR XrResult XRAPI_CALL
_xrCreateAction(XrActionSet actionSet, const XrActionCreateInfo *createInfo, XrAction *action)
{

	XrResult res = _nextXrCreateAction(actionSet, createInfo, action);
	if (XR_SUCCEEDED(res)) {
		std::cout << _layerName << ": created action " << createInfo->actionName << "|" << *action
		          << " in action set " << actionSetInfos[actionSet].actionSetName << std::endl;
		actionInfos[*action] = *createInfo;
	} else {
		std::cout << _layerName << ": xrCreateAction failed for" << createInfo->actionName << std::endl;
	}
	return res;
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

	if (func_name == "xrCreateAction") {
		*function = (PFN_xrVoidFunction)_xrCreateAction;
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

	result = _nextXrGetInstanceProcAddr(*instance, "xrCreateAction", (PFN_xrVoidFunction *)&_nextXrCreateAction);
	if (XR_FAILED(result)) {
		std::cout << "Failed to load xrCreateAction" << std::endl;
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
