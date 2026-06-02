#include "ChetchUtils.h"
#include "ChetchMCP2515Node.h"


namespace Chetch{

    MCP2515Node::MCP2515Node(byte nodeID, int csPin, unsigned int presenceInterval) : MCP2515Device(nodeID, csPin, presenceInterval)
    { 

        //Add master node as message filter
    }


    bool MCP2515Node::begin(){
        //must call init so that the filters aren't erased by parent begin method
        init();

        //add filter for 'targeted' message types (bit 5 = 0, bit 4 = 1, bit 3,2,1 = x)
        uint32_t mask = 0x18000000; //bits 5 and 4 of first byte //MESSAGE_TYPES_MASK; 
        MCP2515::ERROR err = mcp2515.setFilterMask(MCP2515::MASK::MASK0, true, mask);
        if(err != MCP2515::ERROR_OK){
            return false;
        }

        //Set first filter to targeted messages
        uint32_t filter = 0x08000000; //targeted messages BIT 5 = 0, BIT 4 = 1
        err = mcp2515.setFilter(MCP2515::RXF::RXF0, true, filter);
        if(err != MCP2515::ERROR_OK){
            return false;
        }
        //Set second filter zero to avoid "random" data passing a match test
        err = mcp2515.setFilter(MCP2515::RXF::RXF1, true, filter);
        if(err != MCP2515::ERROR_OK){
            return false;
        }

        //Now we deal with dependencies
        if(firstDependency != NULL){
            mask = 0x18F00000; //we allow braodcast message types AND node ID (bits 8-5 of second byte) filter combinations
            err = mcp2515.setFilterMask(MCP2515::MASK::MASK1, true, mask);
            if(err != MCP2515::ERROR_OK){
                return false;
            }

            /*Serial.println("MASK");
            for (int n = 32; n >= 0; n--) {
                Serial.print(bitRead(mask, n));
                if(n % 8 == 0 && n != 32 && n > 0)Serial.print("-");
            }
            Serial.println();
            Serial.println("----");*/

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
        }

        return MCP2515Device::begin();
    }
}
