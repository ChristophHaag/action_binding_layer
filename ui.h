#pragma once

#include <openxr/openxr.h>

#include <string>
#include <map>

bool
createModifiedBindings(XrInstance instance,
                       XrInstanceCreateInfo *instanceInfo,
                       std::map<XrActionSet, XrActionSetCreateInfo> actionSetInfos,
                       std::map<XrAction, XrActionCreateInfo> actionInfos,
                       std::map<XrAction, XrActionSpaceCreateInfo> actionSpaceInfos,
                       std::map<XrPath, std::string> paths,
                       std::map<XrPath, XrInteractionProfileSuggestedBinding *> *bindings,
                       std::map<XrPath, XrInteractionProfileSuggestedBinding *> *modifiedBindings);
