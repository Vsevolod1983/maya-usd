
#include "RprUsdProductionRenderCmd.h"
#include "RprUsdProductionRender.h"

#include "maya/MGlobal.h"
#include "maya/MDagPath.h"
#include "maya/MFnCamera.h"
#include "maya/MFnRenderLayer.h"
#include "maya/MFnTransform.h"
#include "maya/MRenderView.h"

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/tf/instantiateSingleton.h>
#include <pxr/base/vt/value.h>
#include <pxr/imaging/glf/contextCaps.h>
#include <pxr/imaging/hd/camera.h>
#include <pxr/imaging/hd/renderBuffer.h>
#include <pxr/imaging/hd/rendererPluginRegistry.h>
#include <pxr/imaging/hd/rprim.h>
#include <pxr/imaging/hdx/colorizeSelectionTask.h>
#include <pxr/imaging/hdx/pickTask.h>
#include <pxr/imaging/hdx/renderTask.h>
#include <pxr/imaging/hdx/tokens.h>
#include <pxr/imaging/hgi/hgi.h>
#include <pxr/imaging/hgi/tokens.h>

#include <hdMaya/delegates/delegateRegistry.h>
#include <hdMaya/delegates/sceneDelegate.h>
#include <hdMaya/utils.h>
#include <mayaUsd/render/px_vp20/utils.h>
#include <mayaUsd/utils/hash.h>

#include <pxr/base/tf/debug.h>

#include <memory>

//#include "tokens.h"
//#include "utils.h"


PXR_NAMESPACE_OPEN_SCOPE

MString RprUsdProductionRenderCmd::s_commandName = "rprUsdRender";
std::unique_ptr<RprUsdProductionRender> RprUsdProductionRenderCmd::s_productionRender;

RprUsdProductionRenderCmd::RprUsdProductionRenderCmd()
{

}

// MPxCommand Implementation
// -----------------------------------------------------------------------------
void* RprUsdProductionRenderCmd::creator()
{
	return new RprUsdProductionRenderCmd;
}

// -----------------------------------------------------------------------------
MSyntax RprUsdProductionRenderCmd::newSyntax()
{
	MSyntax syntax;

	CHECK_MSTATUS(syntax.addFlag(kCameraFlag, kCameraFlagLong, MSyntax::kString));
	CHECK_MSTATUS(syntax.addFlag(kRenderLayerFlag, kRenderLayerFlagLong, MSyntax::kString));
	CHECK_MSTATUS(syntax.addFlag(kWidthFlag, kWidthFlagLong, MSyntax::kLong));
	CHECK_MSTATUS(syntax.addFlag(kHeightFlag, kHeightFlagLong, MSyntax::kLong));

	return syntax;
}

MStatus getDimensions(const MArgDatabase& argData, unsigned int& width, unsigned int& height)
{
	// Get dimension arguments.
	if (argData.isFlagSet(kWidthFlag))
		argData.getFlagArgument(kWidthFlag, 0, width);

	if (argData.isFlagSet(kHeightFlag))
		argData.getFlagArgument(kHeightFlag, 0, height);

	// Check that the dimensions are valid.
	if (width <= 0 || height <= 0)
	{
		MGlobal::displayError("Invalid dimensions");
		return MS::kFailure;
	}

	// Success.
	return MS::kSuccess;
}

MStatus getCameraPath(const MArgDatabase& argData, MDagPath& cameraPath)
{
	// Get camera name argument.
	MString cameraName;
	if (argData.isFlagSet(kCameraFlag))
		argData.getFlagArgument(kCameraFlag, 0, cameraName);

	// Get the camera scene DAG path.
	MSelectionList sList;
	sList.add(cameraName);
	MStatus status = sList.getDagPath(0, cameraPath);
	if (status != MS::kSuccess)
	{
		MGlobal::displayError("Invalid camera");
		return MS::kFailure;
	}

	// Extend to include the camera shape.
	cameraPath.extendToShape();
	if (cameraPath.apiType() != MFn::kCamera)
	{
		MGlobal::displayError("Invalid camera");
		return MS::kFailure;
	}

	// Success.
	return MS::kSuccess;
}

// -----------------------------------------------------------------------------
MStatus RprUsdProductionRenderCmd::doIt(const MArgList & args)
{
	// Parse arguments.
	MArgDatabase argData(syntax(), args);

	unsigned int width;
	unsigned int height;

	MStatus status = getDimensions(argData, width, height);

	if (status != MStatus::kSuccess)
	{
		return MStatus::kFailure;
	}

	MDagPath camPath;
	status = getCameraPath(argData, camPath);
	if (status != MS::kSuccess)
		return status;

	//_rendererDesc = 

	MString newLayerName;

	if (argData.isFlagSet(kRenderLayerFlag))
	{
		argData.getFlagArgument(kRenderLayerFlag, 0, newLayerName);
	}

	if (!s_productionRender)
	{
		s_productionRender = std::make_unique<RprUsdProductionRender>();
	}

	return s_productionRender->StartRender(width, height, newLayerName, camPath);
}

// Static Methods
// -----------------------------------------------------------------------------
void RprUsdProductionRenderCmd::cleanUp()
{
	s_productionRender.reset();
}

void RprUsdProductionRenderCmd::RegisterRenderer()
{
	constexpr auto registerCmd = 
	R"mel(global proc registerRprUsdRenderer()
	{
		string $currentRendererName = "hdRPR";

		renderer - rendererUIName $currentRendererName
			- renderProcedure "rprUsdRenderCmd"

			//-logoImageName              "amd.xpm"
			rprUsdRender;

		renderer - edit - addGlobalsNode "RprUsdGlobals" rprUsdRender;
		renderer - edit - addGlobalsNode "defaultRenderGlobals" rprUsdRender;
		renderer - edit - addGlobalsNode "defaultResolution" rprUsdRender;

		renderer - edit - addGlobalsTab "Common" "createMayaSoftwareCommonGlobalsTab" "updateMayaSoftwareCommonGlobalsTab" rprUsdRender;
		renderer - edit - addGlobalsTab "General" "createRprUsdRenderGeneralTab" "updateRprUsdRenderGeneralTab" rprUsdRender;
	}

	global proc string rprUsdRenderCmd(int $resolution0, int $resolution1,
		int $doShadows, int $doGlowPass, string $camera, string $option)
	{
		print("hdRPR command " + $resolution0 + " " + $resolution1 + " " + $doShadows + " " + $doGlowPass + " " + $camera + " " + $option + "\n");
		string $cmd = "rprUsdRender -w " + $resolution0 + " -h " + $resolution1 + " -cam " + $camera + " " + $option;
		eval($cmd);
		string $result = "";
		return $result;
	}

	global proc createRprUsdRenderGeneralTab()
	{
		scrollLayout
			- horizontalScrollBarThickness 0
			scrollLayout;

		columnLayout
			- adjustableColumn true
			generalTabColumn;

		text - label "Put controls here 1!";
		text - label "Put controls here 2!";
		text - label "Put controls here 3!";
		text - label "Put controls here 4!";
	}

	global proc updateRprUsdRenderGeneralTab()
	{

	}

	registerRprUsdRenderer();
)mel";
	MString mstringCmd(registerCmd);
	MStatus status = MGlobal::executeCommand(mstringCmd);
	CHECK_MSTATUS(status);
}


PXR_NAMESPACE_CLOSE_SCOPE