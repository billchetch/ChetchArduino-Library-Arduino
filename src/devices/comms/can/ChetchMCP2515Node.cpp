#include "ChetchUtils.h"
#include "ChetchMCP2515Node.h"


namespace Chetch{

    MCP2515Node::MCP2515Node(byte nodeID, int csPin, unsigned int presenceInterval) : MCP2515Device(nodeID, csPin, presenceInterval)
    { 
    }


    bool MCP2515Node::begin(){
        //must call init so that the filters aren't erased by parent begin method
        init();

        //Now we deal with dependencies
        /*if(firstDependency != NULL){
            mask = 0x18F00000; //we allow braodcast message types AND node ID (bits 8-5 of second byte) filter combinations
            err = mcp2515.setFilterMask(MCP2515::MASK::MASK1, true, mask);
            if(err != MCP2515::ERROR_OK){
                return false;
            }

            

            filter = 0x00000000; //targeted messages BIT 5 = 0, BIT 4 = 0
            NodeDependency* nd = firstDependency;
            int i = 2; 
            do{
                uint32_t ndFilter = filter | ((uint32_t)nd->getNodeID() << 20);
                err = mcp2515.setFilter((MCP2515::RXF)i, true, ndFilter);
                if(err != MCP2515::ERROR_OK){
                    return false;
                }
                i++;
                nd = nd->next;
            }while (nd != NULL);

            //check we haven't exceeded filters
            if(i > 5)return false;
            
            while(i <= 5){
                err = mcp2515.setFilter((MCP2515::RXF)i, true, 0xFFFFFFFF);
                if(err != MCP2515::ERROR_OK){
                    return false;
                }
                i++;
            }
        }*/

        return MCP2515Device::begin();
    }

    void MCP2515Node::setReportInfo(ArduinoMessage* message){
        message->add(statusRequestCount);
        message->add(statusResponseCount);
    }
            
}
