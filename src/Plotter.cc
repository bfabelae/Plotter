
#include "Plotter.h"
#include <iostream>     // std::cout, std::ios
#include <string>
#include <sstream>
#include <typeinfo>
using namespace std;

///// macro to save space.  Tests if name is in latex map, if not, it just uses the name given
#define turnLatex(x) ((latexer[x] == "") ? x : latexer[x])

const double EPSILON_VALUE = 0.0001;

/////map of latex names.  If want to change how things look, change particle names here
unordered_map<string, string> Plotter::latexer = { {"GenTau", "#tau"}, {"GenHadTau", "#tau_{h}"}, {"GenMuon", "#mu"}, {"TauJet", "#tau"}, {"Muon", "#mu"}, {"DiMuon", "#mu, #mu"}, {"DiTau", "#tau, #tau"}, {"Tau", "#tau"}, {"DiJet", "jj"}, {"Met", "#slash{E}_{T}"}, {"BJet", "b"}};

template <typename T>
string to_string_with_precision(const T a_value, const int n = 6);

/// Main function.  Takes TDirectory with the file you are writing to.  Also takes in logfile
//  Gets all of the graphs from files in the plotter class and puts them in one graph.
void Plotter::CreateStack( TDirectory *target, Logfile& logfile) {

  //// Sets up style here.  Does everytime, just in case.  Probably don't need
  gStyle = styler.getStyle();

  /// Naming to make easier to read.  Aren't actually used much, but just in case for future use
  TList* datalist = FileList[0];
  TList* bglist = FileList[1];
  TList* sglist = FileList[2];

  bool noData = datalist->GetSize() == 0;

  bool do_overflow = styler.getDoOverflow();

  if(!onlyTop && noData) {
    if(sglist->GetSize() == 0) onlyTop = true;
    else if(bottomType == Ratio) {
      cout << "Bottom Plot requested with Signal.  Setting to default of SigLeft:\nchange this using options if needed (run ./Plotter -help for all options" << endl;
      setBottomType(SigLeft);
    }
  } else if(!onlyTop && sglist->GetSize() == 0) {
    if(bottomType != Ratio) {
      cout << "Bottom Plot requested without Signal and not ratio bottom.  Setting to default of Ratio:\nchange this using options if needed (run ./Plotter -help for all options" << endl;
      setBottomType(Ratio);
    }
  }

  //// Require Backgrounds to run
  if(bglist->GetSize() == 0) {
    cout << "No backgrounds given: Aborting" << endl;
    exit(1);
  }
  //// Will run without data, just will remove ratio plot
  if(noData) {
    cout << "No Data given: Plotting without Data" << endl;
  }

  TString path( (char*)strstr( target->GetPath(), ":" ) );
  path.Remove( 0, 2 );

  TFile *firstFile = (TFile*)bglist->First();
  firstFile->cd( path );
  TDirectory *current_sourcedir = gDirectory;
  Bool_t status = TH1::AddDirectoryStatus();
  TH1::AddDirectory(kFALSE);


  //// Loop to write cutflow to logfile
  TH1* events;
  current_sourcedir->GetObject("Events", events);
  //cout << "current_sourcedir->GetName() = " << current_sourcedir->GetName() << endl;  
 
  if(events) {
    //cout << "Si hay eventos... " << endl;
    vector<string> logEff;
    string totalval = "";
    logEff.push_back(current_sourcedir->GetName());

    for(int i=0; i < 3; i++) {
      TFile* nextsource = (TFile*)FileList[i]->First();
      while ( nextsource ) {
      	nextsource->cd(path);
      	gDirectory->GetObject("Events", events);
      	totalval = to_string_with_precision(events->GetBinContent(2), 3); // change precision for more decimal places
        // Brenda FE 08/09/2019
        //cout << " --------------- Plotter, process of writing the log file -------------- " << endl;
        //cout << "totalval (number of events) = " <<  to_string_with_precision(events->GetBinContent(2), 1) << endl;
        //cout << "i = " << i << endl;
        //cout << "to_string_with_precision(events->GetBinError(2), 1) = " << to_string_with_precision(events->GetBinError(2), 1) << endl;
      	if(i != 0) totalval += " $\\pm$ " + to_string_with_precision(events->GetBinError(2), 3);
      	logEff.push_back(totalval);
      	nextsource = (TFile*)FileList[i]->After( nextsource );
      }
//      cout << "Events = " << events->GetBinContent(2) << endl; 
    }
    logfile.addLine(logEff);
    delete events;
  }
  events = NULL;


  // loop over all keys in this directory
  TIter nextkey( current_sourcedir->GetListOfKeys() );
  TKey *key, *oldkey=0;
  while ( (key = (TKey*)nextkey())) {

    //keep only the highest cycle number for each key
    if (oldkey && !strcmp(oldkey->GetName(),key->GetName())) continue;

    TObject *obj = key->ReadObj();
    if ( obj->IsA() ==  TH1D::Class() || obj->IsA() ==  TH1F::Class() ) {

      /// h1 is the reference histogram to grab the other histos.
      /// here we also make the containers for the graphs
      TH1* readObj = (TH1*)obj;
      TH1D* error = new TH1D("error", readObj->GetTitle(), readObj->GetXaxis()->GetNbins(), readObj->GetXaxis()->GetXmin(), readObj->GetXaxis()->GetXmax());
      TH1D* datahist = new TH1D("data", readObj->GetTitle(), readObj->GetXaxis()->GetNbins(), readObj->GetXaxis()->GetXmin(), readObj->GetXaxis()->GetXmax());
      TList* sigHists = new TList();
      THStack *hs = new THStack(readObj->GetName(),readObj->GetName());

      /*------------data--------------*/

      //////  This iterates over all the different files in the
      //// FileList array.  Looks a little dirty, but makes the code compact and
      //// not messy.  If statements based on array position (ie which type of file)
      //// tell were to put the histograms.  If adding things, go to respective
      //// if statement.  Want to make styling more robust here, but will take some
      //// annoying configuration stuff.  Maybe later

      int nfile = 0;
 

      for(int i = 0; i < 3; i++) {
	TFile* nextfile = (TFile*)FileList[i]->First();
	while ( nextfile ) {
	  nextfile->cd( path );
	  TKey *key2 = (TKey*)gDirectory->GetListOfKeys()->FindObject(readObj->GetName());
	  if(key2) {
	    TH1* h2 = (TH1*)key2->ReadObj();
     /*------------Data--------------*/
	    if(i == 0)  datahist->Add(h2);
	    else if(i == 1) {
     /*------------background--------------*/
	      error->Add(h2);
	      for(int j = 1; j < h2->GetXaxis()->GetNbins()+1; j++) {
		h2->SetBinError(j, 0);
	      }
	      //////style
	      string title = nextfile->GetTitle();
	      title = title.substr(0, title.size()-5);
	      h2->SetTitle(title.c_str());
	      h2->SetLineColor(color[nfile]);
	      h2->SetFillStyle(1001);
	      h2->SetFillColor(color[nfile]);

	      hs->Add(h2);
	      nfile++;
	    } else if(i == 2) {
     /*------------Signal--------------*/
	      for(int j = 1; j < h2->GetXaxis()->GetNbins()+1; j++) {
		h2->SetBinError(j, 0);
	      }

	      //////style
	      string title = nextfile->GetTitle();
	      title = title.substr(0, title.size()-5);
	      h2->SetTitle(title.c_str());
	      h2->SetLineColor(color[nfile]);
	      h2->SetLineWidth(3);
	      h2->SetLineStyle(2);

	      sigHists->Add(h2);
	      nfile++;
	    }

	  }
	  nextfile = (TFile*)FileList[i]->After(nextfile);
	}
      }

      /*--------------write out------------*/

      datahist->SetMarkerStyle(20);
      datahist->SetLineColor(1);

      /// sort based on integral.  Change this function is want other order
      hs = sortStack(hs);

      ///rebin
      /// default rebinning based on data error.  If no data, bin
      /// based on background error
      vector<double> bins;

      TH1D* fullHist = new TH1D("full", readObj->GetTitle(), readObj->GetXaxis()->GetNbins(), readObj->GetXaxis()->GetXmin(), readObj->GetXaxis()->GetXmax());
      fullHist->Add(error);
      if(!noData) fullHist->Add(datahist);
      TH1D* tmpsig = (TH1D*)sigHists->First();
      while(tmpsig) {
	fullHist->Add(tmpsig);
	tmpsig = (TH1D*)sigHists->After(tmpsig);
      }

      // if(noData ) bins = rebinner(error, styler.getRebinLimit());
      // else bins = rebinner(datahist, styler.getRebinLimit());
      if(explicitBins.find(readObj->GetTitle()) == explicitBins.end()) bins = rebinner(fullHist, styler.getRebinLimit());
      else {
	bins.push_back(readObj->GetXaxis()->GetXmin());
	double lastbin = readObj->GetXaxis()->GetXmax();
	double currentVal = bins.at(0);

	for(auto it: explicitBins[readObj->GetTitle()]) {
	  int numLeft = it.first;
	  double binWidth = it.second;
	  if(binWidth <= 0) {
	    if(numLeft <= 0) {
	      numLeft = 1;
	    }
	    binWidth = lastbin - currentVal/numLeft;
	  } else if(numLeft <= 0) {
	    numLeft = (int)((lastbin - currentVal)/binWidth);
	  }
	  while(numLeft > 0 && lastbin - currentVal > EPSILON_VALUE) {
	    currentVal += binWidth;
	    bins.push_back(currentVal);
	    numLeft--;
	  }
	}
	if(abs(currentVal-lastbin) > EPSILON_VALUE) bins.push_back(lastbin);

	reverse(bins.begin(), bins.end());

      }

      //// need to get rid of continue if possible because dirty deleting
      /// happening here.  Maybe put CreateStack in main and make the class
      /// the stuff that happens in the loop?  Then just make a destructor.
      /// that would be pretty.  huh
      if(bins.size() == 0) {
	hs->Delete();
	delete datahist;
	delete error;
	delete sigHists;
	continue;
      }

      /// Check if rebin vector is in decending order (sometimes didn't happen??)
      /// then puts into a double array in increasing order for Rebin function
      double* binner = new double[bins.size()+1];
      bool passed = true;

      binner[0] = bins.back();
      for(int i = 1; i < bins.size(); i++) {
      	if(bins.at(bins.size() - i) >= bins.at(bins.size() - i - 1))  {
      	  passed = false;
      	  break;
      	}
      	binner[i] = bins.at(bins.size() - i - 1);
      }


      ////rebin histograms
      /// make new stack because hs gets deleted in teh rebinstack function.  Maybe
      /// this isn't necessary.  A little jaring to make the change.  Also, I've put
      /// hs instead of hsdraw so many times...
      THStack* hsdraw = hs;
      if(styler.getDivideBins() && passed && bins.size() > styler.getBinLimit()) {
	      datahist = (TH1D*)datahist->Rebin(bins.size()-1, "data_rebin", binner);
      	error = (TH1D*)error->Rebin(bins.size()-1, "error_rebin", binner);
      	hsdraw = rebinStack(hs, binner, bins.size()-1);
      	TList* tmplist = new TList();
      	TH1D* onesig = (TH1D*)sigHists->First();
      	while(onesig) {
      	  tmplist->Add(onesig->Rebin(bins.size()-1, onesig->GetName(), binner));
      	  onesig = (TH1D*)sigHists->After(onesig);
      	}
	//if(do_overflow){
	  //int last_bin=datahist->GetNbinsX();
	  //datahist->SetBinContent(last_bin,datahist->GetBinContent(last_bin+1));
	  //datahist->SetBinError(last_bin,datahist->GetBinError(last_bin+1));

	  //error->SetBinContent(last_bin,datahist->GetBinError(last_bin+1));
	  //error->SetBinError(last_bin,lastbin_error_error);

	  //TList* list = (TList*)hsdraw->GetHists();
	  //TH1D* tmp = (TH1D*)list->First();
	  //int i=0;
	  //while ( tmp ) {
	    //tmp->SetBinContent(last_bin,lastbin_bg.at(i));
	    //tmp->SetBinError(last_bin,lastbin_bg_error.at(i));
	    //tmp = (TH1D*)list->After(tmp);
	    //i++;
	  //}

	  //tmp = (TH1D*)sigHists->First();
	  //while ( tmp ) {
	    //tmp->SetBinContent(last_bin,lastbin_sg.at(i));
	    //tmp->SetBinError(last_bin,lastbin_sg_error.at(i));
	    //tmp = (TH1D*)sigHists->After(tmp);
	  //}

	//}
      	delete sigHists;
      	sigHists = tmplist;
        divideBin(datahist, error, hsdraw, sigHists);
      }

      ///legend stuff
      TLegend* legend = createLeg(datahist, hsdraw->GetHists(), sigHists);

      ////divide by binwidth is option is given
      //if(styler.getDivideBins()) divideBin(datahist, error, hsdraw, sigHists);

      //error for top
      TGraphErrors* errorstack = createError(error, false);

      ////draw graph
      target->cd();

      TCanvas *c = new TCanvas(readObj->GetName(), readObj->GetName());//403,50,600,600);
      //// need to work on top text
      // TPaveText* text = new TPaveText(0.05, 0.7, 0.5, 1.);
      // text->AddText("CMS Preliminary");
      // text->Draw();

      if(!(onlyTop)) {
	c->Divide(1,2);
	c->cd(1);
	sizePad(styler.getPadRatio(), gPad, true);
      }

      hsdraw->Draw();
      datahist->Draw("e1same");

    TPaveText *pt = new TPaveText(0.80,0.941,0.95,1.0,"NBNDC");
    pt->AddText("35.92 fb^{-1} (13 TeV)");
    // pt->AddText("41.53 fb^{-1} (13 TeV)");
    // pt->AddText("59.74 fb^{-1} (13 TeV)");
    pt->SetTextFont(42);
    pt->SetTextAlign(32);
    pt->SetFillStyle(0);
    pt->SetBorderSize(0);
    pt->Draw();

    TPaveText *pt2 = new TPaveText(0.09,0.88,0.21,0.95,"NBNDC");
    pt2->AddText("CMS ");
    pt2->SetTextAlign(12);
    pt2->SetFillStyle(0);
    pt2->SetBorderSize(0);
    pt2->Draw();

    //TPaveText *pt3 = new TPaveText(0.09,0.82,0.21,0.88,"NBNDC");
    TPaveText *pt3 = new TPaveText(0.09,0.82,0.25,0.88,"NBNDC");
    //pt3->AddText("Preliminary");
    pt3->AddText("Work in progress");
    pt3->SetTextAlign(12);
    pt3->SetTextFont(52);
    pt3->SetFillStyle(0);
    pt3->SetBorderSize(0);
    pt3->Draw();




      tmpsig = (TH1D*)sigHists->First();
      while(tmpsig) {
	tmpsig->Draw("same");
	tmpsig = (TH1D*)sigHists->After(tmpsig);
      }
      errorstack->Draw("2");
      legend->Draw();
      setYAxisTop(datahist, error, styler.getHeightRatio(), hsdraw);
      if(noData) {
	hsdraw->GetXaxis()->SetTitle(newLabel(hsdraw->GetTitle()).c_str());
	hsdraw->GetXaxis()->SetTitleSize(hsdraw->GetYaxis()->GetLabelSize());
      }
      if(do_overflow){
        //latex.SetNDC();
        //latex.SetTextAngle(90);
        //latex.SetTextColor(kBlack);
        //latex.SetTextFont(43);
        //latex.SetTextAlign(31);
        //latex.SetTextSize(16);
        //latex.DrawLatex(0.97,0.3,"Overflow");
      }

      // ///second pad
      TF1* PrevFitTMP = NULL;
      TGraphErrors* errorratio = NULL;
      TList* signalBot = NULL;

      if( !onlyTop ) {
	c->cd(2);
	sizePad(styler.getPadRatio(), gPad, false);

	TH1* botaxis = error;
	botaxis->Draw("AXIS");
	setXAxisBot(botaxis, styler.getPadRatio());

	signalBot = (bottomType != Ratio) ? signalBottom(sigHists, error) : signalBottom(sigHists, datahist, error);

	errorratio = createError(error, true);
	if(bottomType == Ratio) {
	  tmpsig = (TH1D*)signalBot->Last();
	  PrevFitTMP = createLine(tmpsig);
	  setYAxisBot(error->GetYaxis(), tmpsig, styler.getPadRatio());
	} else setYAxisBot(botaxis->GetYaxis(), signalBot, styler.getPadRatio());

	tmpsig = (TH1D*)signalBot->First();
	while(tmpsig) {
	  tmpsig->Draw("same");
	  tmpsig = (TH1D*)signalBot->After(tmpsig);
	}
	if(bottomType == Ratio) errorratio->Draw("2");
      }

      c->cd();
      c->Write(c->GetName());
      c->Close();

      /// so many delete.  Probably not doing this right, but this program is so small
      /// memory leaks basically don't matter.
      /// delete vs Delete() still up in the air.  delete doesn't delete objects in container
      /// while Delete() does, but this only is true sometimes.  idk
      // All deletes commented out by Brenda FE, Aug 7, 2020.
      // hsdraw->Delete();
      // delete datahist;
      // delete error;
      // delete sigHists;
      // delete legend;
      // delete errorstack;

      // delete[] binner;
      //if( !onlyTop ) {
	//delete errorratio;
	//delete PrevFitTMP;
	//signalBot->Delete();
      //}

    } else if ( obj->IsA()->InheritsFrom( TDirectory::Class() ) ) {

      target->cd();
      TDirectory *newdir = target->mkdir( obj->GetName(), obj->GetTitle() );

      CreateStack( newdir, logfile );

    } else if ( obj->IsA()->InheritsFrom( TH1::Class() ) ) {

      continue;

    } else {
         cout << "Unknown object type, name: "
	   << obj->GetName() << " title: " << obj->GetTitle() << endl;
    }
  }

  TH1::AddDirectory(status);
}



template <typename T>
string to_string_with_precision(const T a_value, const int n)
{
  ostringstream out;
  out << fixed << setprecision(n) << a_value;
  return out.str();
}



///// Function takes an old THStack and sorts the stack based on
//// integral of the graph, smallest to largest.
/// deletes old stack so pointer nonsense doesn't happen
THStack* Plotter::sortStack(THStack* old) {
  if(old == NULL || old->GetNhists() == 0) return old;
  string name = old->GetName();
  THStack* newstack = new THStack(name.c_str(),name.c_str());

  TList* list = (TList*)old->GetHists();

  while(list->GetSize() > 1) {

    TH1* smallest = (TH1*)list->First();;
    TH1* tmp = (TH1*)list->After(smallest);
    while ( tmp ) {
      if(smallest->Integral() > tmp->Integral()) smallest = tmp;
      tmp = (TH1*)list->After(tmp);
    }
    newstack->Add(smallest);
    list->Remove(smallest);
  }
  TH1* last = (TH1*)list->First();
  if(last) newstack->Add(last);

  delete old;
  return newstack;
}


////make legend position adjustable
//// Puts all of the histograms in the legend.  Set so backgrounds show up
/// as colored blocks, but signal and data as T's in the format they are on the graph
TLegend* Plotter::createLeg(const TH1* data, const TList* bgl, const TList* sigl) {
  double width = 0.05;
  int items = bgl->GetSize() + sigl->GetSize();
  items += (data->GetEntries() != 0) ? 1 : 0;
//TLegend* leg = new TLegend(0.73, 0.9-items*width ,0.93,0.90);
  TLegend* leg = new TLegend(0.79, 0.92-(items+1)*width ,0.94,0.92);

  if(data->GetEntries() != 0) leg->AddEntry(data, "Data", "lep");
  TH1* tmp = (TH1*)bgl->First();
  while(tmp) {
    leg->AddEntry(tmp, tmp->GetTitle(), "f");
    tmp = (TH1*)bgl->After(tmp);
  }
  tmp = (TH1*)sigl->First();
  while(tmp) {
    leg->AddEntry(tmp, tmp->GetTitle(), "lep");
    tmp = (TH1*)sigl->After(tmp);
  }


   TH1* mcErrorleg = new TH1I("mcErrorleg", "BG stat. uncer.", 100, 0.0, 4.0);
    mcErrorleg->SetLineWidth(1);
    mcErrorleg->SetFillColor(kMagenta+1);
    mcErrorleg->SetFillStyle(3004);
  mcErrorleg->SetLineColor(kMagenta+1);
 leg->AddEntry(mcErrorleg,"BG stat. uncer.","f");
 leg->SetFillStyle(0);

// leg->AddEntry((TObject*)0,"BG stat. uncer.","f");
  return leg;
}

//// Creates the TGraphError that is plotted in the stack and ratio plots
/// tricky function because it takes a bool to ask if you want the ratio
/// plots error or the stack plots error.  (True for ratio, false for stack)
/// make more clean maybe?  Maybe?
TGraphErrors* Plotter::createError(const TH1* error, bool ratio) {
  int Nbins =  error->GetXaxis()->GetNbins();
  Double_t* mcX = new Double_t[Nbins];
  Double_t* mcY = new Double_t[Nbins];
  Double_t* mcErrorX = new Double_t[Nbins];
  Double_t* mcErrorY = new Double_t[Nbins];

  for(int bin=0; bin < error->GetXaxis()->GetNbins(); bin++) {
    mcY[bin] = (ratio) ? 1.0 : error->GetBinContent(bin+1);
    mcErrorY[bin] = (ratio) ?  error->GetBinError(bin+1)/error->GetBinContent(bin+1) : error->GetBinError(bin+1);
    mcX[bin] = error->GetBinCenter(bin+1);
    mcErrorX[bin] = error->GetBinWidth(bin+1) * 0.5;
  }
  TGraphErrors *mcError = new TGraphErrors(error->GetXaxis()->GetNbins(),mcX,mcY,mcErrorX,mcErrorY);

  mcError->SetLineWidth(1);
  mcError->SetFillColor(kMagenta+1);
  mcError->SetFillStyle(3004);

  delete[] mcX;
  delete[] mcY;
  delete[] mcErrorX;
  delete[] mcErrorY;
  return mcError;
}

/// Resize pad in question (based on the bool values) to the ratio set in the style config
// file, namely the value PadRatio which is TopHeight/BottomHeight.
/// If more space is needed, use the Margin values in the style config file
void Plotter::sizePad(double ratio, TVirtualPad* pad, bool isTop) {
  if(isTop) pad->SetPad("top", "top", 0, 1 / (1.0 + ratio), 1, 1, 0);
  else  {
    pad->SetPad("bottom", "bottom", 0, 0, 1, 1 / (1.0 + ratio), 0);
    pad->SetMargin(pad->GetLeftMargin(),pad->GetRightMargin(),ratio*pad->GetBottomMargin(),0);
    pad->SetTitle("");
  }
}

//// make line that is in the ratio plot at the value 1.  Most is beautification
TF1* Plotter::createLine(TH1* data_mc) {
  TF1 *PrevFitTMP = new TF1("PrevFitTMP","pol0",-10000,10000);
  PrevFitTMP->SetMarkerStyle(20);
  PrevFitTMP->SetLineColor(2);
  PrevFitTMP->SetLineWidth(1);
  PrevFitTMP->SetParameter(0,1.0);
  PrevFitTMP->SetParError(0,0);
  PrevFitTMP->SetParLimits(0,0,0);
  data_mc->GetListOfFunctions()->Add(PrevFitTMP);
  return PrevFitTMP;
}

//// Monster function here.  Takes a histogram and starts from the left of it
/// adds up the error in it and finds the ratio of the error to the value.  If
// this ratio is less than limit value, it takes that bin.  If not, it will move
/// to the left and add the next bin till the ratio value is less than the limit

///some sort of witchcraft is going on here, may need to check and fix.  The limit
/// value is not very representative for instance
vector<double> Plotter::rebinner(const TH1* hist, double limit) {
  vector<double> bins;
  double toterror = 0.0, prevbin=0.0;
  double limit2 = pow(limit,2);
  bool foundfirst = false;
  double end;

  //how to tell if ok?

  if(hist->GetEntries() == 0 || hist->Integral() <= 0) return bins;

  for(int i = hist->GetXaxis()->GetNbins(); i > 0; i--) {
    if(hist->GetBinContent(i) <= 0.0) continue;
    if(!foundfirst) {
      bins.push_back(hist->GetXaxis()->GetBinUpEdge(i));
      foundfirst = true;
    } else end = hist->GetXaxis()->GetBinLowEdge(i);

    if(toterror* prevbin != 0.) toterror *= pow(prevbin,2)/pow(prevbin+hist->GetBinContent(i),2);

    prevbin += hist->GetBinContent(i);
    toterror += (2 * pow(hist->GetBinError(i),2))/pow(prevbin,2);

    if(toterror < limit2) {
      bins.push_back(hist->GetXaxis()->GetBinLowEdge(i));
      toterror = 0.0;
      prevbin = 0.0;
    }
  }

   ////// makes sure the first bin of the graph is included
   //// probalby error here (that's why I do a check in the Checkstack
   /// function, gulp
  if(bins.back() != end) {
    bins.push_back(end);
    if(hist->GetXaxis()->GetXmin() >= 0 && end !=hist->GetXaxis()->GetXmin()) bins.push_back(hist->GetXaxis()->GetXmin());
  }

  return bins;

}


///// The stack has a list inherently in it, so I made a function to
/// get the thstacks list and rebin it.  Not necessary, but eh.  Need to
/// fix delete stuff to stop hs and hsdraw nonsense
THStack* Plotter::rebinStack(THStack* hs, const double* binner, int total) {
  THStack* newstack = new THStack(hs->GetName(), hs->GetName());
  TList* list = (TList*)hs->GetHists();

  TH1D* tmp = (TH1D*)list->First();
  while ( tmp ) {
    TH1D* forstack = (TH1D*)tmp->Clone();
    forstack = (TH1D*)forstack->Rebin(total, tmp->GetName(), binner);
    newstack->Add(forstack);
    tmp = (TH1D*)list->After(tmp);
  }

  hs->Delete();

  return newstack;
}


//// Function creates the significance plot on the bottom.
TList* Plotter::signalBottom(const TList* signal, const TH1D* background) {
  TList* returnList = new TList();

  TH1D* holder = (TH1D*)signal->First();

  while(holder) {
    TH1D* signif = (TH1D*)holder->Clone();
    int Nbins = signif->GetXaxis()->GetNbins();
    for(int i = 0; i < Nbins;i++) {
      if(signif->GetBinContent(i+1) <= 0 && background->GetBinContent(i+1) <= 0) continue;
      int edge1 = i+1, edge2= i+1;
      double sigErr, backErr;
      if(bottomType == SigLeft) edge1 = 0;
      if(bottomType == SigRight) edge2 = Nbins;
      double sigInt = signif->IntegralAndError(edge1, edge2, sigErr);
      double backInt = background->IntegralAndError(edge1, edge2, backErr);

      double total = (ssqrtsb) ? sigInt/sqrt(sigInt+backInt) : sigInt/sqrt(backInt);
      double perErr = (ssqrtsb) ? pow(sigErr/sigInt-sigErr/(2*(sigInt+backInt)),2) + pow(backErr/(2*(sigInt+backInt)),2) : pow(sigErr/sigInt,2) + pow(backErr/(2*backInt),2);

      signif->SetBinContent(i+1, total);
      signif->SetBinError(i+1, total*perErr);
    }
    returnList->Add(signif);
    holder = (TH1D*)signal->After(holder);
  }
  holder = NULL;
  return returnList;
}


//// Function creates the Ratio Plot on the bottom
TList* Plotter::signalBottom(const TList* signal, const TH1D* data, const TH1D* background) {
  TList* returnList = new TList();

  TH1D* holder = (TH1D*)signal->First();

  while(holder) {
    TH1D* total = (TH1D*)holder->Clone();
    total->Add(background);

    total->Divide(data, total);

    returnList->Add(total);
    holder = (TH1D*)signal->After(holder);
  }
  TH1D* data_mc = (TH1D*)data->Clone();
  data_mc->Divide(background);
  returnList->Add(data_mc);

  holder = NULL;
  return returnList;
}

////This block as functions to set up the axes in the graph

// Sets the graph so the maximum is seen (else if data is too tall, might not show up)
/// Need to add in signal stuff to this as well



void Plotter::setYAxisTop(TH1* datahist, TH1* error, double ratio, THStack* hs) {
  TAxis* yaxis = hs->GetYaxis();
  std::string y_title_name, x_axis_name, t;

  if(newLabel(hs->GetTitle()).find("GeV") != std::string::npos) {
    x_axis_name = "GeV";
  }

  else {
    x_axis_name = "";
  }
  t = x_axis_name.c_str();

  //when Divide bin option and fixed bin size option is given
  if(styler.getDivideBins() && styler.getRebinLimit() >1.0) {
    double bin_width = datahist->GetBinWidth(1);
    double nearest= round(bin_width * 100) / 100;
    std::string s = std::to_string(nearest);
    std::string temp;
    std::size_t loc = s.find('.');

    s.erase (loc+3,4);

    if (s[s.length()-1] == '0' && s[s.length()-2] == '0'){
      s.erase(loc,3);
      temp=s.c_str();

      y_title_name = "Events/"+temp+" "+t;
    }
    else {
      y_title_name = "Events/"+s+" "+t;
    }
    yaxis->SetTitle(y_title_name.c_str());
    s.clear();
  }
   //when Divide bin option and auto bin size option is given
  else if(styler.getDivideBins() && styler.getRebinLimit() < 1.0) {
    if(t=="GeV") {
      y_title_name = "Events/"+t;
    }


    else {
      y_title_name = "Events";
    }

    yaxis->SetTitle(y_title_name.c_str());
  }

  else {
    yaxis->SetTitle("Events");}
  ///  yaxis->SetLabelSize(hs->GetXaxis()->GetLabelSize());
  double max = (error->GetMaximum() > datahist->GetMaximum()) ? error->GetMaximum() : datahist->GetMaximum();

   hs->SetMaximum(max*(1.0/ratio + 1.0));

}

//// just set label
void Plotter::setXAxisBot(TH1* data_mc, double ratio) {
  TAxis* xaxis = data_mc->GetXaxis();
  xaxis->SetTitle(newLabel(data_mc->GetTitle()).c_str());
  xaxis->SetLabelSize(xaxis->GetLabelSize()*ratio);
}


//// If it is a ratio plot, use a lot of magic to find a good
//// window for the ratio plot.
void Plotter::setYAxisBot(TAxis* yaxis, TH1* data_mc, double ratio) {
  double divmin = 0.0, divmax = 2.99;
  double low=2.99, high=0.0, tmpval;
  for(int i = 0; i < data_mc->GetXaxis()->GetNbins(); i++) {
    tmpval = data_mc->GetBinContent(i+1);
    if(tmpval < 2.99 && tmpval > high) {high = tmpval;}
    if(tmpval > 0. && tmpval < low) {low = tmpval;}
  }
  double val = min(abs(1 / (high - 1.)), abs(1 / (1/low -1.)));
  if(high == 0.0) val = 0;
  double factor = 2.0;
  while(val > factor || (factor == 2 && val > 1.)) {
    divmin = 1.0 - 1.0/factor;
    divmax = 1/divmin;
    factor *= 2.0;
  }
  yaxis->SetRangeUser(divmin - 0.00001,divmax - 0.00001);
  yaxis->SetLabelSize(yaxis->GetLabelSize()*ratio);
  yaxis->SetTitleSize(ratio*yaxis->GetTitleSize());
  yaxis->SetTitleOffset(yaxis->GetTitleOffset()/ratio);
  yaxis->SetTitle("#frac{Data}{MC}");
}

//// if plot is a significance plot, finds the maximum and
/// changes the range to fit all of the graphs
void Plotter::setYAxisBot(TAxis* yaxis, TList* signal, double ratio) {
  double max = 0;
  TH1D* tmphist = (TH1D*)signal->First();
  while(tmphist) {
    max = (max < tmphist->GetMaximum()) ? tmphist->GetMaximum() : max;
    tmphist = (TH1D*)signal->After(tmphist);
  }

  yaxis->SetRangeUser(0,max*11./10 -0.00001);
  yaxis->SetLabelSize(yaxis->GetLabelSize()*ratio);
  yaxis->SetTitleSize(ratio*yaxis->GetTitleSize());
  yaxis->SetTitleOffset(yaxis->GetTitleOffset()/ratio);
  if(ssqrtsb) yaxis->SetTitle("#frac{S}{#sqrt{S+B}}");
  else  yaxis->SetTitle("#frac{S}{#sqrt{B}}");
}


//// divdes all of the histogram by their width if told to by the style config file
/// namely with the value DivideBins
void Plotter::divideBin(TH1* data, TH1* error, THStack* hs, TList* signal) {
  //bu asagidaki kismi ben comment  ettim

  for(int i = 0; i < data->GetXaxis()->GetNbins(); i++) {
    data->SetBinContent(i+1, data->GetBinContent(i+1)/data->GetBinWidth(i+1));
    data->SetBinError(i+1, data->GetBinError(i+1)/data->GetBinWidth(i+1));
  }
  for(int i = 0; i < error->GetXaxis()->GetNbins(); i++) {
    error->SetBinContent(i+1, error->GetBinContent(i+1)/error->GetBinWidth(i+1));
    error->SetBinError(i+1, error->GetBinError(i+1)/error->GetBinWidth(i+1));
  //bu yukaridaki kismi ben comment  ettim
//burdan itiabren ben ekledim
  //buraya kadar ben ekledim yukaridaki kismi comment ettiktan sonra.
  // for(int i = 0; i < data->GetXaxis()->GetNbins(); i++) {
  //   data->SetBinContent(i+1, data->GetBinContent(i+1));
  //   data->SetBinError(i+1, data->GetBinError(i+1));
  // }
  // for(int i = 0; i < error->GetXaxis()->GetNbins(); i++) {
  //   error->SetBinContent(i+1, error->GetBinContent(i+1));
  //   error->SetBinError(i+1, error->GetBinError(i+1));
   }

  TList* list = (TList*)hs->GetHists();

  TIter next(list);
  TH1* tmp = NULL;
  while ( (tmp = (TH1*)next()) ) {
    for(int i = 0; i < tmp->GetXaxis()->GetNbins(); i++) {
      tmp->SetBinContent(i+1,tmp->GetBinContent(i+1)/tmp->GetBinWidth(i+1));
      //tmp->SetBinContent(i+1,tmp->GetBinContent(i+1));
      //tmp->SetBinError(i+1,tmp->GetBinError(i+1));
      tmp->SetBinError(i+1,tmp->GetBinError(i+1)/tmp->GetBinWidth(i+1));
    }
  }
  tmp = (TH1*)signal->First();
  while(tmp) {
    for(int i = 0; i < tmp->GetXaxis()->GetNbins(); i++) {
      tmp->SetBinContent(i+1,tmp->GetBinContent(i+1)/tmp->GetBinWidth(i+1));
      tmp->SetBinError(i+1,tmp->GetBinError(i+1)/tmp->GetBinWidth(i+1));
    }
    tmp = (TH1*)signal->After(tmp);
  }

}


//// get the number of files total in the plotter
int Plotter::getSize() {
  return FileList[0]->GetSize() + FileList[1]->GetSize() + FileList[2]->GetSize();
}


//// Get the filenames (have options for different lists).
/// puts the names into a vector.  Mainly used for the logfile class
vector<string> Plotter::getFilenames(string option) {
  vector<string> filenames;
  TFile* tmp = NULL;
  TList* worklist = NULL;
  if(option == "data") worklist = FileList[0];
  else if(option == "background") worklist = FileList[1];
  else if(option == "signal") worklist = FileList[2];
  else if(option == "all") {
    for(int i = 0; i < 3; i++) {
      tmp = (TFile*)FileList[i]->First();
      while(tmp) {
	filenames.push_back(tmp->GetTitle());
	tmp = (TFile*)FileList[i]->After(tmp);
      }
    }
    return filenames;

  } else {
    cout << "Error in filename option, returning empty vector" << endl;
    return filenames;
  }
  tmp = (TFile*)worklist->First();
  while(tmp) {
    filenames.push_back(tmp->GetTitle());
    tmp = (TFile*)worklist->After(tmp);
  }
  worklist = NULL;
  return filenames;

}

//// set style given to the style of the plotter class
void Plotter::setStyle(Style& style) {
  this->styler = style;
}


/// Add Normalized file to plotter
void Plotter::addFile(Normer& norm) {
  string filename = norm.output;
  if(norm.use == 0) {
    cout << filename << ": Not all files found" << endl << endl;
    return;
  }

  while(filename.find("#") != string::npos) {
    filename.erase(filename.find("#"), 1);
  }

  TFile* normedFile = NULL;
  if(norm.use == 1) {
    norm.print();
    norm.FileList = new TList();
    for(vector<string>::iterator name = norm.input.begin(); name != norm.input.end(); ++name) {
      norm.FileList->Add(TFile::Open(name->c_str()));
    }

    normedFile = new TFile(filename.c_str(), "RECREATE");
    norm.MergeRootfile(normedFile);
  } else if(norm.use == 2) {
    cout << filename << " is already Normalized" << endl << endl;;
    normedFile = new TFile(filename.c_str());
  }

  normedFile->SetTitle(norm.output.c_str());

  if(norm.type == "data") FileList[0]->Add(normedFile);
  else if(norm.type == "bg") FileList[1]->Add(normedFile);
  else if(norm.type == "sig") FileList[2]->Add(normedFile);


}

void Plotter::getPresetBinning(string filename) {

  ifstream info_file(filename);

  if(!info_file) {
    std::cout << "could not open file " << filename <<std::endl;
    exit(1);
  }

  string line;
  while(getline(info_file, line)) {
    string name;
    vector<string> tmpVals;
    vector<pair<int, double>> tmpPairs;
    string current = "";
    int bracNum = 0;
    for(auto it: line) {
      if(it == '[' || it == ']' || it == ',') {
        if(current != "") {
          if(bracNum == 0) name = current;
          else if(bracNum > 0) tmpVals.push_back(current);
          current = "";
        }
        if(it == '[' && ++bracNum > 2) {
          cout << "Error: Current code only allows for 2 levels of brackets.  Review line:" << endl;
          cout << line << endl;
          exit(1);
        } else if(it == ']') {
          if(--bracNum < 0 || (tmpVals.size() != 2 && tmpVals.size() != 0)) {
            cout << "Error: Closing Bracket without corresponding open bracket: Review line:" << endl;
            cout << line << endl;
            exit(1);
          }
          if(tmpVals.size() == 2) tmpPairs.push_back(make_pair(stoi(tmpVals.at(0)), stod(tmpVals.at(1))));
          tmpVals.clear();
        }
      } else if(it == ' ' || it == '\t') continue;
      else current.push_back(it);
    }

    if(bracNum != 0) {
      cout << "Error: Not all Brackets were terminated.  Please review the line:" << endl;
      cout << line << endl;
      exit(1);
    } // else if(e1.size()*e2.size() != 0) {
    //   cout << "Error: Input requires all inputs to be enclosed by brackets. Review line:" << endl;
    //   cout << line << endl;
    //   exit(1);
    // }
    if(explicitBins.find(name) != explicitBins.end()) {
      cout << "Duplicate histogram:" << endl;
      cout << name << endl;
      exit(1);
    }
    explicitBins[name] = tmpPairs;
  }
  info_file.close();

}




//// A lot of regex stuff to extract the x axis label from the title
// of the graph.  If the xaxis label is wrong, change the code here.
string Plotter::newLabel(string stringkey) {
  string particle = "";

  smatch m;
  regex part ("(Di)?(Tau(Jet)?|Muon|Electron|Jet)");

  regex e ("^(.+?)(1|2)?Energy$");
  regex n ("^N(.+)$");
  regex charge ("^(.+?)(1|2)?Charge");
  regex mass ("(.+?)(Not)?Mass");
  regex zeta ("(.+?)(P)?Zeta(1D|Vis)?");
  regex deltar ("(.+?)DeltaR");
  regex MetMt ("^(([^_]*?)(1|2)|[^_]+_(.+?)(1|2))MetMt$");
  regex eta ("^(.+?)(Delta)?(Eta)");
  regex phi ("^(([^_]*?)|[^_]+_(.+?))(Delta)?(Phi)");
  regex cosdphi ("(.+?)(CosDphi)(.*)");
  regex pt ("^(.+?)(Delta)?(Pt)(Div)?.*$");
  regex osls ("^(.+?)OSLS");
  regex zdecay ("^[^_]_(.+)IsZdecay$");


  if(regex_match(stringkey, m, e)) {
    return "E(" + turnLatex(m[1].str()) + ") [GeV]";
  }
  else if(regex_match(stringkey, m, n)) {
    return "N(" + turnLatex(m[1].str()) + ")";
  }
  else if(regex_match(stringkey, m, charge)) {
    return  "charge(" + turnLatex(m[1].str())+ ") [e]";
  }
  else if(regex_match(stringkey, m, mass)) {
    return ((m[2].str() == "Not") ? "Not Reconstructed M(": "M(") + listParticles(m[1].str()) + ") [GeV]";
  }
  else if(regex_match(stringkey, m, zeta)) {
    return m[2].str() + "#zeta" +((m[3].str() != "") ? "_{" + m[3].str() + "}":"") + "(" + listParticles(m[1].str()) + ")";
  }
  else if(regex_match(stringkey, m, deltar)) {
    return "#DeltaR("+ listParticles(m[1].str()) + ")";
  }
  else if(regex_match(stringkey, m, MetMt)) {
    return "M_{T}(" + turnLatex(m[2].str()+m[4].str()) + ") [GeV]";
  }
  else if(regex_match(stringkey, m, eta))  {
    particle += (m[2].str() != "") ? "#Delta" : "";
    particle += "#eta(" + listParticles(m[1].str()) + ")";
    return particle;
  }
  else if(regex_match(stringkey, m, phi))  {
    if(m[4].str() != "") particle = "#Delta";
    particle += "#phi(" + listParticles(m[2].str()+m[3].str()) + ")";
    return particle;
  }
  else if(regex_match(stringkey, m, cosdphi)) {
    return "cos(#Delta#phi(" + listParticles(m[1].str()) + "))";
  }
  else if(regex_match(stringkey, m, pt)) {
    if(m[4].str() != "") particle += "#frac{#Delta p_{T}}{#Sigma p_{T}}(";
    else if(m[2] != "") particle += "#Delta p_{T}(";
    else particle += "p_{T}(";
    particle += listParticles(m[1].str()) + ") [GeV]";
    return particle;
  }
  else if(stringkey.find("Met") != string::npos) return "#slash{E}_{T} [GeV]";
  else if(stringkey.find("MHT") != string::npos) return "#slash{H}_{T} [GeV]";
  else if(stringkey.find("HT") != string::npos) return "H_{T} [GeV]";
  else if(stringkey.find("Meff") != string::npos) return "M_{eff} [GeV]";
  else if(regex_match(stringkey, m, osls)) {
    particle = m[1].str();
    string full = "";
    while(regex_search(particle, m, part)) {
      full += "q_{" + turnLatex(m[0].str()) + "} ";
      particle = m.suffix().str();
    }
    return full;
  } else if(regex_match(stringkey, m, zdecay)) {
    return listParticles(m[1].str()) + "is Z Decay";
  }

  return stringkey;

}

////// helper function for newLabel.  Takes a section of the string
/// and extracts the particles from it, then turns them into latex format.
/// seperated by comma
//// More robust would be vector<string> so could use different formatting
string Plotter::listParticles(string toParse) {
  smatch m;
  regex part ("(Di)?(Tau(Jet)?|Muon|Electron|Jet|Met)");
  bool first = true;
  string final = "";

  while(regex_search(toParse,m,part)) {
    if(first) first = false;
    else final += ", ";
    final += turnLatex(m[0].str());
    toParse = m.suffix().str();
  }
  return final;
}
