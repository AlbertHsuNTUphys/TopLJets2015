#include <TFile.h>
#include <TROOT.h>
#include <TH1.h>
#include <TH2.h>
#include <TSystem.h>
#include <TGraph.h>
#include <TLorentzVector.h>
#include <TGraphAsymmErrors.h>

#include "TopLJets2015/TopAnalysis/interface/MiniEvent.h"
#include "TopLJets2015/TopAnalysis/interface/CommonTools.h"
#include "TopLJets2015/TopAnalysis/interface/VBFVectorBoson.h"
#include "TopLJets2015/TopAnalysis/interface/EfficiencyScaleFactorsWrapper.h"

#include "PhysicsTools/CandUtils/interface/EventShapeVariables.h"

#include <vector>
#include <set>
#include <iostream>
#include <algorithm>
#include "TMath.h"

using namespace std;

//TODOS
// pedro: jet distribution before pu id in boson+1jet
//

//
void VBFVectorBoson::RunVBFVectorBoson()
{
  ///////////////////////
  // LOOP OVER EVENTS //
  /////////////////////
  float xsec = getXsec();
  for (Int_t iev=0;iev<nentries;iev++)
    {
      t->GetEntry(iev);
      if(iev%10000==0) printf ("\r [%3.0f%%] done", 100.*(float)iev/(float)nentries);
      std::vector<double>plotwgts(1,1.0);
      ht->fill("qscale",ev.g_qscale,plotwgts);
      
      //assign randomly a run period
      TString period = lumi->assignRunPeriod();
      
      //////////////////
      // CORRECTIONS //
      ////////////////      
      jec->smearJetEnergies(ev);
             
      ///////////////////////////
      // RECO LEVEL SELECTION //
      /////////////////////////
      TString chTag = selector->flagFinalState(ev);
      std::vector<Particle> &photons     = selector->getSelPhotons(); 
      std::vector<Particle> &leptons     = selector->getSelLeptons(); 
      std::vector<Jet>      &alljets     = selector->getJets();  
      std::vector<Jet> jets;

      //Pileup jet id
      for(auto j : alljets) {
        int idx=j.getJetIndex();
        int jid=ev.j_id[idx];
        bool passLoosePu((jid>>2)&0x1);
        if(!passLoosePu) continue;
        jets.push_back(j);
      }
      //Category selection
      if(chTag!="A" && chTag!="MM") continue;
      float cat[6] = {0,0,0,0,0,0};
      if(chTag == "A") cat[1] = 1;
      if(chTag == "MM") cat[0] = 1;

      //jet related variables and selection
      mjj = (jets.size()>=2 ?  (jets[0]+jets[1]).M() : 0.);
      detajj = (jets.size()>=2 ? fabs(jets[0].Eta()-jets[1].Eta()) : -1.);
      dphijj = (jets.size()>=2 ? jets[0].DeltaPhi(jets[1]) : -1.);
      jjpt = (jets.size()>=2 ? (jets[0]+jets[1]).Pt() : 0.);

      scalarht = 0.;
      TLorentzVector mhtP4(0,0,0,0);
      mht = 0;
      for(auto j : jets) {
        scalarht += j.Pt();
        mhtP4 += j;
      }
      mht = mhtP4.Pt();
      bool passJets(jets.size()>=2 && mjj>400);
      bool passVBFJetsTrigger(passJets && detajj>3.0);

      //categorize the event according to the boson kinematics
      //for the photon refine also the category according to the trigger  bit
      TLorentzVector boson(0,0,0,0);     
      bool isHighPt(false),isVBF(false),isHighPtAndVBF(false),isBosonPlusOneJet(false);
      sihih = 0, chiso = 0 ,r9 = 0, hoe = 0;
      if(chTag=="A") {        
        boson += photons[0];
        sihih = ev.gamma_sieie[photons[0].originalReference()];
        chiso = ev.gamma_chargedHadronIso[photons[0].originalReference()];
        r9    = ev.gamma_r9[photons[0].originalReference()];
        hoe   = ev.gamma_hoe[photons[0].originalReference()];
        isVBF    = (selector->hasTriggerBit("HLT_Photon75_R9Id90_HE10_IsoM_EBOnly_PFJetsMJJ300DEta3_v", ev.triggerBits) 
                    && photons[0].Pt()>75 
                    && fabs(photons[0].Eta())<1.442
                    && passVBFJetsTrigger);
        isHighPt = ( selector->hasTriggerBit("HLT_Photon200_v", ev.triggerBits) 
                     && photons[0].Pt()>200 );       
        isHighPtAndVBF = (isHighPt && isVBF);
        isBosonPlusOneJet=(isHighPt && alljets.size()==1);

        //veto prompt photons on the QCDEM enriched sample
        if( isQCDEMEnriched && ev.gamma_isPromptFinalState[ photons[0].originalReference() ] ) {
          isVBF          = false;
          isHighPt       = false;
          isHighPtAndVBF = false;
        }
          
      } else {
        boson   += leptons[0];
        boson   += leptons[1];
        isVBF    = boson.Pt()>75 && fabs(boson.Rapidity())<1.442 && passVBFJetsTrigger;
        isHighPt = boson.Pt()>200;
        isHighPtAndVBF = (isHighPt && isVBF);
        isBosonPlusOneJet=(isHighPt && alljets.size()==1);
      }

      if(!isVBF && !isHighPt && !isBosonPlusOneJet) continue;      

      //leptons and boson
      double mindrl(9999.);
      for(auto &l: leptons) mindrl=min(l.DeltaR(boson),mindrl);

      std::vector<TString> chTags;      
      if(isVBF)             {chTags.push_back("VBF"+chTag); cat[2]=1;}
      if(isHighPt)	    {chTags.push_back("HighPt"+chTag); cat[3]=1;}
      if(isHighPtAndVBF)    {chTags.push_back("HighPtVBF"+chTag); cat[4]=1;}
      if(isBosonPlusOneJet) {chTags.push_back("V1J"+chTag); cat[5]=1;}

      category.set(cat);

      //leptons and boson
      mindrl = 9999.;
      for(auto &l: leptons) mindrl=min(l.DeltaR(boson),mindrl);

      //system variables and event shapes
      ystar=0;
      balance=0;
      relbpt=0;
      dphibjj=0;
      if(passJets) {
        ystar=boson.Rapidity()-0.5*(jets[0].Rapidity()+jets[1].Rapidity());
        balance=(boson+jets[0]+jets[1]).Pt();
        relbpt=(jets[0].Pt()+jets[1].Pt())/boson.Pt();
        dphibjj=boson.DeltaPhi( jets[0]+jets[1] );
      }
      else if(jets.size()>0){
        balance=(boson+jets[0]).Pt();
        relbpt=jets[0].Pt()/boson.Pt();
        dphibjj=boson.DeltaPhi(jets[0]);
      }
        
      std::vector<math::XYZVector> inputVectors;
      inputVectors.push_back( math::XYZVector(boson.Px(),boson.Py(),boson.Pz()) );
      for(size_t ij=0; ij<min(size_t(2),jets.size());ij++) {
        inputVectors.push_back( math::XYZVector(jets[ij].Px(),jets[ij].Py(),jets[ij].Pz()) );
      }
      EventShapeVariables esv(inputVectors);
      esv.set_r(1.0);
      isotropy    = esv.isotropy();
      circularity = esv.circularity();
      sphericity  = esv.sphericity();
      aplanarity  = esv.aplanarity();
      C           = esv.C();
      D           = esv.D();
      
      ////////////////////
      // EVENT WEIGHTS //
      //////////////////
      float wgt(1.0);
      if (!ev.isData) {

        // norm weight
        wgt  = (normH? normH->GetBinContent(1) : 1.0);
            
        // pu weight
        ht->fill("puwgtctr",0,plotwgts);
        double puWgt(lumi->pileupWeight(ev.g_pu,period)[0]);
        std::vector<double>puPlotWgts(1,puWgt);
        ht->fill("puwgtctr",1,puPlotWgts);
        
        // photon trigger*selection weights
        float trigSF(1.0), selSF(1.0);
        if(chTag=="A")
          {
            trigSF *= gammaEffWR->getTriggerCorrection({},photons,{}, period).first;
            selSF  *= gammaEffWR->getOfflineCorrection(photons[0], period).first;
          }
        else
          {
            trigSF *=gammaEffWR->getTriggerCorrection(leptons,{},{}, period).first;
            selSF  *=gammaEffWR->getOfflineCorrection(leptons[0], period).first;
            selSF  *=gammaEffWR->getOfflineCorrection(leptons[1], period).first;
          }
        wgt *= puWgt*trigSF*selSF;
        
       
        // generator level weights
        wgt *= (ev.g_nw>0 ? ev.g_w[0] : 1.0);

        //update weight for plotter
        plotwgts[0]=wgt;
      }

      //control histograms
      for( auto c : chTags) {
        std::vector<double> cplotwgts(plotwgts);

        //photon pT weighting
        if(chTag=="A") {
          float photonPtWgt(1.0);
          if(photonPtWgts.find(c)!=photonPtWgts.end()) {
            photonPtWgt=photonPtWgts[c]->Eval(boson.Pt());
            if(photonPtWgt>0) photonPtWgt = 1./photonPtWgt;
            else              photonPtWgt = 1.0;
          }
          photonPtWgtCtr[c].first  += 1.0;
          photonPtWgtCtr[c].second += photonPtWgt;
          cplotwgts[0]*=photonPtWgt;
        } 

	//What is the final weight? 0 or 1 in the array?
	evtWeight = cplotwgts[0]*xsec;
	training = useForTraining(); 
	fill( ev,  boson,  jets,  cplotwgts, c);
       }
    }

  
  //close input file
  f->Close();

  //compute the scale factor needed to keep the normalization
  //due to photon pT weighting
  for(auto &wit : photonPtWgtCtr) {
    if(wit.second.second<=0) wit.second.first=1.0;
    else                     wit.second.first /= wit.second.second;
  }
  
  //save histos to file  
  saveHistos();
  if(skimtree){
    fMVATree->cd();
    newTree->Write();
    fMVATree->Close();
  }
}

void VBFVectorBoson::saveHistos(){
  fOut->cd();
  for (auto& it : ht->getPlots())  { 
    for(auto &wit : photonPtWgtCtr){
      if(!it.first.Contains(wit.first)) continue;
      cout << "Scaling " << it.first << " by "<< wit.second.first <<endl;
      it.second->Scale(wit.second.first);
      break;
    }
    it.second->SetDirectory(fOut); it.second->Write(); 
  }
  for (auto& it : ht->get2dPlots())  { 
    for(auto &wit : photonPtWgtCtr){
      if(!it.first.Contains(wit.first)) continue;
      it.second->Scale(wit.second.first);
      break;
    }
    it.second->SetDirectory(fOut); it.second->Write(); 
  }
  fOut->Close();		
}

void VBFVectorBoson::readTree(){
  f = TFile::Open(filename);
  triggerList=(TH1 *)f->Get("analysis/triggerList");
  t = (TTree*)f->Get("analysis/data");
  attachToMiniEventTree(t,ev,true);
  nentries = t->GetEntriesFast();
  if (debug) nentries = 10000; //restrict number of entries for testing
  t->GetEntry(0);
  isQCDEMEnriched = filename.Contains("MC13TeV_QCDEM");
}

void VBFVectorBoson::prepareOutput(){
  baseName=gSystem->BaseName(outname); 
  TString dirName=gSystem->DirName(outname);
  
  if(skimtree){
    fMVATree=TFile::Open(dirName+"/MVATree_"+baseName,"RECREATE");
    newTree = t->CloneTree(0);
  }
  fOut=TFile::Open(dirName+"/"+baseName,"RECREATE");
  fOut->cd();
}

void VBFVectorBoson::bookHistograms(){
  ht = new HistTool(0);
  ht->addHist("puwgtctr",      new TH1F("puwgtctr",         ";Weight sums;Events",2,0,2));  
  ht->addHist("qscale",        new TH1F("qscale",           ";Q^{2} scale;Events",100,0,2000));  
  ht->addHist("nvtx",          new TH1F("nvtx",             ";Vertex multiplicity;Events",100,-0.5,99.5));  
  ht->addHist("vpt", 	     new TH1F("vectorbosonPt",    ";Boson p_{T}[GeV];Events",25,0,500));  
  ht->addHist("vy", 	     new TH1F("vectorbosony",     ";Boson rapidity;Events",25,-3,3));  
  ht->addHist("mindrl", 	     new TH1F("mindrl",           ";min #Delta R(boson,lepton);Events",25,0,6));  
  ht->addHist("sihih", 	     new TH1F("sihih",            ";#sigma(i#eta,i#eta);Events",50,0,0.1));  
  ht->addHist("hoe", 	     new TH1F("hoe",              ";h/e;Events",25,0,0.1));  
  ht->addHist("r9", 	     new TH1F("r9",               ";r9;Events",25,0,1.0));  
  ht->addHist("chiso", 	     new TH1F("chiso",            ";Charged isolation [GeV];Events",25,0,0.10));  
  ht->addHist("vystar",        new TH1F("vectorbosonystar", ";y-(1/2)(y_{j1}+y_{j2});Events",25,-5,5));  
  ht->addHist("njets",         new TH1F("njets",            ";Jet multiplicity;Events",10,-0.5,9.5));  
  ht->addHist("mjj", 	     new TH1F("mjj",              ";Dijet invariant mass [GeV];Events",40,0,4000));  
  ht->addHist("leadpt",        new TH1F("leadpt",           ";Leading jet p_{T} [GeV];Events",25,0,500));  
  ht->addHist("subleadpt",     new TH1F("subleadpt"   ,     ";Sub-leading jet p_{T} [GeV];Events",25,0,500));  
  ht->addHist("drj1b",         new TH1F("drj1b",            ";#DeltaR(j_{1},boson);Events",25,0,8));  
  ht->addHist("drj2b",         new TH1F("drj2b"   ,         ";#DeltaR(j_{2},boson);Events",25,0,8));  
  ht->addHist("leadpumva",     new TH1F("leadpumva",        ";Pileup MVA;Events",25,-1,1));  
  ht->addHist("subleadpumva",  new TH1F("subleadpumva"   ,  ";Pileup MVA;Events",25,-1,1));  
  ht->addHist("centraleta",    new TH1F("centraleta",       ";Most central jet |#eta|;Events",25,0,5));  
  ht->addHist("forwardeta",    new TH1F("forwardeta",       ";Most forward jet |#eta|;Events",25,0,5));  
  ht->addHist("dijetpt",       new TH1F("dijetpt",          ";Dijet p_{T} [GeV];Events",20,0,1000));  
  ht->addHist("detajj",        new TH1F("detajj" ,          ";#Delta#eta(J,J);Events",20,0,8));  
  ht->addHist("dphijj",        new TH1F("dphijj" ,          ";#Delta#phi(J,J) [rad];Events",20,-3.15,3.15));  
  ht->addHist("ht",            new TH1F("ht",               ";H_{T} [GeV];Events",20,0,4000));  
  ht->addHist("mht",           new TH1F("mht",              ";Missing H_{T} [GeV];Events",20,0,500));  
  ht->addHist("balance",       new TH1F("balance",          ";System p_{T} balance [GeV];Events",20,0,300));  
  ht->addHist("sphericity",    new TH1F("sphericity",       ";Sphericity;Events",20,0,1.0));  
  ht->addHist("aplanarity",    new TH1F("aplanarity",       ";Aplanarity;Events",20,0,1.0));  
  ht->addHist("C",             new TH1F("C",                ";C;Events",20,0,1.0));  
  ht->addHist("D",             new TH1F("D",                ";D;Events",20,0,1.0));  
  ht->addHist("isotropy",      new TH1F("isotropy",         ";Isotropy;Events",20,0,1.0));  
  ht->addHist("circularity",   new TH1F("circularity",      "Circularity;;Events",20,0,1.0));
  // Study of jet variables
  ht->addHist("jet_c1_00", 	  new TH1F("jet_c1_00",          ";Jet shape var. c1_00;Jets",100,-1,1));  
  ht->addHist("jet_c1_02", 	  new TH1F("jet_c1_02",          ";Jet shape var. c1_02;Jets",100,-1,1));  
  ht->addHist("jet_c1_05", 	  new TH1F("jet_c1_05",          ";Jet shape var. c1_05;Jets",100,-1,1));  
  ht->addHist("jet_c2_00", 	  new TH1F("jet_c2_00",          ";Jet shape var. c2_00;Jets",100,-1,1));  
  ht->addHist("jet_c2_02", 	  new TH1F("jet_c2_02",          ";Jet shape var. c2_02;Jets",100,-1,1));  
  ht->addHist("jet_c2_05", 	  new TH1F("jet_c2_05",          ";Jet shape var. c2_05;Jets",100,-1,1));  
  ht->addHist("jet_c3_00", 	  new TH1F("jet_c3_00",          ";Jet shape var. c3_00;Jets",100,-1,1));  
  ht->addHist("jet_c3_02", 	  new TH1F("jet_c3_02",          ";Jet shape var. c3_02;Jets",100,-1,1));  
  ht->addHist("jet_c3_05", 	  new TH1F("jet_c3_05",          ";Jet shape var. c3_05;Jets",100,-1,1));  
  ht->addHist("jet_zg", 	  	  new TH1F("jet_zg",          ";Jet shape var. zg;Jets",100,-1,1));  
  ht->addHist("jet_gaptd", 	  new TH1F("jet_gaptd",          ";Jet shape var. gaptd;Jets",100,-1,1));  
  ht->addHist("jet_gawidth",        new TH1F("jet_gawidth",          ";Jet shape var. gawidth;Jets",100,-1,1));
  //additional variables from https://link.springer.com/content/pdf/10.1140/epjc/s10052-017-5315-6.pdf
  ht->addHist("jjetas", 	  	  new TH1F("jjetas",          ";#eta_{j1}#eta_{j2};Events",200,-25,25));  
  ht->addHist("centjy",		  new TH1F("centjy",          ";Central jet rapidity;Jets",25,0,3));  
  ht->addHist("ncentj", 	  	  new TH1F("ncentjj",          ";Number of central jets;Events",10,-0.5,9.5));  
  ht->addHist("dphivj0", 	  new TH1F("dphivj0",          ";#Delta#phi(V,j0);Jets",20,0,4));  
  ht->addHist("dphivj1", 	  new TH1F("dphivj1",          ";#Delta#phi(V,j1);Jets",20,0,4));  
  ht->addHist("dphivj2", 	  new TH1F("dphivj2",          ";#Delta#phi(V,j2);Jets",20,0,4));  
  ht->addHist("dphivj3", 	  new TH1F("dphivj3",          ";#Delta#phi(V,j3);Jets",20,0,4));
}
void VBFVectorBoson::setGammaZPtWeights(){
  TString wgtUrl("${CMSSW_BASE}/src/TopLJets2015/TopAnalysis/test/analysis/VBFVectorBoson/raw/plots/ratio_plotter.root");
  gSystem->ExpandPathName(wgtUrl);
  TFile *wgtF=TFile::Open(wgtUrl);
  if(wgtF) {
    cout << "Reading photon/Z pT weights" << endl;
    TString pfix(baseName.Contains("Data13TeV_") ? "" : "_mc_MC");
    photonPtWgts["VBFA"]      = new TGraph((TH1* )wgtF->Get("VBFA_vectorbosonPt_ratio/VBFA_vectorbosonPt"+pfix));
    photonPtWgts["HighPtA"]   = new TGraph((TH1* )wgtF->Get("HighPtA_vectorbosonPt_ratio/HighPtA_vectorbosonPt"+pfix));
    photonPtWgtCtr["VBFA"]    = std::pair<double,double>(0.0,0.0);
    photonPtWgtCtr["HighPtA"] = std::pair<double,double>(0.0,0.0);
    wgtF->Close();
  } else {
    cout << "Requested to reweight photon spectrum but could not find " << wgtUrl << endl
         << "Proceeding without" << endl;
  }
}
void VBFVectorBoson::loadCorrections(){
  lumi = new LumiTools(era,genPU);
  gammaEffWR = new EfficiencyScaleFactorsWrapper(filename.Contains("Data13TeV"),era);
  jec = new JECTools(era);
  if(anFlag>0) this->setGammaZPtWeights();
}
void VBFVectorBoson::addMVAvars(){
  newTree->Branch("centralEta", &centraleta);
  newTree->Branch("mjj", &mjj);
  newTree->Branch("detajj", &detajj);
  newTree->Branch("jjpt", &jjpt);
  newTree->Branch("dphijj", &dphijj);
  newTree->Branch("forwardeta", &forwardeta);
  newTree->Branch("jjetas", &jjetas);
  newTree->Branch("centjy", &centjy);
  newTree->Branch("ncentjj", &ncentjj);
  newTree->Branch("dphivj0", &dphivj0);
  newTree->Branch("dphivj1", &dphivj1);
  newTree->Branch("dphivj2", &dphivj2);
  newTree->Branch("dphivj3", &dphivj3);
  newTree->Branch("evtWeight", &evtWeight);
  newTree->Branch("mht", &mht);
  newTree->Branch("balance", &balance);
  newTree->Branch("ht", &scalarht);
  newTree->Branch("isotropy", &isotropy);
  newTree->Branch("circularity",&circularity);
  newTree->Branch("sphericity",&sphericity);
  newTree->Branch("aplanarity",&aplanarity);
  newTree->Branch("C",&C);
  newTree->Branch("D",&D);
  newTree->Branch("training",&training);
  newTree->Branch("category", &category, "MM:A:VBF:HighPt:HighPtVBF:V1J");
}

void VBFVectorBoson::fill(MiniEvent_t ev, TLorentzVector boson, std::vector<Jet> jets, std::vector<double> cplotwgts, TString c){
  ht->fill("nvtx",   ev.nvtx,          cplotwgts,c);        

  //boson histos
  ht->fill("vpt",    boson.Pt(),       cplotwgts,c);
  ht->fill("vy",     boson.Rapidity(), cplotwgts,c);   
  ht->fill("sihih",  sihih,            cplotwgts,c);   
  ht->fill("r9",     r9,               cplotwgts,c);   
  ht->fill("hoe",    hoe,             cplotwgts,c);   
  ht->fill("chiso",  chiso,            cplotwgts,c);   
  ht->fill("mindrl", mindrl,           cplotwgts,c);   

  //jet histos
  centraleta = 9999;
  forwardeta = -9999;
  for(size_t ij=0; ij<min(size_t(2),jets.size());ij++) {
    TString jtype(ij==0?"lead":"sublead");
    ht->fill(jtype+"pt",       jets[ij].Pt(),        cplotwgts,c);          
    ht->fill(jtype+"pumva",    jets[ij].getPUMVA(),  cplotwgts,c);
    ht->fill("dr"+jtype+"b",   jets[ij].DeltaR(boson),  cplotwgts,c);
    ht->fill("jet_c1_00", 	ev.j_c1_00[jets[ij].getJetIndex()]	  ,  cplotwgts,c);
    ht->fill("jet_c1_02", 	ev.j_c1_02[jets[ij].getJetIndex()]	  ,  cplotwgts,c);
    ht->fill("jet_c1_05", 	ev.j_c1_05[jets[ij].getJetIndex()]	  ,  cplotwgts,c);
    ht->fill("jet_c2_00", 	ev.j_c2_00[jets[ij].getJetIndex()]	  ,  cplotwgts,c);
    ht->fill("jet_c2_02", 	ev.j_c2_02[jets[ij].getJetIndex()]	  ,  cplotwgts,c);
    ht->fill("jet_c2_05",	ev.j_c2_05[jets[ij].getJetIndex()]	  ,  cplotwgts,c);
    ht->fill("jet_c3_00", 	ev.j_c3_00[jets[ij].getJetIndex()]	  ,  cplotwgts,c);
    ht->fill("jet_c3_02",	ev.j_c3_02[jets[ij].getJetIndex()]	  ,  cplotwgts,c);
    ht->fill("jet_c3_05",	ev.j_c3_05[jets[ij].getJetIndex()]	  ,  cplotwgts,c);
    ht->fill("jet_zg", 		ev.j_zg[jets[ij].getJetIndex()]	  ,  cplotwgts,c);
    ht->fill("jet_gaptd", 	ev.j_gaptd[jets[ij].getJetIndex()]	  ,  cplotwgts,c);
    ht->fill("jet_gawidth", ev.j_gawidth[jets[ij].getJetIndex()]	  ,  cplotwgts,c);
    centraleta=min(centraleta,float(fabs(jets[ij].Eta())));
    forwardeta=max(forwardeta,float(fabs(jets[ij].Eta())));
  }
  jjetas = 9999;
  dphivj0 = 9999; dphivj1 = 9999;
  if(jets.size() >= 2){
    jjetas = jets[0].Eta()*jets[1].Eta();
    dphivj0 = fabs(jets[0].DeltaPhi(boson));
    dphivj1 = fabs(jets[1].DeltaPhi(boson));
    ht->fill("jjetas",  jjetas,   cplotwgts,c);
    ht->fill("dphivj0", dphivj0 ,  cplotwgts,c);
    ht->fill("dphivj1", dphivj1 ,  cplotwgts,c);
  }
  dphivj2 = 9999; dphivj3 = 9999;
  centjy = 9999; ncentjj = 0;  
  if(jets.size() > 2){
    dphivj2 = fabs(jets[2].DeltaPhi(boson));
    ht->fill("dphivj2", dphivj2 ,  cplotwgts,c);
    for(unsigned int iJet = 2; iJet < jets.size(); iJet++){	
      float dy = fabs(jets[0].Rapidity() - jets[1].Rapidity())/2;
      float sumy = (jets[0].Rapidity() + jets[1].Rapidity())/2;
      if(fabs(jets[iJet].Rapidity() - sumy) < dy){
        centjy =  jets[iJet].Rapidity();
        ht->fill("centjy",centjy,  cplotwgts,c);
        ncentjj++;
      }
    }
    ht->fill("ncentj", ncentjj, cplotwgts, c);
  }
  if(jets.size() > 3){
    dphivj3 =  fabs(jets[3].DeltaPhi(boson));
    ht->fill("dphivj3", dphivj3 , cplotwgts,c);
  }
  ht->fill("njets",        jets.size(), cplotwgts,c);
  ht->fill("ht",           scalarht,    cplotwgts,c);
  ht->fill("mht",          mht,         cplotwgts,c);
  ht->fill("centraleta",   centraleta,      cplotwgts,c);
  ht->fill("forwardeta",   forwardeta,      cplotwgts,c);
  ht->fill("dijetpt",      jjpt,        cplotwgts,c);
  ht->fill("detajj",       detajj,      cplotwgts,c);
  ht->fill("dphijj",       dphijj,      cplotwgts,c);
  ht->fill("mjj", 	        mjj,         cplotwgts,c);
	
  //visible system histos
  ht->fill("vystar",       ystar,              cplotwgts,c);        
  ht->fill("balance",      balance,            cplotwgts,c);
  ht->fill("isotropy",     isotropy,     cplotwgts,c);
  ht->fill("circularity",  circularity,  cplotwgts,c);
  ht->fill("sphericity",   sphericity, cplotwgts,c);
  ht->fill("aplanarity",   aplanarity, cplotwgts,c);
  ht->fill("C",            C,          cplotwgts,c);
  ht->fill("D",            D,          cplotwgts,c);
  cout<< "hist filling is done ... "<<endl;
  if(skimtree) newTree->Fill();
}
