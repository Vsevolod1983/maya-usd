#include "RPR_MX_NodeName1.h"
#include <maya/MFnNumericAttribute.h>
#include "../scripts/MayaMaterialX/common.h"

MTypeId RPR_MX_NodeName1::m_typeId = 0;

void* RPR_MX_NodeName1::creator()
{
	return new RPR_MX_NodeName1();
}

MStatus RPR_MX_NodeName1::initialize()
{
	MFnNumericAttribute nAttr;

	MStatus status;

	MObject output = nAttr.create("out", "o", MFnNumericData::kFloat, 0.0f, &status);

	CHECK_MSTATUS(nAttr.setKeyable(false));
	CHECK_MSTATUS(nAttr.setStorable(false));
	CHECK_MSTATUS(nAttr.setReadable(true));
	CHECK_MSTATUS(nAttr.setWritable(false));
	addAttribute(output);


//	CreateFloatAttribute("input1", "in1", 0.5f, 0.0f, 1.0f, 0.1f, 0.9f, output);
//	CreateLongAttribute("input2", "in2", 37, 0, 100, output);
//	CreateBooleanAttribute("input3", "in3", true, output);

//	CreateColor3Attribute("color", "c", {0.3f, 0.3f, 0.3f}, output);


	MObject obj = nAttr.create("asdd", "ff", MFnNumericData::kInt, 4);

	nAttr.setMin(2);
	nAttr.setMax(22);

	nAttr.setSoftMin(4);
	nAttr.setSoftMax(20);

	CHECK_MSTATUS(nAttr.setKeyable(true));
	CHECK_MSTATUS(nAttr.setStorable(true));
	CHECK_MSTATUS(nAttr.setReadable(false));
	CHECK_MSTATUS(nAttr.setWritable(true));
	addAttribute(obj);

	//CreateFloatArrayAttribute<3>("color3", "cl3", { 1.0f, 1.0f, 1.0f }, 0.0f, 1.0f, output);
	//CreateFloatArrayAttribute<4>("color4", "cl4", { 1.0f, 1.0f, 1.0f, 1.0f }, 0.0f, 1.0f, output);

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


	//static const MString FireRenderSurfacesDrawDBClassification("drawdb/shader/surface/" + materialName);
	//static const MString FireRenderSurfacesRegistrantId("RPR_MX_<NODE_NAME>RegistrantId");
	//static const MString FireRenderSurfacesFullClassification = utilityClassify;// +":" + FireRenderSurfacesDrawDBClassification/* + ":swatch/" + swatchName*/;

	MStatus status = pINodeRegisterer->registerNode(materialName, m_typeId,
		creator,
		initialize,
		MPxNode::kDependNode, &utilityClassify);
}

void RPR_MX_NodeName1::deregisterNode(INodeRegisterer* pINodeRegisterer)
{
	pINodeRegisterer->deregisterNode(m_typeId);
}
