 #pragma once

#include "base/api.h"

#include <mayaUsd/fileio/jobs/jobArgs.h>

#include <pxr/pxr.h>

#include <maya/MFileObject.h>
#include <maya/MPxFileTranslator.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

namespace MAYAUSD_NS_DEF {

/// File translator for USD files. Handles the USD option in the Export window.
class RPRMaterialXMayaTranslator : public MPxFileTranslator
{
public:
    MAYAUSD_PLUGIN_PUBLIC
    static const MString translatorName;

    /**
     * method to create UsdMayaExportTranslator file translator
     */
    MAYAUSD_PLUGIN_PUBLIC
    static void* creator();

    MAYAUSD_PLUGIN_PUBLIC
    MStatus writer(
        const MFileObject&                file,
        const MString&                    optionsString,
        MPxFileTranslator::FileAccessMode mode) override;

	MAYAUSD_PLUGIN_PUBLIC
		MStatus reader(
			const MFileObject&                file,
			const MString&                    optionsString,
			MPxFileTranslator::FileAccessMode mode) override;

    bool haveReadMethod() const override { return true; }
    bool haveWriteMethod() const override { return true; }

    MAYAUSD_PLUGIN_PUBLIC
    MFileKind identifyFile(const MFileObject& file, const char* buffer, short size) const override;

    MString defaultExtension() const override
    {
        return "mtlx";
    }
    MString filter() const override
    {
		return "*.mtlx";// PXR_NS::UsdMayaTranslatorTokens->UsdWritableFileFilter.GetText();
    }

    MAYAUSD_PLUGIN_PUBLIC
    static const std::string& GetDefaultOptions();

private:
    RPRMaterialXMayaTranslator();
    RPRMaterialXMayaTranslator(const RPRMaterialXMayaTranslator&);
    ~RPRMaterialXMayaTranslator() override;
    RPRMaterialXMayaTranslator& operator=(const RPRMaterialXMayaTranslator&);
};

} // namespace MAYAUSD_NS_DEF
