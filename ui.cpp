#include "ui.h"

bool
createModifiedBindings(XrInstance instance,
                       XrInstanceCreateInfo *instanceInfo,

                       std::map<XrActionSet, XrActionSetCreateInfo> actionSetInfos,
                       std::map<XrAction, XrActionCreateInfo> actionInfos,
                       std::map<XrAction, XrActionSpaceCreateInfo> actionSpaceInfos,
                       std::map<XrPath, std::string> paths,
                       std::map<XrPath, XrInteractionProfileSuggestedBinding *> *bindings,
                       std::map<XrPath, XrInteractionProfileSuggestedBinding *> *modifiedBindings)
{
	for (const auto &profileBindings : *bindings) {
		XrInteractionProfileSuggestedBinding *b = profileBindings.second;

		// TODO modify something
		(*modifiedBindings)[profileBindings.first] = b;
	}

	return true;
}
