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

vector<string> getChildNames(const string& parentLongName, const string& parentShortName, const string& attrType)
{
	// copied from common.h to avoid inclusion common.h in this project. Might be chnaged though

	bool xyzPattern = true;
	int count = 0;

	vector<string> names;

	if (attrType == "vector2")
	{
		count = 2;
	}
	else if (attrType == "vector3")
	{
		count = 3;
	}
	else if (attrType == "vector4")
	{
		count = 4;
	}
	else if (attrType == "color3")
	{
		count = 3;
		xyzPattern = false;
	}
	else if (attrType == "color4")
	{
		count = 4;
		xyzPattern = false;
	}

	if (count == 0)
	{
		return names;
	}

	static vector<string> childPostfixesXYZW = { "x", "y", "z", "w" };
	static vector<string> childPostfixesRGBA = { "r", "g", "b", "a" };

	string longName = parentLongName;
	string shortName = parentShortName;

	vector<string>& postfixes = xyzPattern ? childPostfixesXYZW : childPostfixesRGBA;

	for (size_t index = 0; index < count; ++index)
	{
		names.push_back(longName + "_" + postfixes[index]);
		names.push_back(shortName + postfixes[index]);
	}

	return names;
}

// true if particular name is found hence we need to continue searching for "free" name
bool checkNameAndAdd(const string& parentLongName, const string& nameToTry, set<string>& usedNames, const mx::PortElementPtr elementPtr)
{
	vector<string> namesToCheck = getChildNames(parentLongName, nameToTry, elementPtr->getType());

	namesToCheck.push_back(nameToTry);

	bool found = false;

	for (const string& name : namesToCheck)
	{
		if (usedNames.find(name) != usedNames.end())
		{
			found = true;
			break;
		}
	}

	if (!found)
	{
		usedNames.insert(namesToCheck.begin(), namesToCheck.end());
		usedNames.insert(nameToTry);
	}

	return found;
}

string getShortName(set<string>& usedNames, const mx::PortElementPtr elementPtr)
{
	string parentLongName = elementPtr->getName();

	int attempt = 0;

	usedNames.insert(parentLongName);

	string nameToTry;
	vector<string> childNames;
	do
	{ 
		// first try just first character
		if (attempt == 0)
		{
			nameToTry = parentLongName.substr(0, 1);
		}
		else if (attempt == 1)
		{
			nameToTry = parentLongName.substr(0, 2);
		}
		else
		{
			nameToTry = parentLongName.substr(0, 2) + std::to_string(attempt);
		}
		attempt++;
	
	} while (checkNameAndAdd(parentLongName, nameToTry, usedNames, elementPtr));

	return nameToTry;
}

string getMinMaxTupleStringHelper(mx::PortElementPtr elementPtr, const string& attrName)
{
	if (elementPtr->hasAttribute(attrName))
	{
		string val = elementPtr->getAttribute(attrName);
		return "true, " + val;
	}
	else
	{
		return "false, 0";
	}
}

template <typename T = float>
string getMinMaxTupleBundleStringHelper(mx::PortElementPtr elementPtr)
{
	string typeString = typeid(T) == typeid(float) ? "float" : "int";

	return string_format("{std::tuple<bool, %s> (%s), std::tuple<bool, %s> (%s), std::tuple<bool, %s> (%s), std::tuple<bool, %s> (%s) }",
		typeString.c_str(),
		getMinMaxTupleStringHelper(elementPtr, "uimin").c_str(),
		typeString.c_str(),
		getMinMaxTupleStringHelper(elementPtr, "uimax").c_str(),
		typeString.c_str(),
		getMinMaxTupleStringHelper(elementPtr, "uiSoftMin").c_str(),
		typeString.c_str(),
		getMinMaxTupleStringHelper(elementPtr, "uiSoftMax").c_str());
}

string getOutputVectorString(const vector<mx::OutputPtr>& outputs)
{
	string retVal = "{";

	for (size_t i = 0; i < outputs.size(); ++i)
	{
		retVal += outputs[i]->getName();

		if (i < outputs.size() - 1)
		{
			retVal += ", ";
		}
	}

	retVal += "}";
	return retVal;
}

string CreateFloatAttribute(mx::PortElementPtr elementPtr, set<string>& usedNames, const vector<mx::OutputPtr>& outputs)
{
	string shortName = getShortName(usedNames, elementPtr);
	string strDefValue;

	mx::ValuePtr defValue = elementPtr->getDefaultValue();
	if (defValue)
	{
		strDefValue = defValue->getValueString();
	}
	else
	{
		strDefValue = "0.0";
	}

	string name = elementPtr->getName();

	string minmax = getMinMaxTupleBundleStringHelper(elementPtr);

	string callStr;
	callStr = string_format("\t%s = CreateFloatAttribute(\"%s\", \"%s\", %s, %s, %s);\n", 
		name.c_str(),
		name.c_str(),
		shortName.c_str(),
		strDefValue.c_str(),
		minmax.c_str(),
		getOutputVectorString(outputs).c_str());
	
	return callStr;
}

string CreateIntAttribute(mx::PortElementPtr elementPtr, set<string>& usedNames, const vector<mx::OutputPtr>& outputs)
{
	string shortName = getShortName(usedNames, elementPtr);

	string strDefValue;

	mx::ValuePtr defValue = elementPtr->getDefaultValue();
	if (defValue)
	{
		strDefValue = defValue->getValueString();
	}
	else
	{
		strDefValue = "0";
	}

	string name = elementPtr->getName();

	string callStr;
	callStr = string_format("\t%s = CreateIntAttribute(\"%s\", \"%s\", %s, %s, %s);\n",
		name.c_str(),
		name.c_str(),
		shortName.c_str(),
		strDefValue.c_str(),
		getMinMaxTupleBundleStringHelper<int>(elementPtr).c_str(),
		getOutputVectorString(outputs).c_str());

	return callStr;

}

string CreateBooleanAttribute(mx::PortElementPtr elementPtr, set<string>& usedNames, const vector<mx::OutputPtr>& outputs)
{
	string shortName = getShortName(usedNames, elementPtr);

	string strDefValue;
	mx::ValuePtr defValue = elementPtr->getDefaultValue();
	if (defValue)
	{
		strDefValue = defValue->getValueString();
	}
	else
	{
		strDefValue = "false";
	}

	string name = elementPtr->getName();

	string callStr;
	callStr = string_format("\t%s = CreateBooleanAttribute(\"%s\", \"%s\", %s, %s);\n",
		name.c_str(),
		name.c_str(),
		shortName.c_str(),
		strDefValue.c_str(),
		getOutputVectorString(outputs).c_str());

	return callStr;
}

string CreateColor3Attribute(mx::PortElementPtr elementPtr, set<string>& usedNames, const vector<mx::OutputPtr>& outputs)
{
	string shortName = getShortName(usedNames, elementPtr);

	string strDefValue;

	mx::ValuePtr defValue = elementPtr->getDefaultValue();
	if (defValue)
	{
		strDefValue = defValue->getValueString();
	}
	else
	{
		strDefValue = "0.0, 0.0, 0.0";
	}

	string name = elementPtr->getName();

	string defaultColorVector = "{" + strDefValue + "}";

	string callStr;
	callStr = string_format("\t%s = CreateColor3Attribute(\"%s\", \"%s\", %s, %s);\n",
		name.c_str(),
		name.c_str(),
		shortName.c_str(),
		defaultColorVector.c_str(),
		getOutputVectorString(outputs).c_str());

	return callStr;
}

string CreateFloatArrayAttribute(unsigned int count, mx::PortElementPtr elementPtr, bool xyzNamingPattern, set<string>& usedNames, const vector<mx::OutputPtr>& outputs)
{
	string shortName = getShortName(usedNames, elementPtr);
	string strDefValue;
	
	mx::ValuePtr defValue = elementPtr->getDefaultValue();
	if (defValue)
	{
		strDefValue = defValue->getValueString();
	}
	else
	{
		for (unsigned int i = 0; i < count; i++)
		{
			strDefValue += "0.0f";
			if (i < count - 1)
			{
				strDefValue += ", ";
			}
		}
	}

	strDefValue = "{" + strDefValue + "}";

	string name = elementPtr->getName();
	string childNamingPattern = string("ChildAttributeNamingPattern::") + (xyzNamingPattern ? "XYZW" : "RGBA");

	string callStr;
	callStr = string_format("\t%s = CreateFloatArrayAttribute<%d>(\"%s\", \"%s\", %s, %s, %s, %s);\n",
		name.c_str(),
		count,
		name.c_str(),
		shortName.c_str(),
		strDefValue.c_str(),
		getMinMaxTupleBundleStringHelper(elementPtr).c_str(),
		childNamingPattern.c_str(),
		getOutputVectorString(outputs).c_str());

	return callStr;
}

string CreateStringAttribute(mx::PortElementPtr elementPtr, set<string>& usedNames, bool isFileName, const vector<mx::OutputPtr>& outputs)
{
	string shortName = getShortName(usedNames, elementPtr);

	/*string strDefValue;
	mx::ValuePtr defValue = elementPtr->getDefaultValue();
	if (defValue)
	{
		strDefValue = defValue->getValueString();
	}*/

	string name = elementPtr->getName();
	string isFileNameStr = isFileName ? "true" : "false";

	string callStr;
	callStr = string_format("\t%s = CreateStringAttribute(\"%s\", \"%s\", %s, %s);\n",
		name.c_str(),
		name.c_str(),
		shortName.c_str(),
		isFileNameStr.c_str(),
		getOutputVectorString(outputs).c_str());

	return callStr;
}

string getQuotedValuesFromEnum(mx::PortElementPtr elementPtr)
{
	string rawEnum = elementPtr->getAttribute("enum");
	string retVal;

	size_t pos = 0;
	size_t commaIndex = 0;

	do 
	{
		commaIndex = rawEnum.find(",", pos);
		size_t endWordIndex = commaIndex;

		if (commaIndex == string::npos)
		{
			endWordIndex = rawEnum.length();
		}

		retVal += "\"" + rawEnum.substr(pos, endWordIndex - pos ) + "\"";

		if (commaIndex != string::npos)
		{
			retVal += ",";
			pos = commaIndex + 1;
		}
	} while (commaIndex != string::npos);

	return retVal;
}

string CreateStringEnumAttribute(mx::PortElementPtr elementPtr, set<string>& usedNames, const vector<mx::OutputPtr>& outputs)
{
	string shortName = getShortName(usedNames, elementPtr);

	string strDefValue;
	mx::ValuePtr defValue = elementPtr->getDefaultValue();
	if (defValue)
	{
		strDefValue = "\"" + defValue->getValueString() + "\"";
	}

	string values = "{" + getQuotedValuesFromEnum(elementPtr) + "}";
	

	string name = elementPtr->getName();

	string callStr;
	callStr = string_format("\t%s = CreateStringEnumAttribute(\"%s\", \"%s\", %s, %s, %s);\n",
		name.c_str(),
		name.c_str(),
		shortName.c_str(),
		values.c_str(),
		strDefValue.c_str(),
		getOutputVectorString(outputs).c_str());

	return callStr;
}


string CreateAppropriateAttribute(mx::PortElementPtr elementPtr, set<string>& usedNames, const vector<mx::OutputPtr>& outputs)
{
	const string type = elementPtr->getType();

	if (type == "float")
	{
		return CreateFloatAttribute(elementPtr, usedNames, outputs);
	}
	else if (type == "integer")
	{
		return CreateIntAttribute(elementPtr, usedNames, outputs);
	}
	else if (type == "boolean")
	{
		return CreateBooleanAttribute(elementPtr, usedNames, outputs);
	}
	else if (type == "color3" || type == "surfaceshader")
	{
		return CreateColor3Attribute(elementPtr, usedNames, outputs);
	}
	else if (type == "color4")
	{
		return CreateFloatArrayAttribute(4, elementPtr, false, usedNames, outputs);
	}
	else if (type == "vector2")
	{
		return CreateFloatArrayAttribute(2, elementPtr, true, usedNames, outputs);
	}
	else if (type == "vector3")
	{
		return CreateFloatArrayAttribute(3, elementPtr, true, usedNames, outputs);
	}
	else if (type == "vector4")
	{
		return CreateFloatArrayAttribute(4, elementPtr, true, usedNames, outputs);
	}
	else if (type == "string")
	{
		if (elementPtr->hasAttribute("enum"))
		{
			return CreateStringEnumAttribute(elementPtr, usedNames, outputs);
		}
		else
		{
			return CreateStringAttribute(elementPtr, usedNames, false, outputs);
		}
	}
	else if (type == "filename")
	{
		return CreateStringAttribute(elementPtr, usedNames, true, outputs);
	}
	else
	{
		cout << "Attribute type is not supported: " << type << ". Attribute name: " << elementPtr->getName() << endl;
	}

	return "";
}

void ProcessOutputs(const vector<mx::OutputPtr>& outputs, string& attrObjectList, string& attrCreationList, string& classifyVariableName, set<string>& usedNames)
{
	if (outputs[0]->getType() == "surfaceshader")
	{
		classifyVariableName = "shaderClassify";
	}
	else
	{
		classifyVariableName = "utilityClassify";
	}

	for (const mx::OutputPtr outputPtr : outputs)
	{
		string name = outputPtr->getName();
		attrObjectList += "\tMObject " + name + ";\n";

		attrCreationList += CreateAppropriateAttribute(outputPtr, usedNames, {});
	}
}

void ProcessInput(mx::InputPtr inputPtr, string& attrObjectList, string& attrCreationList, set<string>& usedNames, const vector<mx::OutputPtr>& outputs)
{
	mx::StringVec attrNameList = inputPtr->getAttributeNames();

	const string name = inputPtr->getName();

	attrObjectList += "\tMObject " + name + ";\n";

	attrCreationList += CreateAppropriateAttribute(inputPtr, usedNames, outputs);
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

	set<string> usedNames;
	
	vector<mx::OutputPtr> outputs = nodeDefPtr->getOutputs(); 

	if (outputs.size() > 0)
	{
		ProcessOutputs(outputs, attrObjectList, attrCreationList, classifyVariableName, usedNames);
	}
	else
	{
		cout << "WARNING: no output detected: " << className << endl;
	}

	//ProcessOutputs(outputs, attrObjectList, attrCreationList, classifyVariableName, usedNames);

	vector<mx::InputPtr> inputs = nodeDefPtr->getInputs();

	for (const mx::InputPtr inputPtr : inputs)
	{
		ProcessInput(inputPtr, attrObjectList, attrCreationList, usedNames, outputs);
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
		, std::tuple<std::string, std::string> ( "USD", libraryPath + "bxdf/usd_preview_surface.mtlx")
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