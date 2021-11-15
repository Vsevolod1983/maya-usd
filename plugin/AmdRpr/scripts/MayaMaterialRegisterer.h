#pragma once

#include "INodeRegisterer.h"

class MayaMaterialRegisterer
{
public:
	static void registerNodes(INodeRegisterer* pINodeRegisterer);
	static void deregisterNodes(INodeRegisterer* pINodeRegisterer);
};

