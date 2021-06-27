#include <openxr/openxr.h>
#include "external/loader_interfaces.h"

#include <string.h>

#include <map>
#include <vector>
#include <iostream>

extern "C" {

// for logging purposes
static const char *_layerName = NULL;

// load next function pointers in _xrCreateApiLayerInstance
static PFN_xrStringToPath _nextXrStringToPath = NULL;
static PFN_xrGetInstanceProcAddr _nextXrGetInstanceProcAddr = NULL;
static PFN_xrCreateActionSet _nextXrCreateActionSet = NULL;
static PFN_xrCreateAction _nextXrCreateAction = NULL;
static PFN_xrCreateActionSpace _nextXrCreateActionSpace = NULL;
static PFN_xrSuggestInteractionProfileBindings _nextXrSuggestInteractionProfileBindings = NULL;

// cache create infos
static XrInstanceCreateInfo instanceInfo;
static std::map<XrActionSet, XrActionSetCreateInfo> actionSetInfos;
static std::map<XrAction, XrActionCreateInfo> actionInfos;
// only for pose actions
static std::map<XrAction, XrActionSpaceCreateInfo> actionSpaceInfos;

static std::map<XrPath, std::string> paths;

static XRAPI_ATTR XrResult XRAPI_CALL
_xrStringToPath(XrInstance instance, const char *pathString, XrPath *path)
{
	XrResult res = _nextXrStringToPath(instance, pathString, path);

	if (XR_SUCCEEDED(res)) {
		std::cout << _layerName << ": cached string " << pathString << "|" << path << std::endl;
		paths[*path] = std::string(pathString);
	}

	return res;
}

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
_xrCreateActionSpace(XrSession session, const XrActionSpaceCreateInfo *createInfo, XrSpace *space)
{
	XrActionCreateInfo *actionInfo = &actionInfos[createInfo->action];

	XrResult res = _nextXrCreateActionSpace(session, createInfo, space);
	if (XR_SUCCEEDED(res)) {
		std::cout << _layerName << ": created action space " << space << " for action "
		          << actionInfo->actionName << "|" << createInfo->action << std::endl;
		actionSpaceInfos[createInfo->action] = *createInfo;
	} else {
		std::cout << _layerName << ": xrCreateActionSpace failed for" << actionInfo->actionName << std::endl;
	}
	return res;
}

static XRAPI_ATTR XrResult XRAPI_CALL
_xrSuggestInteractionProfileBindings(XrInstance instance, const XrInteractionProfileSuggestedBinding *suggestedBindings)
{
	std::cout << _layerName << ": application suggested bindings for profile "
	          << paths[suggestedBindings->interactionProfile] << std::endl;
	for (uint32_t i = 0; i < suggestedBindings->countSuggestedBindings; i++) {
		XrAction action = suggestedBindings->suggestedBindings[i].action;
		XrPath binding = suggestedBindings->suggestedBindings[i].binding;
		std::cout << "\t" << actionInfos[action].actionName << "|" << action << " -> " << paths[binding]
		          << std::endl;
	}

	XrResult res = _nextXrSuggestInteractionProfileBindings(instance, suggestedBindings);

	if (XR_SUCCEEDED(res)) {
		std::cout << _layerName << ": suggested bindings" << std::endl;
		for (uint32_t i = 0; i < suggestedBindings->countSuggestedBindings; i++) {
		};
	} else {
		std::cout << _layerName << ": xrSuggestInteractionProfileBindings failed" << std::endl;
	}
	return res;
}

static XRAPI_ATTR XrResult XRAPI_CALL
_xrGetInstanceProcAddr(XrInstance instance, const char *name, PFN_xrVoidFunction *function)
{
	// std::cout << _layerName << ": " << name << std::endl;

	std::string func_name = name;

	if (func_name == "xrStringToPath") {
		*function = (PFN_xrVoidFunction)_xrStringToPath;
		return XR_SUCCESS;
	}

	if (func_name == "xrCreateActionSet") {
		*function = (PFN_xrVoidFunction)_xrCreateActionSet;
		return XR_SUCCESS;
	}

	if (func_name == "xrCreateAction") {
		*function = (PFN_xrVoidFunction)_xrCreateAction;
		return XR_SUCCESS;
	}

	if (func_name == "xrCreateActionSpace") {
		*function = (PFN_xrVoidFunction)_xrCreateActionSpace;
		return XR_SUCCESS;
	}

	if (func_name == "xrSuggestInteractionProfileBindings") {
		*function = (PFN_xrVoidFunction)_xrSuggestInteractionProfileBindings;
		return XR_SUCCESS;
	}

	return _nextXrGetInstanceProcAddr(instance, name, function);
}

// xrCreateInstance is a special case that we can't hook. We get this amended call instead.
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
	result = _nextXrGetInstanceProcAddr(*instance, "xrStringToPath", (PFN_xrVoidFunction *)&_nextXrStringToPath);
	if (XR_FAILED(result)) {
		std::cout << "Failed to load xrSuggestInteractionProfileBindings" << std::endl;
		return result;
	}

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

	result = _nextXrGetInstanceProcAddr(*instance, "xrCreateActionSpace",
	                                    (PFN_xrVoidFunction *)&_nextXrCreateActionSpace);
	if (XR_FAILED(result)) {
		std::cout << "Failed to load xrCreateActionSpace" << std::endl;
		return result;
	}

	result = _nextXrGetInstanceProcAddr(*instance, "xrSuggestInteractionProfileBindings",
	                                    (PFN_xrVoidFunction *)&_nextXrSuggestInteractionProfileBindings);
	if (XR_FAILED(result)) {
		std::cout << "Failed to load xrSuggestInteractionProfileBindings" << std::endl;
		return result;
	}

	instanceInfo = *info;
	std::cout << _layerName << ": Created api layer instance for app " << info->applicationInfo.applicationName
	          << std::endl;
	;

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
