#pragma once

#include <maya/MObject.h>
#include <maya/MFnCompoundAttribute.h>

#include <string>
#include <vector>
#include <tuple>

using namespace std;

enum class ChildAttributeNamingPattern
{
	XYZW = 0,
	RGBA = 1
	//INDEX_0123
};


// 0 - MIN
// 1 - MAX
// 2 - SOFTMIN
// 3 - SOFTMAX
// bool - wheather particular limit should be set or not
// float - value itself to be set if first parameter is true
typedef vector<tuple<bool, float>> MinMaxData;

void LogAttrCreationError(const string& longName, const string& error);
void MakeAsInputAndAdd(MFnAttribute& attr, MObject affectingOutput);
void CreateFloatChildAttributes(
	unsigned int count,
	ChildAttributeNamingPattern childNamingPattern,
	MFnCompoundAttribute& parentAttr,
	const vector<float>& defaultValue,
	float minValue,
	float maxValue);


template <unsigned int count>
MObject CreateFloatArrayAttribute(
	const string& longName,
	const string& shortName,
	const vector<float>& defaultValue,
	float minValue,
	float maxValue,
	MObject affectingOutput)
{
	MFnCompoundAttribute cAttr;

	MStatus status;
	MObject compoundAttr = cAttr.create(longName.c_str(), shortName.c_str(), &status);

	if (status != MStatus::kSuccess)
	{
		LogAttrCreationError(longName, "compound attribute could not be created!");
	}

	CreateFloatChildAttributes(count, ChildAttributeNamingPattern::XYZW, cAttr, defaultValue, minValue, maxValue);

	MakeAsInputAndAdd(cAttr, affectingOutput);

	return compoundAttr;
}


MObject CreateFloatAttribute(
	const string& longName,
	const string& shortName,
	float defaultValue,
	const MinMaxData& minmaxData,
	MObject affectingOutput);

MObject CreateIntAttribute(
	const string& longName,
	const string& shortName,
	int defaultValue,
	int minValue,
	int maxValue,
	MObject affectingOutput);

MObject CreateBooleanAttribute(
	const string& longName,
	const string& shortName,
	bool defaultValue,
	MObject affectingOutput);

MObject CreateColor3Attribute(
	const string& longName,
	const string& shortName,
	const vector<float>& defaultColor,
	MObject affectingOutput);

MObject CreateStringAttribute(
	const string& longName,
	const string& shortName,
	bool isFilename,
	MObject affectingOutput);

MObject Create4x4MatrixAttribute(
	const string& longName,
	const string& shortName,
	const MMatrix& defaultMatrix,
	MObject affectingOutput);

MObject CreateStringEnumAttribute(
	const string& longName,
	const string& shortName,
	const vector<string>& values,
	string defaultValue,
	MObject affectingOutput);
