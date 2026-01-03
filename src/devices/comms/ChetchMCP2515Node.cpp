#include "ChetchUtils.h"
#include "ChetchMCP2515Node.h"


namespace Chetch{
    MCP2515Node::MCP2515Node(byte nodeID, unsigned long presenceInterval, int csPin) : MCP2515Device(nodeID, presenceInterval, csPin)
    { 

        //Add master node as message filter
    }

}
