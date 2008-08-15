#include <iostream>

#include "DataFormats/FEDRawData/interface/FEDHeader.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"  
#include "DQM/SiStripMonitorHardware/interface/Fed9UDebugEvent.hh"
#include "DQM/SiStripMonitorHardware/interface/Fed9UEventAnalyzer.hh"
#include "CondFormats/SiStripObjects/interface/FedChannelConnection.h"

Fed9UEventAnalyzer::Fed9UEventAnalyzer(std::pair<int,int> newFedBoundaries,
				       bool doSwap, bool doPreSwap ) {
  // First of all we instantiate the Fed9U object of the event
  fedEvent_ = NULL; 
  fedIdBoundaries_ = newFedBoundaries;
  swapOn_=doSwap;
  preSwapOn_=doPreSwap;
  thisFedId_=0;
}


Fed9UEventAnalyzer::Fed9UEventAnalyzer(Fed9U::u32* data_u32, Fed9U::u32 size_u32,
				       std::pair<int,int> newFedBoundaries,
				       bool doSwap, bool doPreSwap ) {
  Fed9UEventAnalyzer(newFedBoundaries,doSwap, doPreSwap);

  Initialize(data_u32, size_u32);
}

Fed9UEventAnalyzer::~Fed9UEventAnalyzer() {
  if (fedEvent_) delete fedEvent_;
}

bool Fed9UEventAnalyzer::Initialize(Fed9U::u32* data_u32, Fed9U::u32 size_u32) {

  // Let's perform some cross-check...
  if(data_u32 == NULL){ 
    edm::LogWarning("MissingData") << "Fed9U data pointer is NULL";
    return false;
  }
	
  // Ignores buffers of zero size (container (fed ID) is present but contains nothing) 
  if (!size_u32) {
    edm::LogInfo("MissingData") << "Fed9U data size is zero";
    return false;
  }
  
  //if data is old VME then header needs to be swapped before using FEDHeader
  bool swapHeader = ( ( (data_u32[0] & 0xF0000000) == 0x50000000 ) &&          //check begining mark in DAQ header
                      ( (data_u32[size_u32-2] & 0xF0000000) == 0xA0000000 ) && //check end mark in DAQ trailer
                      ( (data_u32[size_u32-2] & 0x00FFFFFF)*2 == size_u32 )    //check size
                    );
  //if data is old SLink then whole buffer needs to be swapped before using Fed9UDebugEvent
  bool swapBuffer = ( ( (data_u32[1] & 0xF0000000) == 0x50000000 ) &&          //check begining mark in DAQ header
                      ( (data_u32[size_u32-1] & 0xF0000000) == 0xA0000000 ) && //check end mark in DAQ trailer
                      ( (data_u32[size_u32-1] & 0x00FFFFFF)*2 == size_u32 )    //check size
                    );
  //new format works with both provided new enough Fed9UEvent is used
  
  Fed9U::u32 header[2];
  if (swapHeader) {
    header[0] = data_u32[1];
    header[1] = data_u32[0];
  } else {
    header[0] = data_u32[0];
    header[1] = data_u32[1];
  }
  // We are now initializing the fedHeader in order to check that
  // the buffer is SiStripTracker's and it is valid
  FEDHeader fedHeader( reinterpret_cast<const unsigned char*>(header) );

  /*// Adjusts the buffer pointers for the DAQ header and trailer present when FRLs are running
  // additonally preforms "flipping" of the bytes in the buffer
  if(preSwapOn_){
    Fed9U::u32 temp1,temp2;
		
    // 32 bit word swapping for the real FED buffers	
    for(unsigned int i = 0; i < (size_u32 - 1); i+=2){	
      temp1 = *(data_u32+i);
      temp2 = *(data_u32+i+1);
      *(data_u32+i) = temp2;
      *(data_u32+i+1) = temp1;
    }
    }


  // We are now initializing the fedHeader in order to check that
  // the buffer is SiStripTracker's and it is valid
  FEDHeader fedHeader( reinterpret_cast<const unsigned char*>(data_u32) );*/

  // Fisrt let's check that the header is not malformed
  if ( ! fedHeader.check() ) {
    edm::LogWarning("CorruptData") << "FED header is corrupt";
    return false;
  }

  // Here we also check that the FEDid corresponds to a tracker one
  thisFedId_=fedHeader.sourceID();
  if ( (thisFedId_<fedIdBoundaries_.first) || (thisFedId_>fedIdBoundaries_.second) ) {
    edm::LogInfo("SkipData") << "FED with ID" << thisFedId_ << "is not a Tracker FED";
    return false;
  }
  
  /*//dump
  std::cout << "Dumping start of buffer" << std::endl;
  for (unsigned int i=0; i<size_u32/2; i++) {
    std::cout << std::dec << "word: " << i << ": " << std::hex << data_u32[2*i+1] << " " << std::hex << data_u32[2*i] << std::endl;
  }
  std::cout << std::dec << "End of dump" << std::endl;*/

  // Adjusts the buffer pointers for the DAQ header and trailer present when FRLs are running
  // additonally preforms "flipping" of the bytes in the buffer
  if(swapBuffer){

    Fed9U::u32 temp1,temp2;
		
    // 32 bit word swapping for the real FED buffers	
    for(unsigned int i = 0; i < (size_u32 - 1); i+=2){	
      temp1 = *(data_u32+i);
      temp2 = *(data_u32+i+1);
      *(data_u32+i) = temp2;
      *(data_u32+i+1) = temp1;
    }
  }


  //#define DO_DUMP_BUFFERS
#ifdef DO_DUMP_BUFFERS

  //dumps a specified number of 32 bit words to the screen prior ot initalization			
  std::cerr << "BUFFERD FED NUMBER " << std::dec << thisFedId_ << std::endl;
	
  for(int i = 0; i<int(size_u32/2.); i++)  { // prints out the specified number of 32 bit words
    std::cerr << std::setiosflags(std::ios::right)
	      << "64 Bit Word #  " 
	      << std::dec << std::setfill(' ') << std::setw(4) 
	      << i << " "
	      << std::hex << std::setfill('0') << std::setw(8) 
	      << (data_u32[2*i] & 0xFFFFFFFF) << " "
	      << std::hex << std::setfill('0') << std::setw(8) 
	      << (data_u32[(2*i + 1)] & 0xFFFFFFFF) << " "
	      << std::endl;
  }
#endif
  
  // The actual event initialization, catching its possible exceptions
  try{
    // Initialize the fedEvent with offset for slink
    fedEvent_ = new Fed9U::Fed9UDebugEvent( data_u32, 0, size_u32 );
  } catch ( const ICUtils::ICException& e ) {
    fedEvent_ = NULL;
    std::stringstream ss;
    ss << "Caught ICUtils::ICException in Fed9UDebugEvent constructor with message:"
       << std::endl << e.what();
    ss << std::endl << "Marking event as corrupt.";
    edm::LogInfo("FEDEventInitException") << ss.str();
    //return false;
  }

  return true;
}


Fed9UErrorCondition Fed9UEventAnalyzer::Analyze(bool useConns, const std::vector<FedChannelConnection>* conns) {
  Fed9UErrorCondition result;
  
  // **********************************
  // *                                *
  // * Initialize the result variable *
  // *                                *
  // **********************************

  // TODO: tidy up this piece of code
  result.problemsSeen = 0;
  if (!useConns) {
    if (fedEvent_) result.totalChannels = fedEvent_->totalChannels();
    else result.totalChannels = 0;
  } else {
    result.totalChannels = 0;
    if (conns) {
      for (int channelIndex=0; channelIndex<96; channelIndex++) {
	if ((conns->at(95-channelIndex)).isConnected()) result.totalChannels++;
      }
    } else {
      edm::LogWarning("Fed9UEventAnalyzer") << "for some reason I have useConns == true and conns==NULL)";
    }
  }


  // Clear the FED errors
  result.internalFreeze       = false;
  result.bxError              = false;
  result.corruptBuffer        = false;

  // Clear the FPGA errors
  for (unsigned int fpga=0; fpga <8; fpga++) {
    result.feMajorAddress[fpga]  = 0x0;
    result.feOverflow[fpga]      = false;
    result.feEnabled[fpga]       = false;
    result.apvAddressError[fpga] = false;
  }

  // Clear the APV and channel errors
  for (unsigned int channelIndex=0; channelIndex<96; channelIndex++) {
    result.channel[channelIndex]=0;
    result.apv[channelIndex*2]=0;
    result.apv[channelIndex*2+1]=0;
  }
  
  //clear the buffer status
  result.qdrFull = false;
  result.qdrPartialFull = false;
  result.qdrEmpty = false;
  result.l1aFull = false;
  result.l1aPartialFull = false;
  result.l1aEmpty = false;
  result.slinkFull = false;

  
  //check that fedEvent was properly constructed. If not then it is corrupt
  if (!fedEvent_) {
    result.corruptBuffer = true;
    return result;
  }
    
  // **********************************
  // *                                *
  // * FED error checks (BE FPGA)     *
  // *                                *
  // **********************************

  //if buffer is corrupt then can't really rely on anything else
  if (fedEvent_->getBufferCorrupt()) {
    result.corruptBuffer = true;
    return result;
  }
  
  // The main error condition is the FED freeze
  if (fedEvent_->getInternalFreeze()) {
    result.internalFreeze = true;
    return result;
  }

  // The second internal condition is the
  // Failure in bx counting (FED screwed)
  if (fedEvent_->getBXError()) {
    result.bxError = true;
    return result;
  }
  
  // **********************************
  // *                                *
  // * FED buffer checks (BE FPGA)     *
  // *                                *
  // **********************************
  
  if (fedEvent_->getQDRFull()) result.qdrFull = true;
  if (fedEvent_->getQDRPartialFull()) result.qdrPartialFull = true;
  if (fedEvent_->getQDREmpty()) result.qdrEmpty = true;
  if (fedEvent_->getL1AFull()) result.l1aFull = true;
  if (fedEvent_->getL1APartialFull()) result.l1aPartialFull = true;
  if (fedEvent_->getL1AEmpty()) result.l1aEmpty = true;
  if (fedEvent_->getSLinkFull()) result.slinkFull = true;
  
  

  result.apveAddress = fedEvent_->getSpecialApvEmulatorAddress();


  // **********************************
  // *                                *
  // * FE FPGA Main cycle and checks  *
  // *                                *
  // **********************************

  // We cycle over the FPGAs of the FED board
  for(unsigned int fpga = 0; fpga < 8 ; fpga++){

    // Enter into the FPGA only if it is enabled
    if( fedEvent_->getFeEnabled(fpga) ){

      result.feEnabled[fpga] = true;
      
      // Look only at FEs with no overflow
      if (fedEvent_->getFeOverflow(fpga)) {

	// Report the overflow and update the number of error channels
	result.feOverflow[fpga] = true;
	if ((useConns)&&(conns)) {
	  for (unsigned int fi=0; fi<12; fi++) 
	    if ((conns->at(95-(12*fpga+fi))).isConnected()) result.problemsSeen++;
	} else {
	  result.problemsSeen+=12;
	}
	
      } else {

	// TODO: add APV Address Error bit (i.e. x-check with APVe)

	// FE Address verification portion (for later use)
	result.feMajorAddress[fpga] = fedEvent_->getFeMajorAddress(fpga);
	
	// **********************************
	// *                                *
	// * Checks of 12 fibers in a FE    *
	// *                                *
	// **********************************	
	
	// This cycles over the fibers of the FPGA
	for (unsigned int fi=0; fi<12; fi++) {

	  // Local index to access the result channel vector
	  unsigned channelIndex=12*fpga+fi;

	  bool readChannel = true;
	  if (useConns) {
	    if (conns) {
	      if (!(conns->at(95-channelIndex)).isConnected()) {
		readChannel = false;
	      }
	    } else {
	      edm::LogWarning("Fed9UEventAnalyzer") << "for some reason I have useConns == true and conns==NULL)";
	    }
	  }
	  
	  if (readChannel) {
	    bool APV1Error       = fedEvent_->getAPV1Error(fpga,fi);
	    bool APV2Error       = fedEvent_->getAPV2Error(fpga,fi);
	    bool APV1WrongHeader = fedEvent_->getAPV1WrongHeader(fpga,fi);
	    bool APV2WrongHeader = fedEvent_->getAPV2WrongHeader(fpga,fi);
	    bool outOfSync       = fedEvent_->getOutOfSync(fpga,fi);
	    bool unlocked        = fedEvent_->getUnlocked(fpga,fi);
	    
	    bool anyError =
	      APV1Error ||
	      APV2Error ||
	      APV1WrongHeader ||
	      APV2WrongHeader ||
	      outOfSync ||
	      unlocked ;
	    
	    outOfSync     = outOfSync && (!unlocked);
	    
	    bool badAPV1  = (APV1Error || APV1WrongHeader) && (!outOfSync) && (!unlocked);
	    bool badAPV2  = (APV2Error || APV2WrongHeader) && (!outOfSync) && (!unlocked);
	    
	    if (anyError) {
	      result.problemsSeen++;
	      
	      if (unlocked) {
		result.channel[channelIndex]=FIBERUNLOCKED;
	      }
	      if (outOfSync) {
		result.channel[channelIndex]=FIBEROUTOFSYNCH;
	      }
	      if (badAPV1) {
		result.apv[channelIndex*2]=BADAPV;
	      }
	      if (badAPV2) {
		result.apv[channelIndex*2+1]=BADAPV;
	      }
	    }
	  }
	  
	} // Fiber loop end
	
	
      } // FE no overflow
      
    } // if FE enabled
  } // for FE fpga loop 
  
  return result;
}
