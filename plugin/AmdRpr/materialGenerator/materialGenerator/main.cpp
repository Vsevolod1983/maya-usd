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

const std::string USD_BUILD = "C:\\Projects\\USD\\USDBuild/";

using namespace std;
namespace mx = MaterialX;

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

string CreateFloatAttribute(mx::InputPtr inputPtr, vector<string>& usedShortNames, const vector<mx::OutputPtr>& outputs)
{
	ostringstream os;
	string shortName = getShortName(usedShortNames, inputPtr);
	const string strDefValue = inputPtr->getDefaultValue()->getValueString();

	os << "\t" << inputPtr->getName() << " = " << "nAttr.create(\"" << inputPtr->getName() <<"\", \"" << shortName << "\",  MFnNumericData::kFloat, " << strDefValue  << ", &status);" << endl;
	os << "\tnAttr.setMin(\"" << inputPtr->getAttribute("uimin") << "\");" << endl;
	os << "\tnAttr.setMax(\"" << inputPtr->getAttribute("uimax") << "\");" << endl;

	os << "\tstatus = addAttribute(" << outputs[0]->getName() << ");" << endl;
	os << "\tstatus = attributeAffects(" << inputPtr->getName() << "," << outputs[0]->getName() << ");\n" << endl;
	
	return os.str();
	//inputPtr->getName()
}

string CreateAppropriateAttribute(mx::InputPtr inputPtr, vector<string>& usedShortNames, const vector<mx::OutputPtr>& outputs)
{
	const string type = inputPtr->getType();

	if (type == "float")
	{
		return CreateFloatAttribute(inputPtr, usedShortNames, outputs);
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

	//for (const string& attr : attrNameList)
	//{
	//	const string& str = inputPtr->getAttribute(attr);
	//}
}


void ProcessNodeDef(mx::NodeDefPtr nodeDefPtr, const string& strCppTemplate, const string& strHTemplate)
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
	strOutput = strHTemplate;
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