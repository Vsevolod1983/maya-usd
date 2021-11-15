#include "MayaMaterialRegisterer.h"
#include "../materialgenerator/RPR_MX_NodeName1.h"


void MayaMaterialRegisterer::registerNodes(INodeRegisterer* pINodeRegisterer)
{
	RPR_MX_NodeName1::registerNode(pINodeRegisterer, 1);
}

void MayaMaterialRegisterer::deregisterNodes(INodeRegisterer* pINodeRegisterer)
{
	RPR_MX_NodeName1::deregisterNode(pINodeRegisterer);
}

