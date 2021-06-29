#include "util.h"


const char *_layerName = NULL;

// load next function pointers in _xrCreateApiLayerInstance
PFN_xrStringToPath _nextXrStringToPath = NULL;
PFN_xrGetInstanceProcAddr _nextXrGetInstanceProcAddr = NULL;
PFN_xrCreateActionSet _nextXrCreateActionSet = NULL;
PFN_xrCreateAction _nextXrCreateAction = NULL;
PFN_xrCreateActionSpace _nextXrCreateActionSpace = NULL;
PFN_xrSuggestInteractionProfileBindings _nextXrSuggestInteractionProfileBindings = NULL;
PFN_xrAttachSessionActionSets _nextXrAttachSessionActionSets = NULL;

XrInteractionProfileSuggestedBinding *
deepCopyBinding(const XrInteractionProfileSuggestedBinding *suggestedBindings)
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

void
freeBinding(const XrInteractionProfileSuggestedBinding *suggestedBindings)
{
	delete (suggestedBindings->suggestedBindings);
	delete (suggestedBindings);
}
