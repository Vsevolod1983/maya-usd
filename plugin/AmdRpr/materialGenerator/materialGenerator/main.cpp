#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>

#include "MaterialXCore/Document.h"
#include "MaterialXFormat/File.h"
#include "MaterialXFormat/Util.h"

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

	while ((pos = inout.find(inout, pos)) != std::string::npos) {
		inout.replace(pos, replaceWith.length(), replaceWith);
		start_pos += to.length(); // ...
	}
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

	vector<mx::InputPtr> inputs = nodeDefPtr->getInputs();

	for (const mx::InputPtr inputPtr : inputs)
	{
		mx::StringVec attrNameList = inputPtr->getgetAttributeNames();

		for (const string& attr : attrNameList)
		{
			const string& str = inputPtr->getAttribute(attr);
		}
	}

	std::string fileOutputPath = "../Scripts/MayaMaterialX/" + className;
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

void ProcessFile(const string& file)
{
	cout << "Opening file: " << file << endl;

	try
	{
		mx::DocumentPtr mtlxDocumentPtr = mx::createDocument();

		mx::loadLibrary(file, mtlxDocumentPtr);

		ifstream templateFile;

		std::ifstream templateHFile("RPR_MX_Template_H");
		std::ifstream templateCppFile("RPR_MX_Template_Cpp");

		std::string strTemplate((std::istreambuf_iterator<char>(templateFile)),
			std::istreambuf_iterator<char>());

		vector<mx::NodeDefPtr> nodeDefs = mtlxDocumentPtr->getNodeDefs();

		for (mx::NodeDefPtr nodeDefPtr : nodeDefs)
		{
			ProcessNodeDef(nodeDefPtr, strTemplate);
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

	for (const MtlxFileTuple& tup : filesToParse)
	{
		ProcessFile(std::get<1>(tup));
	}

	return 0;
}