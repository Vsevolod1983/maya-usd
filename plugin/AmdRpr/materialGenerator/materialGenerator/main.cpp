#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <assert.h>

#include "MaterialXCore/Document.h"
#include "MaterialXFormat/File.h"
#include "MaterialXFormat/Util.h"

#include <sstrean>

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

string CreateFloatAttribute(mx::InputPtr inputPtr)
{
	ostringstream os;
	string creationString;
	os << inputPtr->getName() << " = " << nAttr.create" << endl;
	
	//inputPtr->getName()
}

string CreateAppropriateAttribute(const string& type, mx::InputPtr inputPtr)
{

}

void ProcessOutput(mx::OutputPtr outputPtr, string& attrObjectList, string& attrCreationList, string& classifyVariableName)
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

void ProcessInput(mx::InputPtr inputPtr, string& attrObjectList, string& attrCreationList)
{
	mx::StringVec attrNameList = inputPtr->getAttributeNames();

	const string name = inputPtr->getName();

	attrObjectList += "\tMObject " + name + ";\n";

	const string type = inputPtr->getAttribute("type");

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

	const mx::StringVec strVec = nodeDefPtr->getAttributeNames();

	vector<mx::OutputPtr> outputs = nodeDefPtr->getOutputs(); 
	assert(outputs.size() == 1);
	ProcessOutput(outputs[0], attrObjectList, attrCreationList, classifyVariableName);

	vector<mx::InputPtr> inputs = nodeDefPtr->getInputs();

	for (const mx::InputPtr inputPtr : inputs)
	{
		ProcessInput(inputPtr, attrObjectList, attrCreationList);
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