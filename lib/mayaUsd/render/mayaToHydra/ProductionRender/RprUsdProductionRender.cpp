#include "RprUsdProductionRender.h" 
#include "ProductionSettings.h"
#include "common.h"

#include "maya/MGlobal.h"
#include "maya/MDagPath.h"
#include "maya/MFnCamera.h"
#include "maya/MFnRenderLayer.h"
#include "maya/MFnTransform.h"
#include "maya/MRenderView.h"
#include "maya/MTimerMessage.h"
#include <maya/MCommonRenderSettingsData.h>

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
//#include "tokens.h"
//#include "utils.h"


PXR_NAMESPACE_OPEN_SCOPE

TfToken RprUsdProductionRender::_rendererName;

RprUsdProductionRender::RprUsdProductionRender() :
	 _renderIsStarted(false)
	, _initialized(false)
	, _hasDefaultLighting(false)
	, _isConverged(false)
	, _hgi(Hgi::CreatePlatformDefaultHgi())
	, _hgiDriver{ HgiTokens->renderDriver, VtValue(_hgi.get()) }
{

}

RprUsdProductionRender::~RprUsdProductionRender()
{
	ClearHydraResources();
}

// -----------------------------------------------------------------------------
MStatus switchRenderLayer(MString& oldLayerName, MString& newLayerName)
{
	// Find the current render layer.
	MObject existingRenderLayer = MFnRenderLayer::currentLayer();
	MFnDependencyNode layerNodeFn(existingRenderLayer);
	oldLayerName = layerNodeFn.name();

	if (newLayerName.length() == 0)
	{
		newLayerName = oldLayerName;
	}

	// Switch the render layer if required.
	if (newLayerName != oldLayerName)
	{
		return MGlobal::executeCommand("editRenderLayerGlobals -currentRenderLayer " + newLayerName, false, true);
	}

	// Success.
	return MS::kSuccess;
}

// -----------------------------------------------------------------------------
MStatus restoreRenderLayer(const MString& oldLayerName, const MString& newLayerName)
{
	// Switch back to the layer that was active
	// before rendering started if necessary.
	if (oldLayerName != newLayerName)
		return MGlobal::executeCommand("editRenderLayerGlobals -currentRenderLayer " + oldLayerName, false, true);

	// Success.
	return MS::kSuccess;
}

void RprUsdProductionRender::RPRMainThreadTimerEventCallback(float, float, void* pClientData)
{
	RprUsdProductionRender* pProductionRender = static_cast<RprUsdProductionRender*> (pClientData);
	pProductionRender->ProcessTimerMessage();
}

void RprUsdProductionRender::ProcessTimerMessage()
{
	RefreshRenderView();

	HdRenderBuffer* bufferPtr = _taskController->GetRenderOutput(HdAovTokens->color);

	HdRenderDelegate* renderDelegate = _renderIndex->GetRenderDelegate();

	VtDictionary dict = renderDelegate->GetRenderStats();

	double percentDone = dict.find("percentDone")->second.Get<double>();
	_renderProgressBars->update((int)percentDone);

	if (bufferPtr->IsConverged())
	{
		StopRender();
	}
	else if (_renderProgressBars->isCancelled())
	{
		StopRender();
	}
}

MStatus RprUsdProductionRender::StartRender(unsigned int width, unsigned int height, MString newLayerName, MDagPath cameraPath)
{
	StopRender();

	_ID = SdfPath("/HdMayaViewportRenderer")
		.AppendChild(
			TfToken(TfStringPrintf("_HdMaya_%s_%p", _rendererName.GetText(), this)));

	_viewport = GfVec4d(0, 0, width, height);

	_newLayerName = newLayerName;
	_camPath = cameraPath;
	_renderIsStarted = true;
	switchRenderLayer(_oldLayerName, _newLayerName);

	if (!InitHydraResources())
	{
		return MStatus::kFailure;
	}

	MRenderView::startRender(width, height, false, true);
	_renderProgressBars = std::make_unique<RenderProgressBars>(false);

	ApplySettings();
	Render();

	const float REFRESH_RATE = 1.0f;
	MStatus status;
	_callbackTimerId = MTimerMessage::addTimerCallback(REFRESH_RATE, RPRMainThreadTimerEventCallback, this, &status);

	return MStatus::kSuccess;
}

void RprUsdProductionRender::ApplySettings()
{
	ProductionSettings::ApplySettings(_GetRenderDelegate());
}

void RprUsdProductionRender::StopRender()
{
	if (!_renderIsStarted)
	{
		return;
	}

	// call abort render somehow

	RefreshRenderView();
	SaveToFile();
	_renderProgressBars.reset();

	MRenderView::endRender();

	HdRenderDelegate* renderDelegate = _renderIndex->GetRenderDelegate();
	renderDelegate->Stop();


	// unregsiter timer callback
	MTimerMessage::removeCallback(_callbackTimerId);
	_callbackTimerId = 0;

	restoreRenderLayer(_oldLayerName, _newLayerName);

	ClearHydraResources();
	_renderIsStarted = false;
}

void RprUsdProductionRender::SaveToFile()
{
	MCommonRenderSettingsData settings;
	MRenderUtil::getCommonRenderSettings(settings);

	MString sceneName = settings.name;

	// Populate the scene name with the current namespace - this
	// should be equivalent to the scene name and is how Maya generates
	// it for post render operations such as layered PSD creation.
	if (sceneName.length() <= 0)
	{
		MStringArray results;
		MGlobal::executeCommand("file -q -ns", results);

		if (results.length() > 0)
			sceneName = results[0];
	}

	MString cameraName = MFnDagNode(_camPath.transform()).name();
	unsigned int frame = static_cast<unsigned int>(MAnimControl::currentTime().value());

	MString fullPath = settings.getImageName(MCommonRenderSettingsData::kFullPathImage, frame,
		sceneName, cameraName, "", MFnRenderLayer::currentLayer());

	// remove existing file to avoid file replace confirmation prompt
	MGlobal::executeCommand("sysFile -delete \"" + fullPath + "\"");

	int dotIndex = fullPath.rindex('.');

	if (dotIndex > 0)
	{
		fullPath = fullPath.substring(0, dotIndex - 1);
	}

	std::string cmd = TfStringPrintf("$editor = `renderWindowEditor -q -editorName`; renderWindowEditor -e -writeImage \"%s\" $editor", fullPath.asChar());

	MStatus status = MGlobal::executeCommand(cmd.c_str());

	if (status != MStatus::kSuccess)
	{
		MGlobal::displayError("[hdRPR] Render image could not be saved!");
	}
}

void RprUsdProductionRender::RefreshRenderView()
{
	HdRenderBuffer* bufferPtr = _taskController->GetRenderOutput(HdAovTokens->color);
	RV_PIXEL* rawBuffer = static_cast<RV_PIXEL*>(bufferPtr->Map());

	// _TODO Remove constants
	unsigned int width = _viewport.GetArray()[2];
	unsigned int height = _viewport.GetArray()[3];

	// Update the render view pixels.
	MRenderView::updatePixels(
		0, width - 1,
		0, height - 1,
		rawBuffer, true);

	// Refresh the render view.
	MRenderView::refresh(0, width - 1, 0, height - 1);

	bufferPtr->Unmap();
}

bool RprUsdProductionRender::InitHydraResources()
{
#if PXR_VERSION < 2102
	GlfGlewInit();
#endif
	GlfContextCaps::InitInstance();
	_rendererPlugin
		= HdRendererPluginRegistry::GetInstance().GetRendererPlugin(_rendererName);
	if (!_rendererPlugin)
		return false;

	auto* renderDelegate = _rendererPlugin->CreateRenderDelegate();
	if (!renderDelegate)
		return false;

	_renderIndex = HdRenderIndex::New(renderDelegate, { &_hgiDriver });
	if (!_renderIndex)
		return false;

	_taskController = new HdxTaskController(
		_renderIndex,
		_ID.AppendChild(TfToken(TfStringPrintf(
			"_UsdImaging_%s_%p",
			TfMakeValidIdentifier(_rendererName.GetText()).c_str(),
			this))));
	_taskController->SetEnableShadows(true);

	HdMayaDelegate::InitData delegateInitData(
		TfToken(),
		_engine,
		_renderIndex,
		_rendererPlugin,
		_taskController,
		SdfPath(),
		/*_isUsingHdSt*/false);

	auto delegateNames = HdMayaDelegateRegistry::GetDelegateNames();
	auto creators = HdMayaDelegateRegistry::GetDelegateCreators();
	TF_VERIFY(delegateNames.size() == creators.size());
	for (size_t i = 0, n = creators.size(); i < n; ++i) {
		const auto& creator = creators[i];
		if (creator == nullptr) {
			continue;
		}
		delegateInitData.name = delegateNames[i];
		delegateInitData.delegateID = _ID.AppendChild(
			TfToken(TfStringPrintf("_Delegate_%s_%lu_%p", delegateNames[i].GetText(), i, this)));
		auto newDelegate = creator(delegateInitData);
		if (newDelegate) {
			// Call SetLightsEnabled before the delegate is populated
			newDelegate->SetLightsEnabled(!_hasDefaultLighting);
			_delegates.emplace_back(std::move(newDelegate));
		}
	}
	if (_hasDefaultLighting) {
		delegateInitData.delegateID
			= _ID.AppendChild(TfToken(TfStringPrintf("_DefaultLightDelegate_%p", this)));
		_defaultLightDelegate.reset(new MtohDefaultLightDelegate(delegateInitData));
	}

	for (auto& it : _delegates) {
		it->Populate();
	}
	if (_defaultLightDelegate) {
		_defaultLightDelegate->Populate();
	}

	_initialized = true;
	return true;
}

HdRenderDelegate* RprUsdProductionRender::_GetRenderDelegate()
{
	return _renderIndex ? _renderIndex->GetRenderDelegate() : nullptr;
}

void RprUsdProductionRender::ClearHydraResources()
{
	_delegates.clear();
	_defaultLightDelegate.reset();

	if (_taskController != nullptr) {
		delete _taskController;
		_taskController = nullptr;
	}

	HdRenderDelegate* renderDelegate = nullptr;
	if (_renderIndex != nullptr) {
		renderDelegate = _renderIndex->GetRenderDelegate();
		delete _renderIndex;
		_renderIndex = nullptr;
	}

	if (_rendererPlugin != nullptr) {
		if (renderDelegate != nullptr) {
			_rendererPlugin->DeleteRenderDelegate(renderDelegate);
		}
		HdRendererPluginRegistry::GetInstance().ReleasePlugin(_rendererPlugin);
		_rendererPlugin = nullptr;
	}
}

MStatus RprUsdProductionRender::Render()
{
	auto renderFrame = [&](bool markTime = false) {
		HdTaskSharedPtrVector tasks = _taskController->GetRenderingTasks();

		SdfPath path;
		for (auto it = tasks.begin(); it != tasks.end(); ++it)
		{
			std::shared_ptr<HdxColorizeSelectionTask> selectionTask
				= std::dynamic_pointer_cast<HdxColorizeSelectionTask>(*it);

			if (selectionTask != nullptr)
			{
				tasks.erase(it);
				break;
			}
		}

		_engine.Execute(_renderIndex, &tasks);
	};

	GLUniformBufferBindingsSaver bindingsSaver;

	HdxRenderTaskParams params;
	params.enableLighting = true;
	params.enableSceneMaterials = true;

	_taskController->SetRenderViewport(_viewport);

	MStatus  status;

	MFnCamera fnCamera(_camPath.node());

	// From Maya Documentation: Returns the orthographic or perspective projection matrix for the camera. 
	// The projection matrix that maya's software renderer uses is almost identical to the OpenGL projection matrix. 
	// The difference is that maya uses a left hand coordinate system and so the entries [2][2] and [3][2] are negated.
	MFloatMatrix projMatrix = fnCamera.projectionMatrix();
	projMatrix[2][2] = -projMatrix[2][2];
	projMatrix[3][2] = -projMatrix[3][2];

	MMatrix viewMatrix = MFnTransform(_camPath.transform()).transformationMatrix();

	_taskController->SetEnablePresentation(false);

	_taskController->SetFreeCameraMatrices(
		GetGfMatrixFromMaya(viewMatrix.inverse()),
		GetGfMatrixFromMaya(projMatrix));

	_taskController->SetRenderParams(params);
	if (!params.camera.IsEmpty())
		_taskController->SetCameraPath(params.camera);

	// Default color in usdview.
	_taskController->SetEnableSelection(false);

	_taskController->SetCollection(_renderCollection);
	
	renderFrame(true);
	

	for (auto& it : _delegates) {
		it->PostFrame();
	}

	return MStatus::kSuccess;
}

void RprUsdProductionRender::Initialize()
{
	_rendererName = TfToken(GetRendererName());

	std::string controlCreationCmds;

	ProductionSettings::RegisterCallbacks();
	controlCreationCmds = ProductionSettings::CreateAttributes();

	RprUsdProductionRender::RegisterRenderer(controlCreationCmds);
}

void RprUsdProductionRender::Uninitialize()
{
	ProductionSettings::UnregisterCallbacks();
}

void RprUsdProductionRender::RegisterRenderer(const std::string& controlCreationCmds)
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
		string $parentForm = `setParent -query`;

		scrollLayout -w 375 -horizontalScrollBarThickness 0 rprmayausd_scrollLayout;
		columnLayout -w 375 -adjustableColumn true rprmayausd_tabcolumn;

		{CONTROLS_CREATION_CMDS}

		formLayout
			-edit
			-af rprmayausd_scrollLayout "top" 0
			-af rprmayausd_scrollLayout "bottom" 0
			-af rprmayausd_scrollLayout "left" 0
			-af rprmayausd_scrollLayout "right" 0
			$parentForm;
	}

	global proc updateRprUsdRenderGeneralTab()
	{

	}

	registerRprUsdRenderer();
)mel";

	std::string registerRenderCmd = TfStringReplace(registerCmd, "{CONTROLS_CREATION_CMDS}", controlCreationCmds);

	MString mstringCmd(registerRenderCmd.c_str());
	MStatus status = MGlobal::executeCommand(mstringCmd);
	CHECK_MSTATUS(status);
}

PXR_NAMESPACE_CLOSE_SCOPE