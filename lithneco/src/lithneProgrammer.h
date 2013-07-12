/*
 * wireless_programming.h
 *
 * Created: 4-7-2013 13:45:02
 *  Author: Elco
 */ 


#ifndef LITHNE_PROGRAMMER_H_
#define LITHNE_PROGRAMMER_H_


/** Lithne Preprocessor Code - Handles uploading
*
* We forward all incoming lithne messages directly to the main processor so the user can handle them there
* HOWEVER we catch all messages in the scope "LithneProgrammingScope" (1737) and processes them here
*
* We receive program 'packets' from the Lithne uploader software, packets contain 64 bytes that are then stored in the 'page buffer'
* Code for the main processor is written in 'pages'; the page buffer contains 512 bytes that are copied to the main processor when the buffer is filled
* After each received packet, we request the next packet
*
* The first packet contains 2 extra bytes providing info on the total number of packets that can be expected
* These 2 additional bytes are stripped from the buffer
*
*/

#include <inttypes.h>
#include "pageBuffer.h"
#include "nodeInfo.h"

/** Define Function Numbers **/   // Must be the same as defined in the processing sketch
enum LithneProgrammer_function_numbers{
	fCheckingIn=1,
	fResetMain,
	fProgramming,
	fNodeName,
	fLastUpload,
	fFileName,
	fRequestNextPacket,
	fUploadCompleted,
	fCodeProgrammed,	
};

#define REMOTE 0                //  Use node at ID 0 as remote
#define PROGRAM_TIMEOUT 3000u     // Value determines the number of millis() after which the programming times out (no packs received)
static const uint16_t lithneProgrammingScope = 1737;


class LithneProgrammer{
	public:
	
	LithneProgrammer();
	~LithneProgrammer();
	
	void init(HardwareSerial * serialPtr);
	void updateRemoteAddress();
	void processPacket();
	void processCheckin();
	void processNodeName();
	void processFileName();
	void processLastUpload();
	void processKill();
	void program( bool firstPacket );
	uint8_t readByteWithDelay();
	int copyPage(int pageNum);
	void checkUploadProgress();
	bool busyProgramming();
	void preventHangup();
	
	void resetMain(void);
	void setMainReset(bool holdInReset);
	void resetXbee(void);
	void setXbeeReset(bool holdInReset);
	
	/** Define buffer size **/
	static const uint16_t packetSize = 64;                  //   Bytes per XBee Packet
	
	private:
	PageBuffer pbuff;
	NodeInfo nodeInfo;
	
	unsigned long lastPacketTimer;   // Used to determine time-out
	bool doneUploading;
	bool programSucces;
	volatile bool programming;
	uint16_t packetsIncoming, packetsReceived;    // Keep track of the expected number of incoming packets and the number of packets received so far
	uint16_t lastPacketRequest;		              // To keep track of our last request so we can request again if it takes too long
	uint16_t pageNumber;                           // Keep track of the number of pages programmed
	HardwareSerial * progSerial;

};

extern LithneProgrammer lithneProgrammer;

#endif /* LITHNE_PROGRAMMER_H_ */