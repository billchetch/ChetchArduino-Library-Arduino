#include "ChetchUtils.h"
#include "ChetchMCP2515Node.h"


namespace Chetch{
    MCP2515Node::MCP2515Node(byte nodeID, int csPin) : MCP2515Device(nodeID, csPin)
    { }

}
