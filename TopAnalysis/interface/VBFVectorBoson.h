#ifndef _VBFVectorBoson_h_
#define _VBFVectorBoson_h_

#include "TLorentzVector.h"
#include "TopLJets2015/TopAnalysis/interface/ObjectTools.h"
#include "TopLJets2015/TopAnalysis/interface/SelectionTools.h"
#include <TFile.h>
#include <TROOT.h>
#include <TH1.h>
#include <TH2.h>
#include <TSystem.h>
#include <TGraph.h>
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
#include "TRandom3.h"
#include "TMath.h"
using namespace std;

//Vector boson will be either Z or photon at the moment

struct Category{
  float MM,A,VBF,HighPt,HighPtVBF,V1J;
  Category(float * cat){
    MM = cat[0];
    A = cat[1];
    VBF = cat[2];
    HighPt = cat[3];
    HighPtVBF = cat[4];
    V1J = cat[5];
  };
  Category(){
    MM = 0;
    A = 0;
    VBF = 0;
    HighPt = 0;
    HighPtVBF = 0; 
    V1J = 0; 
  };
  void set(float * cat){
    MM = cat[0];
    A = cat[1];
    VBF = cat[2];
    HighPt = cat[3];
    HighPtVBF = cat[4];
    V1J = cat[5];
  };
};

class VBFVectorBoson{
public:
	VBFVectorBoson(TString filename_,
                       TString outname_,
                       Int_t anFlag_,
                       TH1F *normH_, 
                       TH1F *genPU_,
                       TString era_,
                       Bool_t debug_=false, Bool_t skimtree_=false):
	filename(filename_),outname(outname_),anFlag(anFlag_), era(era_), debug(debug_), skimtree(skimtree_)
	{
	  normH = normH_ ? (TH1F*)normH_->Clone("normH_c") : 0;
	  genPU = genPU_ ? (TH1F*)genPU_->Clone("genPu_c") : 0;
	  fMVATree = NULL;
	  newTree = NULL;
	  init();
	  setXsecs();
          rnd.SetSeed(123456789);
	};
	~VBFVectorBoson(){}
	void init(){
		this->readTree();
		cout << "...producing " << outname << " from " << nentries << " events" << endl;
		this->prepareOutput();
		this->bookHistograms();
		this->loadCorrections();
		if(skimtree){
			this->addMVAvars();
		}
		selector = new SelectionTool(filename, debug, triggerList,SelectionTool::VBF);
		std::cout << "init done" << std::endl;
	}

	void saveHistos();
	void readTree();
	void prepareOutput();
	void bookHistograms();
	void setGammaZPtWeights();
	void loadCorrections();
	void addMVAvars();
	void fill(MiniEvent_t ev, TLorentzVector boson, std::vector<Jet> jets, std::vector<double> cplotwgts, TString c);
	void RunVBFVectorBoson();



private:
	TString filename, outname, baseName;
	Int_t anFlag, nentries;
	TH1F * normH, * genPU;
	TString era;
	TH1* triggerList;
	Bool_t debug, skimtree, isQCDEMEnriched;
	TFile * f /*inFile*/, *fMVATree, *fOut;
	TTree * t /*inTree*/, *newTree /*MVA*/;
  	HistTool * ht;
	MiniEvent_t ev;
        float sihih,chiso,r9,hoe, ystar,relbpt,dphibjj;
	double mindrl;

	TRandom3 rnd;
	
  	//LUMINOSITY+PILEUP
	LumiTools * lumi;
  
  	//LEPTON EFFICIENCIES
	EfficiencyScaleFactorsWrapper * gammaEffWR;
  
  	//JEC/JER
  	JECTools * jec;
	//Photon/Z pt weights
  	std::map<TString,TGraph *> photonPtWgts;
  	std::map<TString,std::pair<double,double> > photonPtWgtCtr;

	//Variables to be added to the MVA Tree
	float centraleta, forwardeta, jjetas, centjy, ncentjj, dphivj0, dphivj1, dphivj2, dphivj3;
	float evtWeight, mjj, detajj , dphijj ,jjpt;
	float isotropy, circularity,sphericity,	aplanarity, C, D;
	float scalarht,balance, mht, training;

	/////////////////////////////////////
	// Categorie for VBF:              //
	//   MM:A:VBF:HighPt:HighPtVBF:V1J // 
	/////////////////////////////////////
	Category category;

	SelectionTool * selector;

	/////////////////////////////////////////
	// To select events for training       //
	/////////////////////////////////////////

	int useForTraining(){
	  double myRnd = rnd.Rndm();
	  if (myRnd >= 0.5) return 1;
	  return 0;
	}

	/////////////////////////////////////////
	// Quick and ugly cross section getter //
	// To be updated such as to use the    //
	// json file                           //
	/////////////////////////////////////////
	std::map<TString, float> xsecRefs;

	void setXsecs(){
	  xsecRefs["MC13TeV_TTJets"        ] = 832;
	  xsecRefs["MC13TeV_ZZ"            ] = 0.5644; 
	  xsecRefs["MC13TeV_WZ"            ] = 47.13;  
	  xsecRefs["MC13TeV_WW"            ] = 12.178; 
	  xsecRefs["MC13TeV_SingleTbar_tW" ] = 35.85;  
	  xsecRefs["MC13TeV_SingleT_tW"    ] = 35.85;  
	  xsecRefs["MC13TeV_DY50toInf"     ] = 5765.4; 
	  xsecRefs["MC13TeV_QCDEM_15to20"  ] = 2302200;
	  xsecRefs["MC13TeV_QCDEM_20to30"  ] = 5352960;
	  xsecRefs["MC13TeV_QCDEM_30to50"  ] = 9928000;
	  xsecRefs["MC13TeV_QCDEM_50to80"  ] = 2890800;
	  xsecRefs["MC13TeV_QCDEM_80to120" ] = 350000; 
	  xsecRefs["MC13TeV_QCDEM_120to170"] = 62964;  
	  xsecRefs["MC13TeV_QCDEM_170to300"] = 18810;  
	  xsecRefs["MC13TeV_QCDEM_300toInf"] = 1350;  
	  xsecRefs["MC13TeV_GJets_HT40to100" ] = 20790 ;
	  xsecRefs["MC13TeV_GJets_HT100to200"] = 9238;  
	  xsecRefs["MC13TeV_GJets_HT200to400"] = 2305;  
	  xsecRefs["MC13TeV_GJets_HT400to600"] = 274.4; 
	  xsecRefs["MC13TeV_GJets_HT600toInf"] = 93.46; 
	  xsecRefs["MC13TeV_EWKZJJ"        ] = 4.32;
	}
	
	float getXsec(){
	  for (auto const& x : xsecRefs){
	      if (filename.Contains(x.first))
		return x.second;
	  }
	  return 1;
	}
	
};
#endif
