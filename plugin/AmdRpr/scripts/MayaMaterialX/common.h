#pragma once

#include <maya/MFnCompoundAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <string>
#include <vector>

using namespace std;

void LogAttrCreationError(const string& nodeName, const string& longName, const string& error)
{
	cout << "NodeName: " << nodeName << ". Attribute long name: " << longName << ". Error: " << error << endl;
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

enum class ChildAttributeNamingPattern
{
	XYZW = 0,
	RGBA = 1
	//INDEX_0123
};

void CreateFloatChildAttributes(
	unsigned int count, 
	ChildAttributeNamingPattern childNamingPattern, 
	MFnCompoundAttribute& parentAttr,
	const string& nodeName,
	float defaultValue, 
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
		MObject childAttrObj = nAttr.create(longName + postfixes[index], shortName + postfixes[index], MFnNumericData::kFloat, defaultValue, &status);

		if (status != MStatus::kSuccess)
		{
			LogAttrCreationError(nodeName.c_str(), longName.asChar(), "Child Attribute creation failed, index: " + std::to_string(index));
		}

		nAttr.setMin(minValue);
		nAttr.setMax(maxValue);

		MAKE_INPUT(nAttr);

		parentAttr.addChild(childAttrObj);
	}

	//return childAttrs;
}

template <unsigned int count>
MObject CreateFloatArrayAttribute(
	const string& nodeName, 
	const string& longName,  
	const string& shortName, 
	float defaultValue, 
	float minValue, 
	float maxValue, 
	MObject affectingOutput)
{
	MFnCompoundAttribute cAttr;

	MStatus status;
	MObject compoundAttr = cAttr.create(longName.c_str(), shortName.c_str(), &status);

	if (status != MStatus::kSuccess)
	{
		LogAttrCreationError(nodeName, longName, "compound attribute could not be created!");
	}

	CreateFloatChildAttributes(count, ChildAttributeNamingPattern::XYZW, cAttr, nodeName, defaultValue, minValue, maxValue);

	//cAttr.setArray(true);
	MAKE_INPUT(cAttr);

	MPxNode::addAttribute(compoundAttr);

	if (!affectingOutput.isNull())
	{
		MPxNode::attributeAffects(compoundAttr, affectingOutput);
	}

	return compoundAttr;
}
