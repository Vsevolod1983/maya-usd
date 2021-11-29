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
//typedef 
template <typename T>
using MinMaxData = vector<tuple<bool, T>>;

void LogAttrCreationError(const string& longName, const string& error);
void MakeAsInputOrOutputAndAdd(MFnAttribute& attr, const vector<MObject>& affectingOutput);

void CreateFloatChildAttributes(
	unsigned int count,
	ChildAttributeNamingPattern childNamingPattern,
	MFnCompoundAttribute& parentAttr,
	const vector<float>& defaultValue,
	const MinMaxData<float>& minmaxData);


template <unsigned int count>
MObject CreateFloatArrayAttribute(
	const string& longName,
	const string& shortName,
	const vector<float>& defaultValue,
	const MinMaxData<float>& minmaxData,
	ChildAttributeNamingPattern childNamingPattern,
	const vector<MObject> & affectingOutput)
{
	MFnCompoundAttribute cAttr;

	MStatus status;
	MObject compoundAttr = cAttr.create(longName.c_str(), shortName.c_str(), &status);

	if (status != MStatus::kSuccess)
	{
		LogAttrCreationError(longName, "compound attribute could not be created!");
	}

	CreateFloatChildAttributes(count, childNamingPattern, cAttr, defaultValue, minmaxData);

	MakeAsInputOrOutputAndAdd(cAttr, affectingOutput);

	return compoundAttr;
}

MObject CreateFloatAttribute(
	const string& longName,
	const string& shortName,
	float defaultValue,
	const MinMaxData<float>& minmaxData,
	const vector<MObject> & affectingOutput);

MObject CreateIntAttribute(
	const string& longName,
	const string& shortName,
	int defaultValue,
	const MinMaxData<int>& minmaxData,
	const vector<MObject> & affectingOutput);

MObject CreateBooleanAttribute(
	const string& longName,
	const string& shortName,
	bool defaultValue,
	const vector<MObject> & affectingOutput);

MObject CreateColor3Attribute(
	const string& longName,
	const string& shortName,
	const vector<float>& defaultColor,
	const vector<MObject> & affectingOutput);

MObject CreateStringEnumAttribute(
	const string& longName,
	const string& shortName,
	const vector<string>& values,
	string defaultValue,
	const vector<MObject> & affectingOutput);

MObject CreateStringAttribute(
	const string& longName,
	const string& shortName,
	bool isFilename,
	const vector<MObject> & affectingOutput);

MObject Create4x4MatrixAttribute(
	const string& longName,
	const string& shortName,
	const MMatrix& defaultMatrix,
	const vector<MObject> & affectingOutput);
