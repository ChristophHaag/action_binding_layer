#include <ui.h>

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
static PFN_xrAttachSessionActionSets _nextXrAttachSessionActionSets = NULL;

// cache create infos
static XrInstanceCreateInfo instanceInfo;
static XrInstance xrInstance;
static std::map<XrActionSet, XrActionSetCreateInfo> actionSetInfos;
static std::map<XrAction, XrActionCreateInfo> actionInfos;
// only for pose actions
static std::map<XrAction, XrActionSpaceCreateInfo> actionSpaceInfos;
// interactionProfile -> bindings. suggesting multiple times for the same profile replaces earlier suggestions
static std::map<XrPath, XrInteractionProfileSuggestedBinding *> bindings;
static std::map<XrPath, XrInteractionProfileSuggestedBinding *> modifiedBindings;

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

static XrInteractionProfileSuggestedBinding *
_deepCopyBinding(const XrInteractionProfileSuggestedBinding *suggestedBindings)
{
	XrInteractionProfileSuggestedBinding *copy = new XrInteractionProfileSuggestedBinding;
	copy->type = suggestedBindings->type;
	copy->next = suggestedBindings->next; // TODO: handle next
	copy->countSuggestedBindings = suggestedBindings->countSuggestedBindings;
	copy->interactionProfile = suggestedBindings->interactionProfile;

	// suggestedBindings->suggestedBindings is const, so have to fill it before assigning
	XrActionSuggestedBinding *b = new XrActionSuggestedBinding[suggestedBindings->countSuggestedBindings];
	for (uint32_t i = 0; i < suggestedBindings->countSuggestedBindings; i++) {
		b[i].action = suggestedBindings->suggestedBindings[i].action;
		b[i].binding = suggestedBindings->suggestedBindings[i].binding;
	}
	copy->suggestedBindings = b;
	return copy;
}

static void
_freeBinding(const XrInteractionProfileSuggestedBinding *suggestedBindings)
{
	delete (suggestedBindings->suggestedBindings);
	delete (suggestedBindings);
}

static XRAPI_ATTR XrResult XRAPI_CALL
_xrSuggestInteractionProfileBindings(XrInstance instance, const XrInteractionProfileSuggestedBinding *suggestedBindings)
{
	std::cout << _layerName << ": application suggested bindings for profile "
	          << paths[suggestedBindings->interactionProfile] << std::endl;
	for (uint32_t i = 0; i < suggestedBindings->countSuggestedBindings; i++) {
		XrAction action = suggestedBindings->suggestedBindings[i].action;
		XrPath binding = suggestedBindings->suggestedBindings[i].binding;
		std::cout << _layerName << ": \t" << actionInfos[action].actionName << "|" << action << " -> "
		          << paths[binding] << std::endl;
	}

	/*
	 * Do not suggest bindings to the runtime just yet, just collect them all.
	 * When xrAttachSessionActionSets we know all profiles have been suggested.
	 * Only then then do we modify them and suggest our modified bindings.
	 * Deep copy, the application might free the memory after it suggested
	 */
	bindings[suggestedBindings->interactionProfile] = _deepCopyBinding(suggestedBindings);
	(void)instance;

#if 0
	XrResult res = _nextXrSuggestInteractionProfileBindings(instance, suggestedBindings);

	if (XR_SUCCEEDED(res)) {
		std::cout << _layerName << ": suggested bindings" << std::endl;
	} else {
		std::cout << _layerName << ": xrSuggestInteractionProfileBindings failed" << std::endl;
	}

	return res;
#endif

	return XR_SUCCESS;
}

static XRAPI_ATTR XrResult XRAPI_CALL
_xrAttachSessionActionSets(XrSession session, const XrSessionActionSetsAttachInfo *attachInfo)
{
	std::cout << _layerName << ": application wants to attach action sets" << std::endl;

	bool succ = createModifiedBindings(xrInstance, &instanceInfo, actionSetInfos, actionInfos, actionSpaceInfos,
	                                   paths, &bindings, &modifiedBindings);

	if (!succ) {
		std::cout << _layerName << ": We failed to modify bindings, bailing out!" << std::endl;
		return XR_ERROR_RUNTIME_FAILURE;
	}

	std::cout << _layerName << ": modified bindings:" << std::endl;
	for (const auto &profileBindings : modifiedBindings) {
		XrInteractionProfileSuggestedBinding *b = profileBindings.second;
		std::cout << _layerName << ": " << paths[b->interactionProfile] << std::endl;
		for (uint32_t i = 0; i < b->countSuggestedBindings; i++) {
			XrAction action = b->suggestedBindings[i].action;
			XrPath binding = b->suggestedBindings[i].binding;
			std::cout << _layerName << ": \t" << actionInfos[action].actionName << "|" << action << " -> "
			          << paths[binding] << std::endl;
		}

		XrResult res = _nextXrSuggestInteractionProfileBindings(xrInstance, b);

		if (XR_SUCCEEDED(res)) {
			std::cout << _layerName << ": suggested bindings" << std::endl;
		} else {
			std::cout << _layerName << ": xrSuggestInteractionProfileBindings failed" << std::endl;
		}
	}

	XrResult res = _nextXrAttachSessionActionSets(session, attachInfo);
	if (XR_SUCCEEDED(res)) {
		std::cout << _layerName << ": attaching action sets ";
		for (uint32_t i = 0; i < attachInfo->countActionSets; i++) {
			std::cout << actionSetInfos[attachInfo->actionSets[i]].actionSetName << "|"
			          << attachInfo->actionSets[i] << ", ";
		}
		std::cout << std::endl;
	} else {
		std::cout << _layerName << ": xrAttachSessionActionSets failed" << std::endl;
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

	if (func_name == "xrAttachSessionActionSets") {
		*function = (PFN_xrVoidFunction)_xrAttachSessionActionSets;
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

	result = _nextXrGetInstanceProcAddr(*instance, "xrAttachSessionActionSets",
	                                    (PFN_xrVoidFunction *)&_nextXrAttachSessionActionSets);
	if (XR_FAILED(result)) {
		std::cout << "Failed to load xrAttachSessionActionSets" << std::endl;
		return result;
	}

	instanceInfo = *info;
	xrInstance = *instance;
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
