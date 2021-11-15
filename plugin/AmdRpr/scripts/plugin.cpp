//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
/*#include "usdMaya/exportCommand.h"
#include "usdMaya/exportTranslator.h"
#include "usdMaya/importCommand.h"
#include "usdMaya/importTranslator.h"
#include "usdMaya/listShadingModesCommand.h"
#include "usdMaya/proxyShape.h"
#include "usdMaya/referenceAssembly.h"

#include <mayaUsd/listeners/notice.h>
#include <mayaUsd/nodes/proxyShapePlugin.h>
#include <mayaUsd/render/pxrUsdMayaGL/proxyShapeUI.h>
#include <mayaUsd/utils/diagnosticDelegate.h>
#include <mayaUsd/utils/undoHelperCommand.h>*/

#include <pxr/pxr.h>

#include "ImportExportTranslator.h"

#include "MayaMaterialRegisterer.h"
#include "INodeRegisterer.h"

#include <maya/MFnPlugin.h>
//#include <maya/MPxNode.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

//#include "base/api.h"

//#include <pxr/base/plug/plugin.h>
//#include <pxr/base/plug/registry.h>

#include <limits>

PXR_NAMESPACE_USING_DIRECTIVE

//static const MString _RegistrantId("RprUsd");

class NodeRegisterer : public INodeRegisterer
{
public:
	NodeRegisterer(MObject pluginObj) : m_pluginObj(pluginObj) {}

	MStatus registerNode(
		const MString &typeName,
		const MTypeId &typeId,
		CreatorFunction creatorFunction,
		InitializeFunction initFunction,
		MPxNode::Type type = MPxNode::kDependNode,
		const MString *classification = nullptr) override;

	void deregisterNode(MTypeId typeId) override;

private:
	MObject m_pluginObj;
};


MStatus NodeRegisterer::registerNode(
	const MString &typeName,
	const MTypeId &typeId,
	CreatorFunction creatorFunction,
	InitializeFunction initFunction,
	MPxNode::Type type,
	const MString *classification)
{
	MFnPlugin plugin(m_pluginObj);

	return plugin.registerNode(typeName, typeId, creatorFunction, initFunction, type, classification);
}

void NodeRegisterer::deregisterNode(MTypeId typeId)
{
	MFnPlugin plugin(m_pluginObj);
	plugin.deregisterNode(typeId);
}

__declspec(dllexport)
MStatus initializePlugin(MObject pluginObj)
{
    MStatus status = MStatus::kSuccess;

    MFnPlugin plugin(pluginObj, "AMD", "0.1", "Any");

	plugin.registerFileTranslator(MayaUsd::RPRMaterialXMayaTranslator::translatorName, nullptr, MayaUsd::RPRMaterialXMayaTranslator::creator);

	NodeRegisterer nodeRegisterer(pluginObj);
	MayaMaterialRegisterer::registerNodes(&nodeRegisterer);

    return status;
}

__declspec(dllexport)
MStatus uninitializePlugin(MObject pluginObj)
{
    MStatus status = MStatus::kSuccess;
    MFnPlugin plugin(pluginObj);

	plugin.deregisterFileTranslator(MayaUsd::RPRMaterialXMayaTranslator::translatorName);

	NodeRegisterer nodeRegisterer(pluginObj);
	MayaMaterialRegisterer::deregisterNodes(&nodeRegisterer);

    return status;
}
