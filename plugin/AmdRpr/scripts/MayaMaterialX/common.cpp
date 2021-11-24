#include "common.h"

#include <maya/MFnCompoundAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>

#include <maya/MFnMatrixAttribute.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MString.h>
#include <maya/MPxNode.h>


void LogAttrCreationError(const string& longName, const string& error)
{
	cout << "Attribute long name: " << longName << ". Error: " << error << endl;
}

void MAKE_INPUT(MFnAttribute& attr)
{
	CHECK_MSTATUS(attr.setKeyable(true));
	CHECK_MSTATUS(attr.setStorable(true));
	CHECK_MSTATUS(attr.setReadable(false));
	CHECK_MSTATUS(attr.setWritable(true));
}

void MAKE_INPUT_CONST(MFnAttribute& attr)
{
	CHECK_MSTATUS(attr.setKeyable(false));
	CHECK_MSTATUS(attr.setStorable(true));
	CHECK_MSTATUS(attr.setReadable(false));
	CHECK_MSTATUS(attr.setWritable(true));
	CHECK_MSTATUS(attr.setConnectable(false));
}

void MAKE_OUTPUT(MFnAttribute& attr)
{
	CHECK_MSTATUS(attr.setKeyable(false));
	CHECK_MSTATUS(attr.setStorable(false));
	CHECK_MSTATUS(attr.setReadable(true));
	CHECK_MSTATUS(attr.setWritable(false));
}

void CreateFloatChildAttributes(
	unsigned int count,
	ChildAttributeNamingPattern childNamingPattern,
	MFnCompoundAttribute& parentAttr,
	const vector<float>& defaultValue,
	float minValue,
	float maxValue)
{
	MFnNumericAttribute nAttr;

	static vector<MString> childPostfixesXYZW = { "x", "y", "z", "w" };
	static vector<MString> childPostfixesRGBA = { "r", "g", "b", "a" };
	//static vector<string> childPostfixesIndices = "0", "1", "2", "3";

	MString longName = parentAttr.name();
	MString shortName = parentAttr.shortName();

	vector<MString>& postfixes = childNamingPattern == ChildAttributeNamingPattern::XYZW ? childPostfixesXYZW : childPostfixesRGBA;

	//	vector<MObject> childAttrs;
		//childAttrs.resize(count);

	MStatus status;
	for (unsigned int index = 0; index < count; ++index)
	{
		MObject childAttrObj = nAttr.create(longName + postfixes[index], shortName + postfixes[index], MFnNumericData::kFloat, defaultValue[index], &status);

		if (status != MStatus::kSuccess)
		{
			LogAttrCreationError(longName.asChar(), "Child Attribute creation failed, index: " + std::to_string(index));
		}

		nAttr.setMin(minValue);
		nAttr.setMax(maxValue);

		MAKE_INPUT(nAttr);

		parentAttr.addChild(childAttrObj);
	}

	//return childAttrs;
}

void MakeAsInputAndAdd(MFnAttribute& attr, MObject affectingOutput)
{
	MAKE_INPUT(attr);

	MPxNode::addAttribute(attr.object());

	if (!affectingOutput.isNull())
	{
		MPxNode::attributeAffects(attr.object(), affectingOutput);
	}
}


MObject CreateFloatAttribute(
	const string& longName,
	const string& shortName,
	float defaultValue,
	const MinMaxData& minmaxData,
	MObject affectingOutput)
{
	MFnNumericAttribute nAttr;

	MStatus status;

	MObject obj = nAttr.create(longName.c_str(), shortName.c_str(), MFnNumericData::kFloat, defaultValue, &status);

	if (status != MStatus::kSuccess)
	{
		LogAttrCreationError(longName.c_str(), "Attribute creation failed");
	}

	if (std::get<bool>(minmaxData[0]))
	{
		nAttr.setMin(std::get<float>(minmaxData[0]));
	}

	if (std::get<bool>(minmaxData[1]))
	{
		nAttr.setMax(std::get<float>(minmaxData[1]));
	}

	if (std::get<bool>(minmaxData[2]))
	{
		nAttr.setSoftMin(std::get<float>(minmaxData[2]));
	}

	if (std::get<bool>(minmaxData[3]))
	{
		nAttr.setSoftMax(std::get<float>(minmaxData[3]));
	}

	MakeAsInputAndAdd(nAttr, affectingOutput);

	return obj;
}

MObject CreateIntAttribute(
	const string& longName,
	const string& shortName,
	int defaultValue,
	int minValue,
	int maxValue,
	MObject affectingOutput)
{
	MFnNumericAttribute nAttr;

	MStatus status;

	MObject obj = nAttr.create(longName.c_str(), shortName.c_str(), MFnNumericData::kInt, defaultValue, &status);

	if (status != MStatus::kSuccess)
	{
		LogAttrCreationError(longName.c_str(), "Attribute creation failed");
	}

	nAttr.setMin(minValue);
	nAttr.setMax(maxValue);

	MakeAsInputAndAdd(nAttr, affectingOutput);

	return obj;
}


MObject CreateBooleanAttribute(
	const string& longName,
	const string& shortName,
	bool defaultValue,
	MObject affectingOutput)
{
	MFnNumericAttribute nAttr;

	MStatus status;

	MObject obj = nAttr.create(longName.c_str(), shortName.c_str(), MFnNumericData::kBoolean, defaultValue, &status);

	if (status != MStatus::kSuccess)
	{
		LogAttrCreationError(longName.c_str(), "Attribute creation failed");
	}

	MakeAsInputAndAdd(nAttr, affectingOutput);

	return obj;
}

MObject CreateColor3Attribute(
	const string& longName,
	const string& shortName,
	const vector<float>& defaultColor,
	MObject affectingOutput)
{
	MFnNumericAttribute nAttr;

	MStatus status;

	MObject obj = nAttr.createColor(longName.c_str(), shortName.c_str(), &status);

	nAttr.setDefault(defaultColor[0], defaultColor[1], defaultColor[2]);

	if (status != MStatus::kSuccess)
	{
		LogAttrCreationError(longName.c_str(), "Attribute creation failed");
	}

	MakeAsInputAndAdd(nAttr, affectingOutput);

	return obj;
}

MObject CreateStringAttribute(
	const string& longName,
	const string& shortName,
	bool isFilename,
	MObject affectingOutput)
{
	//MFnNumericAttribute nAttr;
	MFnTypedAttribute tAttr;

	MStatus status;

	MObject obj = tAttr.create(longName.c_str(), shortName.c_str(), MFnData::kString, MObject::kNullObj, &status);

	if (status != MStatus::kSuccess)
	{
		LogAttrCreationError(longName.c_str(), "Attribute creation failed");
	}

	if (isFilename)
	{
		tAttr.setUsedAsFilename(true);
	}

	MakeAsInputAndAdd(tAttr, affectingOutput);

	return obj;
}

MObject Create4x4MatrixAttribute(
	const string& longName,
	const string& shortName,
	const MMatrix& defaultMatrix,
	MObject affectingOutput)
{
	MFnMatrixAttribute mAttr;

	MStatus status;

	MObject obj = mAttr.create(longName.c_str(), shortName.c_str(), MFnMatrixAttribute::kFloat, &status);

	if (status != MStatus::kSuccess)
	{
		LogAttrCreationError(longName.c_str(), "Attribute creation failed");
	}

	mAttr.setDefault(defaultMatrix);

	MakeAsInputAndAdd(mAttr, affectingOutput);

	return obj;
}

MObject CreateStringEnumAttribute(
	const string& longName,
	const string& shortName,
	const vector<string>& values,
	string defaultValue,
	MObject affectingOutput)
{
	MFnEnumAttribute eAttr;

	MStatus status;

	MObject obj = eAttr.create(longName.c_str(), shortName.c_str(), 0, &status);

	if (status != MStatus::kSuccess)
	{
		LogAttrCreationError(longName.c_str(), "Attribute creation failed");
	}

	for (short index = 0; index < values.size(); ++index)
	{
		eAttr.addField(values[index].c_str(), index);
	}

	eAttr.setDefault(defaultValue.c_str());

	MakeAsInputAndAdd(eAttr, affectingOutput);

	return obj;
}
