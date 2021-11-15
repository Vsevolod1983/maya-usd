#pragma once
#include <maya/MPxNode.h>

typedef void * (*CreatorFunction)();
typedef MStatus (*InitializeFunction)();

class INodeRegisterer
{
public:
	virtual MStatus registerNode(
		const MString &typeName,
		const MTypeId &typeId,
		CreatorFunction creatorFunction,
		InitializeFunction initFunction,
		MPxNode::Type type = MPxNode::kDependNode,
		const MString *classification = nullptr) = 0;

	virtual void deregisterNode(MTypeId typeId) = 0;
};
