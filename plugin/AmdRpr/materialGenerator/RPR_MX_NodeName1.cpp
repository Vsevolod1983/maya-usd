#include "RPR_MX_NodeName1.h"
#include <maya/MFnNumericAttribute.h>

MTypeId RPR_MX_NodeName1::m_typeId = 0;

void* RPR_MX_NodeName1::creator()
{
	return new RPR_MX_NodeName1();
}

MStatus RPR_MX_NodeName1::initialize()
{
	MFnNumericAttribute nAttr;

	MStatus status;

	MObject output = nAttr.create("out", "o", MFnNumericData::k3Float, 0.0f, &status);

	CHECK_MSTATUS(nAttr.setKeyable(false));
	CHECK_MSTATUS(nAttr.setStorable(false));
	CHECK_MSTATUS(nAttr.setReadable(true));
	CHECK_MSTATUS(nAttr.setWritable(false));
	addAttribute(output);

	MObject base = nAttr.create("base", "b", MFnNumericData::k3Float, 0.0f, &status);
	nAttr.setMin(0.0);
	nAttr.setMax(1.0);

	CHECK_MSTATUS(nAttr.setKeyable(true));
	CHECK_MSTATUS(nAttr.setStorable(true));
	CHECK_MSTATUS(nAttr.setReadable(false));
	CHECK_MSTATUS(nAttr.setWritable(true));
	addAttribute(base);

	attributeAffects(base, output);
	
	MObject base2 = nAttr.create("base_refl", "brl", MFnNumericData::k2Float, 0.0f, &status);
	CHECK_MSTATUS(nAttr.setKeyable(true));
	CHECK_MSTATUS(nAttr.setStorable(true));
	CHECK_MSTATUS(nAttr.setReadable(false));
	CHECK_MSTATUS(nAttr.setWritable(true));
	addAttribute(base2);

	MObject color = nAttr.createColor("color", "c", &status);
	CHECK_MSTATUS(nAttr.setKeyable(true));
	CHECK_MSTATUS(nAttr.setStorable(true));
	CHECK_MSTATUS(nAttr.setReadable(false));
	CHECK_MSTATUS(nAttr.setWritable(true));
	addAttribute(color);


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
