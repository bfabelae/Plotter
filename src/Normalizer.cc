#include "Normalizer.h"

using namespace std;

Normer::Normer() {
  
}

Normer::Normer(vector<string> values) {
  setValues(values);
  output = values[1];
  if(values.size() == 2) {
     isData = true;
     type = "data";
  } else if(values.size() == 5) type = values[4];

}


Normer::Normer(const Normer& other) {
  input = other.input;
  skim = other.skim;
  xsec = other.xsec;
  output = other.output;
  type = other.type;
  lumi = other.lumi;
  integral = other.integral;
  CumulativeEfficiency = other.CumulativeEfficiency;
  scaleFactor = other.scaleFactor;
  scaleFactorError= other.scaleFactorError;
  use = other.use;
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
  use = min(shouldAdd(values[0], values[1]),use);
  CumulativeEfficiency.push_back(1.);
  scaleFactor.push_back(1.);
  scaleFactorError.push_back(0.);
  integral.push_back(0);
  if(values.size() == 2) {
    xsec.push_back(1.0);
    skim.push_back(1.0);
  } else {
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
    int nplot = 0;

    integral.at(nplot) = events->GetBinContent(2);
    CumulativeEfficiency.at(nplot) = events->GetBinContent(2)/ events->GetBinContent(1);

    TFile *nextsource = (TFile*)sourcelist->After( first_source );
    while( nextsource) {
      nplot++;
      nextsource->cd(path);
      gDirectory->GetObject("Events", events);
      integral.at(nplot) = events->GetBinContent(2);
      CumulativeEfficiency.at(nplot) = events->GetBinContent(2)/ events->GetBinContent(1);

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
      h1->Sumw2();
      int spot = 0;
      if(!isData && integral.at(spot) != 0) h1->Scale(CumulativeEfficiency.at(spot)/integral.at(spot) * xsec.at(spot)* lumi* skim.at(spot));

      TFile *nextsource = (TFile*)sourcelist->After( first_source );
      
      while ( nextsource ) {
	spot++;
	nextsource->cd( path );
	TKey *key2 = (TKey*)gDirectory->GetListOfKeys()->FindObject(h1->GetName());
	if (key2) {
	  TH1 *h2 = (TH1*)key2->ReadObj();
	  h2->Sumw2();
	  if(integral.at(spot) == 0) integral.at(spot) = 0.1; 
	  double scale = (isData) ? 1.0 : CumulativeEfficiency.at(spot)/integral.at(spot) * xsec.at(spot)* lumi* skim.at(spot);

	  h1->Add( h2, scale);
	  delete h2;
	  
	}
	nextsource = (TFile*)sourcelist->After( nextsource );
      }
      ////////////////////////////////////////////////////////////
      ////  To gain back Poisson error, uncomment this line /////
      ////////////////////////////////////////////////////////////

      for(int ibin=0; ibin < (h1->GetXaxis()->GetNbins() + 1); ibin++) {
      	h1->SetBinError(ibin, sqrt(pow(h1->GetBinError(ibin),2.0) + h1->GetBinContent(ibin)) );
      }

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

