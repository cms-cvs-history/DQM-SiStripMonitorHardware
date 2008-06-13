import FWCore.ParameterSet.Config as cms

source = cms.Source("NewEventStreamFileReader",
    skipEvents = cms.untracked.uint32(0),
    fileNames = cms.untracked.vstring('/store/data/GlobalNov07/A/000/030/356/GlobalNov07.00030356.0001.A.storageManager.0.0000.dat')
)


