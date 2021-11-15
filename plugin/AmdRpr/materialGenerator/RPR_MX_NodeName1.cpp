#include "RPR_MX_NodeName1.h"

MTypeId RPR_MX_NodeName1::m_typeId = 0;

void* RPR_MX_NodeName1::creator()
{
	return new RPR_MX_NodeName1();
}

MStatus RPR_MX_NodeName1::initialize()
{

	return MStatus::kSuccess;
}

void RPR_MX_NodeName1::registerNode(INodeRegisterer* pINodeRegisterer, MTypeId typeId)
{
	m_typeId = typeId;
	MString materialName = "NODE_NAME";

	MString shaderClassify("rendernode/RprUsd/MaterialX/shader/surface:shader/surface");
	//MString UserVolumeClassify("rendernode/RprUsd/shader/volume:shader/volume");
	MString utilityClassify("rendernode/RprUsd/MaterialX/utility:utility/general");
	MString textureClassify("rendernode/RprUsd/MaterialX/texture/2d:texture/2d");


	static const MString FireRenderSurfacesDrawDBClassification("drawdb/shader/surface/" + materialName);
	//static const MString FireRenderSurfacesRegistrantId("RPR_MX_<NODE_NAME>RegistrantId");
	static const MString FireRenderSurfacesFullClassification = utilityClassify;// +":" + FireRenderSurfacesDrawDBClassification/* + ":swatch/" + swatchName*/;

	MStatus status = pINodeRegisterer->registerNode(materialName, m_typeId,
		creator,
		initialize,
		MPxNode::kDependNode, &FireRenderSurfacesFullClassification);
}

void RPR_MX_NodeName1::deregisterNode(INodeRegisterer* pINodeRegisterer)
{
	pINodeRegisterer->deregisterNode(m_typeId);
}
