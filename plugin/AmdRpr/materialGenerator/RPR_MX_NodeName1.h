#include <maya/MPxNode.h>
#include "../scripts/INodeRegisterer.h"

class RPR_MX_NodeName1 : public MPxNode
{
public:
	// initialize and type info
	static MTypeId typeID() { return m_typeId; }
	static void* creator();
	static MStatus initialize();

	static void registerNode(INodeRegisterer* pINodeRegisterer, MTypeId typeId);
	static void deregisterNode(INodeRegisterer* pINodeRegisterer);
private:
	static MTypeId m_typeId;
};
