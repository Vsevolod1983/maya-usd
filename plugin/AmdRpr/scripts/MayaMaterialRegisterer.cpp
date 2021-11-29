#include "MayaMaterialRegisterer.h"
#include "../materialgenerator/RPR_MX_NodeName1.h"
#include "../scripts/MayaMaterialX/ND_UsdUVTexture.h"
#include "../scripts/MayaMaterialX/ND_UsdPrimvarReader_integer.h"



void MayaMaterialRegisterer::registerNodes(INodeRegisterer* pINodeRegisterer)
{
	RPR_MX_NodeName1::registerNode(pINodeRegisterer, 1);
	ND_UsdUVTexture::registerNode(pINodeRegisterer, 2);
	ND_UsdPrimvarReader_integer::registerNode(pINodeRegisterer, 3);
}

void MayaMaterialRegisterer::deregisterNodes(INodeRegisterer* pINodeRegisterer)
{
	RPR_MX_NodeName1::deregisterNode(pINodeRegisterer);
	ND_UsdUVTexture::deregisterNode(pINodeRegisterer);
	ND_UsdPrimvarReader_integer::deregisterNode(pINodeRegisterer);
}

