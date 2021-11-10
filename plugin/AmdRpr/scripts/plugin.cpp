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

#include <maya/MFnPlugin.h>
//#include <maya/MGlobal.h>
//#include <maya/MPxNode.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

//#include "base/api.h"

//#include <pxr/base/plug/plugin.h>
//#include <pxr/base/plug/registry.h>

#include <limits>

PXR_NAMESPACE_USING_DIRECTIVE

//static const MString _RegistrantId("RprUsd");

__declspec(dllexport)
MStatus initializePlugin(MObject obj)
{
    MStatus status = MStatus::kSuccess;

    MFnPlugin plugin(obj, "AMD", "0.1", "Any");

	plugin.registerFileTranslator(MayaUsd::RPRMaterialXMayaTranslator::translatorName, nullptr, MayaUsd::RPRMaterialXMayaTranslator::creator);

    return status;
}

__declspec(dllexport)
MStatus uninitializePlugin(MObject obj)
{
    MStatus status = MStatus::kSuccess;
    MFnPlugin plugin(obj);

	plugin.deregisterFileTranslator(MayaUsd::RPRMaterialXMayaTranslator::translatorName);

    return status;
}
