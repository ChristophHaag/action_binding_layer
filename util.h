#pragma once

#include <openxr/openxr.h>

// for logging purposes
extern const char *_layerName;

// load next function pointers in _xrCreateApiLayerInstance
extern PFN_xrStringToPath _nextXrStringToPath;
extern PFN_xrGetInstanceProcAddr _nextXrGetInstanceProcAddr;
extern PFN_xrCreateActionSet _nextXrCreateActionSet;
extern PFN_xrCreateAction _nextXrCreateAction;
extern PFN_xrCreateActionSpace _nextXrCreateActionSpace;
extern PFN_xrSuggestInteractionProfileBindings _nextXrSuggestInteractionProfileBindings;
extern PFN_xrAttachSessionActionSets _nextXrAttachSessionActionSets;

XrInteractionProfileSuggestedBinding *
deepCopyBinding(const XrInteractionProfileSuggestedBinding *suggestedBindings);

void
freeBinding(const XrInteractionProfileSuggestedBinding *suggestedBindings);
