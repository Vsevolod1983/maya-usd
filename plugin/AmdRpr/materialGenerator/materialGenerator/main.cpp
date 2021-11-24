#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <assert.h>
#include <algorithm>

#include "MaterialXCore/Document.h"
#include "MaterialXFormat/File.h"
#include "MaterialXFormat/Util.h"

#include <sstream>

#include <memory>
#include <string>
#include <stdexcept>

const std::string USD_BUILD = "C:\\Projects\\USD\\USDBuild/";

using namespace std;
namespace mx = MaterialX;


template<typename ... Args>
std::string string_format(const std::string& format, Args ... args)
{
	int size_s = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'

	size_t size = static_cast<size_t>(size_s);
	auto  buf = std::make_unique<char[]>(size);
	std::snprintf(buf.get(), size, format.c_str(), args ...);

	return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}

void ReplaceAll(string& inout, const string& findWhat, const string& replaceWith)
{
	if (findWhat.empty())
	{
		return;
	}

	size_t pos = 0;

	while ((pos = inout.find(findWhat, pos)) != std::string::npos)
	{
		inout.replace(pos, findWhat.length(), replaceWith);
		pos += replaceWith.length(); // ...
	}
}


string getShortName(vector<string>& usedShortNames, mx::InputPtr inputPtr)
{
	string name = inputPtr->getName();

	int attempt = 0;

	string nameToTry;
	do
	{ 
		// first try just first character
		if (attempt == 0)
		{
			nameToTry = name.substr(0, 1);
		}
		else if (attempt == 1)
		{
			nameToTry = name.substr(0, 2);
		}
		else
		{
			nameToTry = name.substr(0, 2) + std::to_string(attempt);
		}
		attempt++;
	} while (std::find(usedShortNames.begin(), usedShortNames.end(), nameToTry) != usedShortNames.end());

	usedShortNames.push_back(nameToTry);
	return nameToTry;
}

string getMinMaxTupleStringHelper(mx::InputPtr inputPtr, const string& attrName)
{
	if (inputPtr->hasAttribute(attrName))
	{
		string val = inputPtr->getAttribute(attrName);
		return "true, " + val;
	}
	else
	{
		return "false, 0.0f";
	}
}

string CreateFloatAttribute(mx::InputPtr inputPtr, vector<string>& usedShortNames, const vector<mx::OutputPtr>& outputs)
{
	string shortName = getShortName(usedShortNames, inputPtr);
	const string strDefValue = inputPtr->getDefaultValue()->getValueString();

	float min = std::numeric_limits<float>::min();
	float max = std::numeric_limits<float>::max();

	float softmin = min;
	float softmax = max;

	string name = inputPtr->getName();

	string minmax = string_format("{std::tuple<bool, float> (%s), std::tuple<bool, float> (%s), std::tuple<bool, float> (%s), std::tuple<bool, float> (%s) }",
		getMinMaxTupleStringHelper(inputPtr, "uimin").c_str(),
		getMinMaxTupleStringHelper(inputPtr, "uimax").c_str(), 
		getMinMaxTupleStringHelper(inputPtr, "uiSoftMin").c_str(), 
		getMinMaxTupleStringHelper(inputPtr, "uiSoftMax").c_str());

	string callStr;
	callStr = string_format("\t%s = CreateFloatAttribute(\"%s\", \"%s\", %s, %s, %s);\n", 
		name.c_str(),
		name.c_str(),
		shortName.c_str(),
		inputPtr->getDefaultValue()->getValueString().c_str(),
		minmax.c_str(),
		outputs[0]->getName().c_str());
	
	return callStr;
}

string CreateIntAttribute(mx::InputPtr inputPtr, vector<string>& usedShortNames, const vector<mx::OutputPtr>& outputs)
{
	string shortName = getShortName(usedShortNames, inputPtr);
	const string strDefValue = inputPtr->getDefaultValue()->getValueString();

	string name = inputPtr->getName();

	string callStr;
	callStr = string_format("\t%s = CreateIntAttribute(\"%s\", \"%s\", %s, %s, %s);\n",
		name.c_str(),
		name.c_str(),
		shortName.c_str(),
		inputPtr->getDefaultValue()->getValueString().c_str(),
		inputPtr->getAttribute("uimin").c_str(),
		inputPtr->getAttribute("uimax").c_str(),
		outputs[0]->getName().c_str());

	return callStr;

}

string CreateBooleanAttribute(mx::InputPtr inputPtr, vector<string>& usedShortNames, const vector<mx::OutputPtr>& outputs)
{
	string shortName = getShortName(usedShortNames, inputPtr);
	const string strDefValue = inputPtr->getDefaultValue()->getValueString();

	string name = inputPtr->getName();

	string callStr;
	callStr = string_format("\t%s = CreateBooleanAttribute(\"%s\", \"%s\", %s, %s);\n",
		name.c_str(),
		name.c_str(),
		shortName.c_str(),
		inputPtr->getDefaultValue()->getValueString().c_str(),
		outputs[0]->getName().c_str());

	return callStr;
}

string CreateColor3Attribute(mx::InputPtr inputPtr, vector<string>& usedShortNames, const vector<mx::OutputPtr>& outputs)
{
	string shortName = getShortName(usedShortNames, inputPtr);
	const string strDefValue = inputPtr->getDefaultValue()->getValueString();

	string name = inputPtr->getName();

	string defaultColorVector = "{" + strDefValue + "}";

	string callStr;
	callStr = string_format("\t%s = CreateColor3Attribute(\"%s\", \"%s\", %s, %s);\n",
		name.c_str(),
		name.c_str(),
		shortName.c_str(),
		defaultColorVector.c_str(),
		outputs[0]->getName().c_str());

	return callStr;
}

string CreateFloatArrayAttribute(unsigned int count, mx::InputPtr inputPtr, vector<string>& usedShortNames, const vector<mx::OutputPtr>& outputs)
{
	string shortName = getShortName(usedShortNames, inputPtr);
	const string strDefValue = inputPtr->getDefaultValue()->getValueString();

	string name = inputPtr->getName();

	string defaultColorVector = "{" + strDefValue + "}";

	string callStr;
	callStr = string_format("\t%s = CreateFloatArrayAttribute<%d>(\"%s\", \"%s\", %s, %s);\n",
		name.c_str(),
		count,
		name.c_str(),
		shortName.c_str(),
		defaultColorVector.c_str(),
		outputs[0]->getName().c_str());

	return callStr;
}

string CreateAppropriateAttribute(mx::InputPtr inputPtr, vector<string>& usedShortNames, const vector<mx::OutputPtr>& outputs)
{
	const string type = inputPtr->getType();

	if (type == "float")
	{
		return CreateFloatAttribute(inputPtr, usedShortNames, outputs);
	}
	else if (type == "integer")
	{
		return CreateIntAttribute(inputPtr, usedShortNames, outputs);
	}
	else if (type == "boolean")
	{
		return CreateBooleanAttribute(inputPtr, usedShortNames, outputs);
	}
	else if (type == "color3")
	{
		return CreateColor3Attribute(inputPtr, usedShortNames, outputs);
	}
	else if (type == "vector2")
	{
		return CreateFloatArrayAttribute(2, inputPtr, usedShortNames, outputs);
	}
	else if (type == "vector3")
	{
		return CreateFloatArrayAttribute(3, inputPtr, usedShortNames, outputs);
	}
	else if (type == "vector4")
	{
		return CreateFloatArrayAttribute(4, inputPtr, usedShortNames, outputs);
	}
	else
	{
		cout << "Attribute type is not supported: " << type << ". Attribute name: " << inputPtr->getName() << endl;
	}

	return "";
}

void ProcessOutput(mx::OutputPtr outputPtr, string& attrObjectList, string& attrCreationList, string& classifyVariableName, vector<string>& usedShortNames)
{
	string name = outputPtr->getName();
	attrObjectList += "\tMObject " + name + ";\n";

	if (outputPtr->getType() == "surfaceshader")
	{
		classifyVariableName = "shaderClassify";
	}
	else
	{
		classifyVariableName = "utilityClassify";
	}
}

void ProcessInput(mx::InputPtr inputPtr, string& attrObjectList, string& attrCreationList, vector<string>& usedShortNames, const vector<mx::OutputPtr>& outputs)
{
	mx::StringVec attrNameList = inputPtr->getAttributeNames();

	const string name = inputPtr->getName();

	attrObjectList += "\tMObject " + name + ";\n";

	attrCreationList += CreateAppropriateAttribute(inputPtr, usedShortNames, outputs);
}


void ProcessNodeDef(mx::NodeDefPtr nodeDefPtr, const string& strHTemplate, const string& strCppTemplate)
{
	static const char* NodeClassNamePlaceHolder = "<NodeClassName>";
	static const char* NodeNamePlaceHolder = "<NodeName>";
	static const char* AttributeObjectListPlaceHolder = "<Attribute_Object_List>";
	static const char* AttributeCreationListPlaceHolder = "<Attribute_Creation_List>";
	static const char* ClassifyVariableNamePlaceHolder = "<ClassifyVariableName>";

	const string className = nodeDefPtr->getName();

	// Just very simple code to make nodeName by omitting "ND_" prefix in node definition
	const string prefixToOmit = "ND_";
	const string nodeName = className.substr(prefixToOmit.length(), className.length() - prefixToOmit.length());

	cout << "Processing NodeDef: " << className << endl;

	string attrObjectList;
	string attrCreationList;
	string classifyVariableName;

	vector<string> usedShortNames;
	
	vector<mx::OutputPtr> outputs = nodeDefPtr->getOutputs(); 
	if (outputs.size() > 1)
	{
		cout << "WARNING: more than 1 output detected: className: " << className << endl;
	}

	ProcessOutput(outputs[0], attrObjectList, attrCreationList, classifyVariableName, usedShortNames);

	vector<mx::InputPtr> inputs = nodeDefPtr->getInputs();

	for (const mx::InputPtr inputPtr : inputs)
	{
		ProcessInput(inputPtr, attrObjectList, attrCreationList, usedShortNames, outputs);
	}

	std::string fileOutputPath = "../../Scripts/MayaMaterialX/" + className;
	// Writing Header File
	string strOutput = strHTemplate;
	ReplaceAll(strOutput, NodeClassNamePlaceHolder, className);
	ofstream outputHFile(fileOutputPath + ".h");
	outputHFile << strOutput;
	outputHFile.close();

	// Writing Cpp file
	strOutput = strCppTemplate;
	ReplaceAll(strOutput, NodeClassNamePlaceHolder, className);
	ReplaceAll(strOutput, NodeNamePlaceHolder, nodeName);
	ReplaceAll(strOutput, AttributeObjectListPlaceHolder, attrObjectList);
	ReplaceAll(strOutput, AttributeCreationListPlaceHolder, attrCreationList);
	ReplaceAll(strOutput, ClassifyVariableNamePlaceHolder, classifyVariableName);

	ofstream outputCppFile(fileOutputPath + ".cpp");
	outputCppFile << strOutput;
	outputCppFile.close();
}

void ProcessFile(const string& file, const string& strHTemplate, const string& strCppTemplate)
{
	cout << "Opening file: " << file << endl;

	try
	{
		mx::DocumentPtr mtlxDocumentPtr = mx::createDocument();

		mx::loadLibrary(file, mtlxDocumentPtr);

		ifstream templateFile;

		vector<mx::NodeDefPtr> nodeDefs = mtlxDocumentPtr->getNodeDefs();

		for (mx::NodeDefPtr nodeDefPtr : nodeDefs)
		{
			ProcessNodeDef(nodeDefPtr, strHTemplate, strCppTemplate);
		}
	}
	catch (const mx::Exception& ex)
	{
		std::cout << "mx::Exception occured while processing " << file << endl << "Message: " << ex.what() << endl;
	}
	catch (...)
	{
		std::cout << "Unknown exception occured while processing " << file << endl;
	}
}

int main()
{
	typedef std::tuple<std::string, std::string> MtlxFileTuple;

	std::string libraryPath = USD_BUILD + "libraries/";

	std::vector<MtlxFileTuple> filesToParse =
	{ 
		std::tuple<std::string, std::string> ( "PBR", libraryPath + "bxdf/standard_surface.mtlx" )
		//, std::tuple<std::string, std::string> ( "USD", libraryPath + "bxdf/usd_preview_surface.mtlx")
		//, std::tuple<std::string, std::string> ( "STD", libraryPath + "stdlib/stdlib_defs.mtlx")
		//, std::tuple<std::string, std::string> ( "PBR", libraryPath + "pbrlib/pbrlib_defs.mtlx")
		//, std::tuple<std::string, std::string> ( "ALG", libraryPath + "alglib/alglib_defs.mtlx")
	};

	std::ifstream templateHFile("../RPR_MX_Template_H");
	std::ifstream templateCppFile("../RPR_MX_Template_Cpp");

	if (!templateHFile.is_open())
	{
		std::cout << "Template Header file not found " << endl;
		return -1;
	}

	if (!templateCppFile.is_open())
	{
		std::cout << "Template Cpp file not found " << endl;
		return -1;
	}

	std::string strHTemplate((std::istreambuf_iterator<char>(templateHFile)),
			std::istreambuf_iterator<char>());

	std::string strCppTemplate((std::istreambuf_iterator<char>(templateCppFile)),
		std::istreambuf_iterator<char>());


	for (const MtlxFileTuple& tup : filesToParse)
	{
		ProcessFile(std::get<1>(tup), strHTemplate, strCppTemplate);
	}

	return 0;
}