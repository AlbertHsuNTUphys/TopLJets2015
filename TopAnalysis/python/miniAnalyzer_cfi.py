import FWCore.ParameterSet.Config as cms

analysis = cms.EDAnalyzer("MiniAnalyzer",
                          saveTree               = cms.bool(True),
                          savePF                 = cms.bool(True),
                          useRawLeptons          = cms.bool(False),
                          triggerBits            = cms.InputTag("TriggerResults","","HLT"),
                          prescales              = cms.InputTag("patTrigger"),
                          triggersToUse          = cms.vstring('HLT_Ele35_eta2p1_WPTight_Gsf_v',
                                                               'HLT_Ele28_eta2p1_WPTight_Gsf_HT150_v',
                                                               'HLT_Ele30_eta2p1_WPTight_Gsf_CentralPFJet35_EleCleaned_v',
                                                               'HLT_IsoMu24_2p1_v',
                                                               'HLT_IsoMu27_v',
                                                               'HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ',
                                                               'HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_Mass8_v',
                                                               'HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_Mass3p8_v',
                                                               'HLT_Ele23_Ele12_CaloIdL_TrackIdL_IsoVL_v',
                                                               'HLT_Ele23_Ele12_CaloIdL_TrackIdL_IsoVL_DZ_v',
                                                               'HLT_Mu23_TrkIsoVVL_Ele12_CaloIdL_TrackIdL_IsoVL_v',
                                                               'HLT_Mu23_TrkIsoVVL_Ele12_CaloIdL_TrackIdL_IsoVL_DZ_v',
                                                               'HLT_Mu12_TrkIsoVVL_Ele23_CaloIdL_TrackIdL_IsoVL_DZ_v',
                                                               'HLT_Mu8_TrkIsoVVL_Ele23_CaloIdL_TrackIdL_IsoVL_DZ_v',
                                                               'HLT_Photon150_v',
                                                               'HLT_Photon175_v',
                                                               'HLT_Photon200_v',
                                                               'HLT_Photon50_R9Id90_HE10_IsoM_v',
                                                               'HLT_Photon75_R9Id90_HE10_IsoM_v',
                                                               'HLT_Photon90_R9Id90_HE10_IsoM_v',
                                                               'HLT_Photon75_R9Id90_HE10_IsoM_EBOnly_PFJetsMJJ300DEta3_v',
                                                               'HLT_Photon75_R9Id90_HE10_IsoM_EBOnly_PFJetsMJJ600DEta3_v'
                                                               ),
                          rho                    = cms.InputTag("fixedGridRhoFastjetAll"),
                          vertices               = cms.InputTag("offlineSlimmedPrimaryVertices"),                          
                          muons                  = cms.InputTag("slimmedMuons"),                          
                          electrons              = cms.InputTag("slimmedElectrons"),
                          photons                = cms.InputTag("slimmedPhotons"),
                          jets                   = cms.InputTag('slimmedJets'),
                          metFilterBits          = cms.InputTag("TriggerResults","","PAT"),
                          metFiltersToUse        = cms.vstring('Flag_HBHENoiseFilter',
                                                               'Flag_HBHENoiseIsoFilter',
                                                               'Flag_EcalDeadCellTriggerPrimitiveFilter',
                                                               'Flag_goodVertices',
                                                               'Flag_eeBadScFilter',
                                                               'Flag_globalTightHalo2016Filter'), 
                          badChCandFilter        = cms.InputTag('BadChargedCandidateFilter'),
                          badPFMuonFilter        = cms.InputTag('BadPFMuonFilter'),
                          mets                   = cms.InputTag('slimmedMETs'),                          
                          puppimets              = cms.InputTag('slimmedMETsPuppi'),
                          pfCands                = cms.InputTag('packedPFCandidates'),
                          ctppsLocalTracks       = cms.InputTag('ctppsLocalTrackLiteProducer')
                          )
