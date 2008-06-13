import FWCore.ParameterSet.Config as cms

HardwareMonitor = cms.EDAnalyzer("CnBAnalyzer",
    swapOn = cms.untracked.bool(True) ## non zero value does the DAQ header offset, etc.

)


