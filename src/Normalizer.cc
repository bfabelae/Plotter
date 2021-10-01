#include "Normalizer.h"

using namespace std;

Normer::Normer() {
  // Added by Brenda FE, Tue Aug 6th, 2019, 1:27PM
  TH1::SetDefaultSumw2(true);
}

Normer::Normer(vector<string> values) {
  // Added by Brenda FE, Tue Aug 6th, 2019
  TH1::SetDefaultSumw2(true);
  setValues(values);
  output = values[1];
  if(values.size() == 2) {
     isData = true;
     type = "data";
  } else if(values.size() == 4){ 
    isDataDriven = true;
    type = values[3];
  } else if(values.size() >= 5) type = values[4];

}


Normer::Normer(const Normer& other) :
  output(other.output), type(other.type), lumi(other.lumi), use(other.use)
{
  // Added by Brenda FE, Tue Aug 6th, 2019, 1:27PM
  TH1::SetDefaultSumw2(true);
  input = other.input;
  skim = other.skim;
  xsec = other.xsec;
  normFactor = other.normFactor;
  isData = other.isData;
  FileList = new TList();

  
  TObject* object = other.FileList->First();
  while(object) {
    FileList->Add(new TObject(*object));
    object = FileList->After(object);  
  }
}

Normer& Normer::operator=(const Normer& rhs) {

}

Normer::~Normer() {
  TFile* source = (TFile*)FileList->First();
  while(source) {
    source->Close();
    source = (TFile*)FileList->After(source);  
  }
}


/// Set values given by the vector.  Used to declutter
/// the adding of files when read in from the config file
void Normer::setValues(vector<string> values) {
  input.push_back(values[0]);
  string mod_outfile = values[1];
  while(mod_outfile.find("#") != string::npos) {
    mod_outfile.erase(mod_outfile.find("#"), 1);
  }


  use = min(shouldAdd(values[0], mod_outfile),use);
  normFactor.push_back(1.);

  if(values.size() == 6) {
    SF.push_back(stod(values[5]));
  } else  SF.push_back(1.);

  if(values.size() == 2) {
    xsec.push_back(1.0);
    skim.push_back(1.0);
  } else if(values.size() == 4){
    xsec.push_back(1.0);
    skim.push_back(stod(values[2]));
  } else{
    xsec.push_back(stod(values[2]));
    skim.push_back(stod(values[3]));
  }
}

//// Has several return cases
//// 2: output file already exists and is as new as the input files
////    meaning remaking it is a waste of time
//// 1: Output file doesn't exist, so will make it OR the input file
////    is newer than the output file (needs to be remade)
//// 0: Input file doesn't exist, there is an error!

///// Once done for all files that need to added together, take the minimum
/// of this number to find out if the file needs to be made, namely, if one
/// value is 0, theres an error and abort.  If one file is newer than the 
/// output file (1), readd them.  If all are older than the output file, no need
/// to remake it (all 2)
int Normer::shouldAdd(string infile, string globalFile) {
  struct stat buffer;
  if(stat(infile.c_str(), &buffer) != 0) return 0;
  else if(stat(globalFile.c_str(), &buffer) != 0) return 1;
  else if(getModTime(globalFile.c_str()) > getModTime(infile.c_str())) return 2;
  else return 1;

}

void Normer::setLumi(double lumi) {
  this->lumi = lumi;
}


//// Helper function for shouldAdd (finds mod time of file)
int Normer::getModTime(const char *path) {
  struct stat attr;
  stat(path, &attr);
  char date[100] = {0};
  strftime(date, 100, "%s", localtime(&attr.st_mtime));
  return atoi(date);

}


//// prints out info about input files
void Normer::print() {
  cout << " =========== " << output << " =========== " << endl;
  for(int i = 0; i < input.size(); ++i) {
    cout << input.at(i) << endl;
  }
  cout << endl;
}

double Normer::getBayesError(double pass, double full) {
  // BrendaFE
  /*
  cout << "Normer::getBayesError at Normalizer.cc " << endl;
  cout << "pass = " << pass << endl;
  cout << "full = " << full << endl;
  if (pass > full) cout << "there are more events that pass than available, return 0" << endl;
  cout << "-----------------------" << endl;
  */
  if(pass > full) return 0;   // ORIGINAL
  //if(pass > full) pass = full;
  double effer = pass/full;
  //cout << "effer = " << effer << endl;
  double effer_err = sqrt(effer*(1-effer)/full);
  /*
  cout << "effer_err = " << effer_err << endl;
  cout << "effer + effer_err = " << effer + effer_err << endl;
  cout << "effer - effer_err = " << effer - effer_err << endl;
  cout << "effer_err*full = " << effer_err*full << endl;
  */
  if( effer + effer_err < 1 && effer - effer_err > 0) {
    //cout << "first method " << endl;
    return effer_err*full;
  } 
  TH1D* first = new TH1D("first", "first", 1, 0, 1);
  TH1D* second = new TH1D("second", "second", 1, 0, 1);
  first->SetBinContent(1, pass);
  second->SetBinContent(1, full);
  TGraphAsymmErrors* eff = new TGraphAsymmErrors(first, second, "b(1,1) mode");
  effer_err = eff->GetErrorYhigh(0);
  /*
  cout << "effer_err (TGraphAsymmErrors) = " << effer_err << endl;
  cout << "effer + effer_err = " << effer + effer_err << endl;
  cout << "effer - effer_err = " << effer - effer_err << endl;
  */
  if( effer + effer_err < 1 && effer - effer_err > 0) {
    //cout << "second method " << endl;
    return eff->GetErrorYhigh(0)*full;
  } else {
    //cout << "third method " << endl;
    return pass;
  }
  return 0;
  //cout << "The End." << endl;
}



///// Ripped hadd function.  Adds all the histograms together 
/// while normalizing them
void Normer::MergeRootfile( TDirectory *target) {

  TList* sourcelist = FileList;
  TString path( (char*)strstr( target->GetPath(), ":" ) );
  path.Remove( 0, 2 );

  TFile *first_source = (TFile*)sourcelist->First();
  first_source->cd( path );
  TDirectory *current_sourcedir = gDirectory;
  //gain time, do not add the objects in the list in memory
  Bool_t status = TH1::AddDirectoryStatus();
  TH1::AddDirectory(kFALSE);


  ///try to find events to calculate efficiency
  TH1D* events;
  current_sourcedir->GetObject("Events", events);

  if(events) {
    TH1D* efficiency=new TH1D("eff", "eff", 1, 0, 1);

    first_source->cd( path );
    int nplot = 0;
    normFactor.at(nplot) = 1.0/events->GetBinContent(1);
    TFile *nextsource = (TFile*)sourcelist->After( first_source );
    while( nextsource) {
      nplot++;
      nextsource->cd(path);
      gDirectory->GetObject("Events", events);
      normFactor.at(nplot) = 1.0/events->GetBinContent(1);
      nextsource = (TFile*)sourcelist->After( nextsource );
    }
  }
  delete events;

  // loop over all keys in this directory
  TChain *globChain = 0;
  TIter nextkey( current_sourcedir->GetListOfKeys() );
  TKey *key, *oldkey=0;
  while ( (key = (TKey*)nextkey())) {
    //keep only the highest cycle number for each key
    if (oldkey && !strcmp(oldkey->GetName(),key->GetName())) continue;

    first_source->cd( path );

    TObject *obj = key->ReadObj();
    if ( obj->IsA()->InheritsFrom( TH1::Class() ) ) {
      TH1 *h1 = (TH1*)obj;
      // Brenda: Fri Jun 14th, 3:24 pm - comment the line below
      //h1->Sumw2();

      int spot = 0;
      double scale1 = (isData || xsec.at(spot) < 0) ? 1.0 : normFactor.at(spot) * xsec.at(spot)* lumi * skim.at(spot);
      if(isDataDriven){ 
        // std::cout << "Found a data-driven background sample" << std::endl;
        h1->Sumw2();
        scale1 = skim.at(spot); // Since it is data-driven, there is no xsec nor luminosity to normalize to, so we factor them out.
      }

      scale1 *= SF.at(spot);

      if(strcmp(h1->GetTitle(),"Events") == 0) {
      	h1->SetBinError(2,getBayesError(h1->GetBinContent(2), h1->GetBinContent(1)));
      } else {
      	for(int i = 1; i <= h1->GetXaxis()->GetNbins(); i++) {
      	  if(h1->GetBinError(i) != h1->GetBinError(i) || h1->GetBinError(i) > h1->GetBinContent(i)) {
      	    h1->SetBinError(i, abs(h1->GetBinContent(i)));
      	  }
      	}
      }
	
      if(!isData) h1->Scale(scale1);

      TFile *nextsource = (TFile*)sourcelist->After( first_source );
      
      while ( nextsource ) {
      	spot++;
      	nextsource->cd( path );
      	TKey *key2 = (TKey*)gDirectory->GetListOfKeys()->FindObject(h1->GetName());
      	if (key2) {
      	  TH1 *h2 = (TH1*)key2->ReadObj();
      	  // Brenda: Fri Jun 14th, 3:24 pm
      	  //h2->Sumw2();

      	  double scale = (isData || xsec.at(spot) < 0) ? 1.0 : normFactor.at(spot) * xsec.at(spot)* lumi* skim.at(spot);
          if(isDataDriven){ 
            // std::cout << "Found a data-driven background sample" << std::endl;
            h2->Sumw2();
            scale = skim.at(spot);
          }

      	  scale *= SF.at(spot);

      	  if(strcmp(h2->GetTitle(),"Events") == 0) {
      	    h2->SetBinError(2,getBayesError(h2->GetBinContent(2), h2->GetBinContent(1)));
      	  } else {
      	    for(int i = 1; i <= h2->GetXaxis()->GetNbins(); i++) {
      	      if(h2->GetBinError(i) != h2->GetBinError(i) || h2->GetBinError(i) > h2->GetBinContent(i)) {
      		       h2->SetBinError(i, abs(h2->GetBinContent(i)));
      	      }
      	    }
      	  }

      	  h1->Add( h2, scale);
      	  delete h2;
      	  
      	}
      	nextsource = (TFile*)sourcelist->After( nextsource );
      }
      ////////////////////////////////////////////////////////////
      ////  To gain back Poisson error, uncomment this line /////
      ////////////////////////////////////////////////////////////

      // for(int ibin=0; ibin < (h1->GetXaxis()->GetNbins() + 1); ibin++) {
      // 	h1->SetBinError(ibin, sqrt(pow(h1->GetBinError(ibin),2.0) + abs(h1->GetBinContent(ibin))) );
      // }

    }
    else if ( obj->IsA()->InheritsFrom( TTree::Class() ) ) {

      // loop over all source files create a chain of Trees "globChain"
      const char* obj_name= obj->GetName();

      globChain = new TChain(obj_name);
      globChain->Add(first_source->GetName());
      TFile *nextsource = (TFile*)sourcelist->After( first_source );
      //      const char* file_name = nextsource->GetName();
      // cout << "file name  " << file_name << endl;
      while ( nextsource ) {
	  
	globChain->Add(nextsource->GetName());
	nextsource = (TFile*)sourcelist->After( nextsource );
      }
	
    } else if ( obj->IsA()->InheritsFrom( TDirectory::Class() ) ) {
      // it's a subdirectory

      //      cout << "Found subdirectory " << obj->GetName() << endl;

      // create a new subdir of same name and title in the target file
      target->cd();
      TDirectory *newdir = target->mkdir( obj->GetName(), obj->GetTitle() );

      // newdir is now the starting point of another round of merging
      // newdir still knows its depth within the target file via
      // GetPath(), so we can still figure out where we are in the recursion
      MergeRootfile( newdir );

    } else {

      // object is of no type that we know or can handle
      cout << "Unknown object type, name: "
	   << obj->GetName() << " title: " << obj->GetTitle() << endl;
    }

    // now write the merged histogram (which is "in" obj) to the target file
    // note that this will just store obj in the current directory level,
    // which is not persistent until the complete directory itself is stored
    // by "target->Write()" below
    if ( obj ) {
      target->cd();
      if(obj->IsA()->InheritsFrom( TTree::Class() ))
	globChain->Merge(target->GetFile(),0,"keep");
      else
	obj->Write( key->GetName() );
    }

  } // while ( ( TKey *key = (TKey*)nextkey() ) )

  // save modifications to target file
  target->SaveSelf(kTRUE);
  TH1::AddDirectory(status);
}

