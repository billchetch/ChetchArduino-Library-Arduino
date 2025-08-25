#include "ChetchUtils.h"
#include "ChetchMCP2515Node.h"


namespace Chetch{
    MCP2515Node::MCP2515Node(byte nodeID, int csPin) : MCP2515Device(nodeID, csPin)
    { }

    bool MCP2515Node::begin(){
        if(getNodeID() <= MASTER_NODE_ID){
            begun = false;
            return begun;
        } else {
            return MCP2515Device::begin();
        }
	}
}
