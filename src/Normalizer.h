//////////////////////////////////////
////////// NORMALIZER CLASS //////////
//////////////////////////////////////

/*

Almost totally ripped from the hadd function.  Just adds the 
normalization functionality.  Uses same format for the Plotter, but
not as ripped off.

 */

#ifndef _NORMALIZER_H_
#define _NORMALIZER_H_

#include <TTree.h>
#include <TH1.h>
#include <TFile.h>
#include <TDirectory.h>
#include <TClass.h>
#include <TKey.h>
#include <TGraphAsymmErrors.h>
#include <TChain.h>
#include <TCanvas.h>
#include <TText.h>
#include <THStack.h>
#include <TPaveText.h>
#include <TGraphErrors.h>
#include <TLegend.h>
#include <TF1.h>
#include <TStyle.h>
#include <TROOT.h>
#include <TEfficiency.h>

#include <vector>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <map>
#include "tokenizer.hpp"
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <array>
#include <cmath>

using namespace std;

class Normer {
 public:  
  Normer();
  Normer(vector<string>);
  Normer(const Normer& other);
  Normer& operator=(const Normer& rhs);
  ~Normer();

  void setUse() {if(use == 2) use = 1;}
  vector<string> input;
  vector<double> skim, xsec, SF;
  string output, type="", sampletype="";
  double lumi;

  TList* FileList;
  vector<double> normFactor;
  bool isData=false;
  bool isDataDriven=false;
  int use=3;

  
  void setValues(vector<string>);
  void setLumi(double);
  int shouldAdd(string, string);
  int getModTime(const char*);
  void MergeRootfile( TDirectory*);
  double getBayesError(double pass, double full);
  void print();
};

#endif
