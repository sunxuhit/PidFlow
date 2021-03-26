#ifndef StEventPlaneCons_h
#define StEventPlaneCons_h

#include <string>
#include "TString.h"
// #include "StarClassLibrary/SystemOfUnits.h"

namespace recoEP
{
  //--------------------------------------------------
  // used in Event Plane Reconstruction
  const int NumBeamEnergy = 3;
  const std::string mBeamEnergy[NumBeamEnergy] = {"200GeV_2014","54GeV_2017","27GeV_2018"};
  const double mEnergyValue[NumBeamEnergy] = {200.0,54.0,27.0};
  const int mBeamYear[NumBeamEnergy] = {2014,2017,2018};

  // event cut
  const double mVzMaxMap[NumBeamEnergy] = {6.0,40.0,70.0}; // 0: 200GeV_2014 | 1: 54GeV_2017 | 2: 27GeV_2018 
  const double mVrMax[NumBeamEnergy] = {2.0,2.0,2.0};
  const double mVzVpdDiffMax[NumBeamEnergy] = {3.0,3.0,3.0}; // 3.0
  const int mMatchedToFMin[NumBeamEnergy] = {2,2,2}; // 2

  // track cut
  const double mSigScaleMap[NumBeamEnergy] = {1.0,1.0,1.0};
  const double mDcaEPMax[NumBeamEnergy] = {3.0,1.0,1.0}; // for event plane reconstruction: 1.0 for BES
  const int mHitsDedxMin = 5;
  const int mHitsFitTPCMin = 15;
  const int mHitsMaxTPCMin = 0;
  const double mHitsRatioTPCMin = 0.51;
  const double mEtaMax = 1.0;
  const double mEtaGap = 0.05;
  const double mPrimPtMin[NumBeamEnergy] = {0.2,0.2,0.2}; // for event plane reconstruction and for pion, kaon, proton: 0.2 for BES
  const double mPrimPtMax = 2.0;
  const double mPrimPtWeight = 2.0;
  const double mPrimMomMax = 10.0; // also use for gMom
  const double mGlobPtMin = 0.1; // for phi, Lambda, K0s
  const int mTrackMin = 2;
  const int mTrackMinFull = 4;

  const int mNumOfRunIndex = 4000;

  // ZDC-SMD Event Plane
  const std::string mEastWest[2] = {"East","West"};
  const std::string mVertHori[2] = {"Vertical","Horizontal"};

  const std::string mVStr[2] = {"pos","neg"};
  const std::string mOrder = "2nd";
  const double mShiftOrder[5] = {2.0,4.0,6.0,8.0,10.0};
  const int mNumShiftOrder = 20;
}

#endif