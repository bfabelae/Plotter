/////////////////////////////////
//////// PLOTTER CLASS //////////
/////////////////////////////////

/*

Beef of the Plotter code; this takes in all of the
normalized histograms, puts them into a canvas and formats
them all together to look nice.  Most of the functions are for
aesthetics, so to change the look of the graphs, change the Plotter
Functions.  Is a little cumbersome and could use more work into
putting more ability into the style config files/options.

Current state is still work in progress, but does make nice graphs
right now.  may need to revisit when submitting paper and comments
about the graph are more important.

 */



#ifndef _PLOTTER_H_
#define _PLOTTER_H_

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
#include <TLatex.h>
#include <THStack.h>
#include <TPaveText.h>
#include <TGraphErrors.h>
#include <TLegend.h>
#include <TF1.h>
#include <TStyle.h>
#include <TROOT.h>
#include <cmath>
#include <vector>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unordered_map>
#include "tokenizer.hpp"
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <array>
#include <sstream>
#include <iomanip>
#include <regex>


#include "Normalizer.h"
#include "Style.h"
#include "Logfile.h"


enum Bottom {SigLeft, SigRight, SigBoth, SigBin, Ratio};



using namespace std;

class Plotter {
 public:
  void CreateStack( TDirectory*, Logfile&); ///fix plot stuff
  void addFile(Normer&);

  int getSize();
  vector<string> getFilenames(string option="all");
  void setStyle(Style&);
  void setBottomType(Bottom input) {bottomType = input;}
  void setSignificanceSSqrtB() {ssqrtsb = false;}
  void setNoBottom() {onlyTop = true;}
  void getPresetBinning(string);


 private:
  TList* FileList[3] = {new TList(), new TList(), new TList()};
  Style styler;
  // int color[9] = {100, 90, 80, 70, 60, 50, 40, 30, 20};

  //   int color[17] = {kRed, 51, kMagenta, kYellow, kGreen, kCyan, kRed-9, kYellow-10, kGreen+2, kCyan-10, kBlue-4, kViolet, kMagenta-10,  kBlack, kOrange+6, kPink-8, kGray+1 };
  //int color[9] = {kRed, 51, kMagenta, kYellow, kGreen, kCyan, kRed-9, kYellow-10, kGreen+2};
  //int color[9] = {kRed, kBlue, kOrange+6, kYellow, kGreen, kCyan, kWhite, kPink-8, kMagenta};

 // Color scheme for the stacked histogram: colors are assigned according to alphabetic order but displayed in ascending order of number of events.
 //int color[9] = {kBlue-9, 432-9, 23, kOrange+6, kRed-7, kYellow-9, kGreen-10, kPink-8, kMagenta};
 //int color[14] = {kRed+1, kGreen+2, kOrange+7, kPink-6, kOrange, kBlue-9, kCyan+1, kAzure+7, kBlue-7, kViolet-2, kPink-2, kTeal-6, kAzure-6, kOrange-7};
 // int color[15] = {kSpring-1, kBlue-4, kYellow-4, kGreen+2, kPink-8, kAzure-4, kAzure+1, kViolet+6, kRed+1, kOrange, kOrange-3, kTeal-1, kMagenta+2, kBlue-9, kViolet+1};  // Brenda.
  int color[16] = {kRed, kOrange+1, kYellow-7, kGreen+1, kBlue-4, kViolet-9, kMagenta+1, kAzure+10, kRed-9, kYellow-3, kCyan-9, kGreen+2,  kAzure+1, kViolet-1, kRed-7, kGreen-8}; 

  bool ssqrtsb = true, onlyTop = false;
  Bottom bottomType = Ratio;
  static unordered_map<string, string> latexer;

  string newLabel(string);
  string listParticles(string);
  void setXAxisTop(TH1*, TH1*, THStack*);
  void setYAxisTop(TH1*, TH1*, double, THStack*);
  void setXAxisBot(TH1*, double);
  void setYAxisBot(TAxis*, TH1*, double);
  void setYAxisBot(TAxis*, TList*, double);

  TH1D* printBottom(TH1D*, TH1D*);
  TList* signalBottom(const TList*, const TH1D*);
  TList* signalBottom(const TList*, const TH1D*, const TH1D*);

  THStack* sortStack(THStack*);
  TLegend* createLeg(const TH1*, const TList*, const TList*);
  TGraphErrors* createError(const TH1*, bool);
  void sizePad(double, TVirtualPad*, bool);
  TF1* createLine(TH1*);

  vector<double> rebinner(const TH1*, double);

  unordered_map<string, vector<pair<int, double>>> explicitBins;

  THStack* rebinStack(THStack*, const double*, int);
  void divideBin(TH1*, TH1*,THStack*, TList*);
};

#endif
