#include "StMessMgr.h"
#include "TMath.h"
#include "TString.h"
#include "TFile.h"
#include "TProfile2D.h"
#include "TProfile.h"
#include "TNtuple.h"
#include "TRandom3.h"
#include "TF1.h"
#include "TVector3.h"

#include "StPicoDstMaker/StPicoDstMaker.h"
#include "StPicoEvent/StPicoDst.h"
#include "StPicoEvent/StPicoEvent.h"
#include "StPicoEvent/StPicoTrack.h"

#include "StRoot/StEventPlaneMaker/StTpcEpManager.h"
#include "StRoot/StEventPlaneMaker/StEventPlaneCons.h"

double Resolution_TpcFull(double *x_val, double *par)
{
  double y;
  double chi = x_val[0];
  double arg = chi*chi/4.0;
  double norm = TMath::Sqrt(TMath::Pi()/2.0)/2.0;

  y = norm*chi*TMath::Exp(-1.0*arg)*(TMath::BesselI0(arg)+TMath::BesselI1(arg));

  return y;
}

ClassImp(StTpcEpManager)

//---------------------------------------------------------------------------------
StTpcEpManager::StTpcEpManager(Int_t energy)
{
  mEnergy = energy;
  clearTpcEp();
  for(int i_cent = 0; i_cent < 9; ++i_cent)
  {
    mTpcSubRes2Val[i_cent]  = 0.0;
    mTpcSubRes2Err[i_cent]  = 0.0;
    mTpcFullRes2Val[i_cent] = 0.0;
    mTpcFullRes2Err[i_cent] = 0.0;
  }
}

StTpcEpManager::~StTpcEpManager()
{
  /* */
}

void StTpcEpManager::clearTpcEp()
{
  mCent9 = -1;
  mRunIndex = -1;
  mVzSign = -1;

  mQ2VecEastRaw.Set(0.0,0.0);
  mQ2CounterRawEast = 0;
  mQ2VecEast.Set(0.0,0.0);
  mQ2CounterEast = 0;

  mQ2VecWestRaw.Set(0.0,0.0);
  mQ2CounterRawWest = 0;
  mQ2VecWest.Set(0.0,0.0);
  mQ2CounterWest = 0;
  
  mQ2VecFullRaw.Set(0.0,0.0);
  mQ2CounterRawFull = 0;
  mQ2CounterRawFull_East = 0;
  mQ2CounterRawFull_West = 0;
  mQ2VecFull.Set(0.0,0.0);
  mQ2CounterFull = 0;
  mQ2CounterFull_East = 0;
  mQ2CounterFull_West = 0;

  mQ2VecRanA.Set(0.0,0.0);
  mQ2CounterRanA = 0;

  mQ2VecRanB.Set(0.0,0.0);
  mQ2CounterRanB = 0;
}

void StTpcEpManager::initTpcEp(int Cent9, int RunIndex, int VzSign)
{
  mCent9 = Cent9;
  mRunIndex = RunIndex;
  mVzSign = VzSign;
}
//---------------------------------------------------------------------------------
bool StTpcEpManager::passTrackEpEast(StPicoTrack *picoTrack) // neg
{
  TVector3 primMom; // temp fix for StThreeVectorF & TVector3
  const double primPx = picoTrack->pMom().x(); // x works for both TVector3 and StThreeVectorF
  const double primPy = picoTrack->pMom().y();
  const double primPz = picoTrack->pMom().z();
  primMom.SetXYZ(primPx,primPy,primPz);
  const double eta = primMom.PseudoRapidity();

  // const double eta = picoTrack->pMom().pseudoRapidity();

  // eta cut for East EP reconstruction: [-1.0,-0.05]
  // eta_gap between two sub event plane is 2*mEtaGap
  if(!(eta >= -1.0*recoEP::mEtaMax && eta <= -1.0*recoEP::mEtaGap))
  {
    return kFALSE;
  }

  return kTRUE;
}

bool StTpcEpManager::passTrackEpWest(StPicoTrack *picoTrack) // pos 
{
  TVector3 primMom; // temp fix for StThreeVectorF & TVector3
  const double primPx = picoTrack->pMom().x(); // x works for both TVector3 and StThreeVectorF
  const double primPy = picoTrack->pMom().y();
  const double primPz = picoTrack->pMom().z();
  primMom.SetXYZ(primPx,primPy,primPz);
  const double eta = primMom.PseudoRapidity();

  // const double eta = picoTrack->pMom().pseudoRapidity();

  // eta cut for West EP reconstruction: [0.05,1.0]
  // eta_gap between two sub event plane is 2*mEtaGap
  if(!(eta >= recoEP::mEtaGap && eta <= recoEP::mEtaMax))
  {
    return kFALSE;
  }

  return kTRUE;
}

bool StTpcEpManager::passTrackEpFull(StPicoTrack *picoTrack)
{
  TVector3 primMom; // temp fix for StThreeVectorF & TVector3
  const double primPx = picoTrack->pMom().x(); // x works for both TVector3 and StThreeVectorF
  const double primPy = picoTrack->pMom().y();
  const double primPz = picoTrack->pMom().z();
  primMom.SetXYZ(primPx,primPy,primPz);
  const double eta = primMom.PseudoRapidity();

  // eta cut for Full EP reconstruction: [-1.0,1.0]
  // no eta gap for full EP
  if(fabs(eta) > recoEP::mEtaMax)
  {
    return kFALSE;
  }

  return kTRUE;
}
//---------------------------------------------------------------------------------
TVector2 StTpcEpManager::calq2Vector(StPicoTrack *picoTrack)
{
  TVector3 primMom; // temp fix for StThreeVectorF & TVector3
  const double primPx = picoTrack->pMom().x(); // x works for both TVector3 and StThreeVectorF
  const double primPy = picoTrack->pMom().y();
  const double primPz = picoTrack->pMom().z();
  primMom.SetXYZ(primPx,primPy,primPz);
  const double phi = primMom.Phi(); // -pi to pi

  // const double phi = picoTrack->pMom().phi();
  TVector2 q2Vector(0.0,0.0);

  const double q2x = TMath::Cos(2.0*phi);
  const double q2y = TMath::Sin(2.0*phi);
  q2Vector.Set(q2x,q2y);

  return q2Vector;
}

double StTpcEpManager::getWeight(StPicoTrack *picoTrack)
{
  TVector3 primMom; // temp fix for StThreeVectorF & TVector3
  const double primPx = picoTrack->pMom().x(); // x works for both TVector3 and StThreeVectorF
  const double primPy = picoTrack->pMom().y();
  const double primPz = picoTrack->pMom().z();
  primMom.SetXYZ(primPx,primPy,primPz);
  const double pt = primMom.Perp();
  // const double pt = picoTrack->pMom().perp();

  double wgt;
  if(pt <= recoEP::mPrimPtWeight)
  {
    wgt = pt;
  }
  if(pt > recoEP::mPrimPtWeight)
  {
    wgt = recoEP::mPrimPtWeight;
  }

  return wgt;
}
//---------------------------------------------------------------------------------
void StTpcEpManager::addTrackEastRaw(StPicoTrack *picoTrack)
{
  const double wgt = getWeight(picoTrack);
  mQ2VecEastRaw += wgt*calq2Vector(picoTrack);
  mQ2CounterRawEast++;
}

void StTpcEpManager::addTrackWestRaw(StPicoTrack *picoTrack)
{
  const double wgt = getWeight(picoTrack);
  mQ2VecWestRaw += wgt*calq2Vector(picoTrack);
  mQ2CounterRawWest++;
}

void StTpcEpManager::addTrackFullRaw(StPicoTrack *picoTrack)
{
  const double wgt = getWeight(picoTrack);
  mQ2VecFullRaw += wgt*calq2Vector(picoTrack);
  mQ2CounterRawFull++;

  // double eta = picoTrack->pMom().pseudoRapidity();
  TVector3 primMom; // temp fix for StThreeVectorF & TVector3
  const double primPx = picoTrack->pMom().x(); // x works for both TVector3 and StThreeVectorF
  const double primPy = picoTrack->pMom().y();
  const double primPz = picoTrack->pMom().z();
  primMom.SetXYZ(primPx,primPy,primPz);
  const double eta = primMom.PseudoRapidity();
  if(eta >= 0.0)
  {
    mQ2CounterRawFull_West++;
  }
  if(eta < 0.0)
  {
    mQ2CounterRawFull_East++;
  }
}
//---------------------------------------------------------------------------------
void StTpcEpManager::readReCenterCorr()
{
  string InPutFile = Form("StRoot/StEventPlaneUtility/ReCenterParameter/file_%s_ReCenterParameter.root",recoEP::mBeamEnergy[mEnergy].c_str());
  mInPutFile_ReCenter = TFile::Open(InPutFile.c_str());

  string ProName;
  for(int i_vz = 0; i_vz < 2; ++i_vz)
  {
    ProName = Form("p_mTpcq2xEast_%s",recoEP::mVStr[i_vz].c_str());
    p_mTpcq2xEast[i_vz] = (TProfile2D*)mInPutFile_ReCenter->Get(ProName.c_str());
    ProName = Form("p_mTpcq2yEast_%s",recoEP::mVStr[i_vz].c_str());
    p_mTpcq2yEast[i_vz] = (TProfile2D*)mInPutFile_ReCenter->Get(ProName.c_str());
    ProName = Form("p_mTpcq2xWest_%s",recoEP::mVStr[i_vz].c_str());
    p_mTpcq2xWest[i_vz] = (TProfile2D*)mInPutFile_ReCenter->Get(ProName.c_str());
    ProName = Form("p_mTpcq2yWest_%s",recoEP::mVStr[i_vz].c_str());
    p_mTpcq2yWest[i_vz] = (TProfile2D*)mInPutFile_ReCenter->Get(ProName.c_str());

    ProName = Form("p_mTpcq2xFull_%s",recoEP::mVStr[i_vz].c_str());
    p_mTpcq2xFull[i_vz] = (TProfile2D*)mInPutFile_ReCenter->Get(ProName.c_str());
    ProName = Form("p_mTpcq2yFull_%s",recoEP::mVStr[i_vz].c_str());
    p_mTpcq2yFull[i_vz] = (TProfile2D*)mInPutFile_ReCenter->Get(ProName.c_str());
  }
}

TVector2 StTpcEpManager::getReCenterParEast()
{
  double mean_qx, mean_qy;
  TVector2 qVector(0.0,0.0);

  mean_qx = p_mTpcq2xEast[mVzSign]->GetBinContent(p_mTpcq2xEast[mVzSign]->FindBin((double)mRunIndex,(double)mCent9));
  mean_qy = p_mTpcq2yEast[mVzSign]->GetBinContent(p_mTpcq2yEast[mVzSign]->FindBin((double)mRunIndex,(double)mCent9));

  qVector.Set(mean_qx,mean_qy);

  return qVector;
}

TVector2 StTpcEpManager::getReCenterParWest()
{
  double mean_qx, mean_qy;
  TVector2 qVector(0.0,0.0);

  mean_qx = p_mTpcq2xWest[mVzSign]->GetBinContent(p_mTpcq2xWest[mVzSign]->FindBin((double)mRunIndex,(double)mCent9));
  mean_qy = p_mTpcq2yWest[mVzSign]->GetBinContent(p_mTpcq2yWest[mVzSign]->FindBin((double)mRunIndex,(double)mCent9));

  qVector.Set(mean_qx,mean_qy);

  return qVector;
}

TVector2 StTpcEpManager::getReCenterParFull()
{
  double mean_qx, mean_qy;
  TVector2 qVector(0.0,0.0);

  mean_qx = p_mTpcq2xFull[mVzSign]->GetBinContent(p_mTpcq2xFull[mVzSign]->FindBin((double)mRunIndex,(double)mCent9));
  mean_qy = p_mTpcq2yFull[mVzSign]->GetBinContent(p_mTpcq2yFull[mVzSign]->FindBin((double)mRunIndex,(double)mCent9));

  qVector.Set(mean_qx,mean_qy);

  return qVector;
}
//---------------------------------------------------------------------------------
void StTpcEpManager::addTrackEast(StPicoTrack *picoTrack)
{
  const double wgt = getWeight(picoTrack);
  mQ2VecEast += wgt*(calq2Vector(picoTrack) - getReCenterParEast());

  mQ2CounterEast++;
}

void StTpcEpManager::addTrackWest(StPicoTrack *picoTrack)
{
  const double wgt = getWeight(picoTrack);
  mQ2VecWest += wgt*(calq2Vector(picoTrack) - getReCenterParWest());

  mQ2CounterWest++;
}

void StTpcEpManager::addTrackFull(StPicoTrack *picoTrack)
{
  const double wgt = getWeight(picoTrack);
  mQ2VecFull += wgt*(calq2Vector(picoTrack) - getReCenterParFull());

  mQ2CounterFull++;

  // double eta = picoTrack->pMom().pseudoRapidity();
  TVector3 primMom; // temp fix for StThreeVectorF & TVector3
  const double primPx = picoTrack->pMom().x(); // x works for both TVector3 and StThreeVectorF
  const double primPy = picoTrack->pMom().y();
  const double primPz = picoTrack->pMom().z();
  primMom.SetXYZ(primPx,primPy,primPz);
  const double eta = primMom.PseudoRapidity();
  if(eta >= 0.0)
  {
    mQ2CounterFull_West++;
  }
  if(eta < 0.0)
  {
    mQ2CounterFull_East++;
  }
}

void StTpcEpManager::addTrackRanA(StPicoTrack *picoTrack)
{
  const double wgt = getWeight(picoTrack);
  mQ2VecRanA += wgt*(calq2Vector(picoTrack) - getReCenterParFull());

  mQ2CounterRanA++;
}

void StTpcEpManager::addTrackRanB(StPicoTrack *picoTrack)
{
  const double wgt = getWeight(picoTrack);
  mQ2VecRanB += wgt*(calq2Vector(picoTrack) - getReCenterParFull());

  mQ2CounterRanB++;
}

void StTpcEpManager::Randomization()
{
  TRandom3 Ran;
  TVector2 Q2Switch_EP;
  Int_t CSwitch;
  Ran.SetSeed();
  Float_t ran = Ran.Rndm(); // random number between [0,1]
  if(ran < 0.5)
  {
    // switch Event Plane Q Vector
    Q2Switch_EP = mQ2VecRanA;
    mQ2VecRanA = mQ2VecRanB;
    mQ2VecRanB = Q2Switch_EP;

    // switch Counter
    CSwitch = mQ2CounterRanA;
    mQ2CounterRanA = mQ2CounterRanB;
    mQ2CounterRanB = CSwitch;
  }
}

void StTpcEpManager::print(TVector2 vector)
{
  cout << "qx = " << vector.X() << endl;
  cout << "qy = " << vector.Y() << endl;
  cout << endl;
}
//---------------------------------------------------------------------------------
bool StTpcEpManager::passTrackEtaNumRawCut()
{
  if(!(mQ2CounterRawEast > recoEP::mTrackMin && mQ2CounterRawWest > recoEP::mTrackMin))
  {
    return kFALSE;
  }

  return kTRUE;
}

bool StTpcEpManager::passTrackFullNumRawCut()
{
  if(!(mQ2CounterRawFull > recoEP::mTrackMinFull && mQ2CounterRawFull_East > recoEP::mTrackMin && mQ2CounterRawFull_West > recoEP::mTrackMin))
  {
    return kFALSE;
  }
  
  return kTRUE;
}

bool StTpcEpManager::passTrackEtaNumCut()
{
  if(!(mQ2CounterEast > recoEP::mTrackMin && mQ2CounterWest > recoEP::mTrackMin))
  {
    return kFALSE;
  }

  return kTRUE;
}

bool StTpcEpManager::passTrackFullNumCut()
{
  if(!(mQ2CounterFull > recoEP::mTrackMinFull && mQ2CounterFull_East > recoEP::mTrackMin && mQ2CounterFull_West > recoEP::mTrackMin))
  {
    return kFALSE;
  }
  
  return kTRUE;
}
//---------------------------------------------------------------------------------
void StTpcEpManager::readShiftCorr()
{
  string InPutFile = Form("StRoot/StEventPlaneUtility/ShiftParameter/file_%s_ShiftParameter.root",recoEP::mBeamEnergy[mEnergy].c_str());
  mInPutFile_Shift = TFile::Open(InPutFile.c_str());

  string ProName;
  for(int i_vz = 0; i_vz < 2; ++i_vz)
  {
    for(int i_shift = 0; i_shift < recoEP::mNumShiftOrder; ++i_shift) // Shift Order
    {
      ProName = Form("p_mTpcQ2EastCos_%s_%d",recoEP::mVStr[i_vz].c_str(),i_shift);
      p_mTpcQ2EastCos[i_vz][i_shift] = (TProfile2D*)mInPutFile_Shift->Get(ProName.c_str());
      ProName = Form("p_mTpcQ2EastSin_%s_%d",recoEP::mVStr[i_vz].c_str(),i_shift);
      p_mTpcQ2EastSin[i_vz][i_shift] = (TProfile2D*)mInPutFile_Shift->Get(ProName.c_str());

      ProName = Form("p_mTpcQ2WestCos_%s_%d",recoEP::mVStr[i_vz].c_str(),i_shift);
      p_mTpcQ2WestCos[i_vz][i_shift] = (TProfile2D*)mInPutFile_Shift->Get(ProName.c_str());
      ProName = Form("p_mTpcQ2WestSin_%s_%d",recoEP::mVStr[i_vz].c_str(),i_shift);
      p_mTpcQ2WestSin[i_vz][i_shift] = (TProfile2D*)mInPutFile_Shift->Get(ProName.c_str());

      ProName = Form("p_mTpcQ2FullCos_%s_%d",recoEP::mVStr[i_vz].c_str(),i_shift);
      p_mTpcQ2FullCos[i_vz][i_shift] = (TProfile2D*)mInPutFile_Shift->Get(ProName.c_str());
      ProName = Form("p_mTpcQ2FullSin_%s_%d",recoEP::mVStr[i_vz].c_str(),i_shift);
      p_mTpcQ2FullSin[i_vz][i_shift] = (TProfile2D*)mInPutFile_Shift->Get(ProName.c_str());
    }
  }
}

double StTpcEpManager::AngleShift(double Psi2Raw)
{
  double Psi2Corr = Psi2Raw;
  if(Psi2Raw > 0.5*TMath::Pi())
  {
    Psi2Corr = Psi2Raw - TMath::Pi();
  }
  if(Psi2Raw < -0.5*TMath::Pi())
  {
    Psi2Corr = Psi2Raw + TMath::Pi();
  }

  return Psi2Corr;
}
//---------------------------------------------------------------------------------
double StTpcEpManager::calShiftAngle2East()
{
  double Psi_ReCenter = TMath::ATan2(mQ2VecEast.Y(),mQ2VecEast.X())/2.0;
  double delta_Psi = 0.0;
  double Psi_Shift;

  for(Int_t i_shift = 0; i_shift < recoEP::mNumShiftOrder; ++i_shift) // Shift Order loop
  {
    int bin_Cos = p_mTpcQ2EastCos[mVzSign][i_shift]->FindBin((double)mRunIndex,(double)mCent9);
    float mean_Cos = p_mTpcQ2EastCos[mVzSign][i_shift]->GetBinContent(bin_Cos);

    int bin_Sin = p_mTpcQ2EastSin[mVzSign][i_shift]->FindBin((double)mRunIndex,(double)mCent9);
    float mean_Sin = p_mTpcQ2EastSin[mVzSign][i_shift]->GetBinContent(bin_Sin);

    delta_Psi += 0.5*(2.0/((double)i_shift+1.0))*(-1.0*mean_Sin*TMath::Cos(2.0*((double)i_shift+1.0)*Psi_ReCenter)+mean_Cos*TMath::Sin(2.0*((double)i_shift+1.0)*Psi_ReCenter));
  }

  double Psi_Shift_raw = Psi_ReCenter + delta_Psi;
  Psi_Shift = AngleShift(Psi_Shift_raw);

  return Psi_Shift;
}

double StTpcEpManager::calShiftAngle2West()
{
  double Psi_ReCenter = TMath::ATan2(mQ2VecWest.Y(),mQ2VecWest.X())/2.0;
  double delta_Psi = 0.0;
  double Psi_Shift;

  for(Int_t i_shift = 0; i_shift < recoEP::mNumShiftOrder; ++i_shift) // Shift Order loop
  {
    int bin_Cos = p_mTpcQ2WestCos[mVzSign][i_shift]->FindBin((double)mRunIndex,(double)mCent9);
    float mean_Cos = p_mTpcQ2WestCos[mVzSign][i_shift]->GetBinContent(bin_Cos);

    int bin_Sin = p_mTpcQ2WestSin[mVzSign][i_shift]->FindBin((double)mRunIndex,(double)mCent9);
    float mean_Sin = p_mTpcQ2WestSin[mVzSign][i_shift]->GetBinContent(bin_Sin);

    delta_Psi += 0.5*(2.0/((double)i_shift+1.0))*(-1.0*mean_Sin*TMath::Cos(2.0*((double)i_shift+1.0)*Psi_ReCenter)+mean_Cos*TMath::Sin(2.0*((double)i_shift+1.0)*Psi_ReCenter));
  }

  double Psi_Shift_raw = Psi_ReCenter + delta_Psi;
  Psi_Shift = AngleShift(Psi_Shift_raw);

  return Psi_Shift;
}

double StTpcEpManager::calShiftAngle2RanA()
{
  double Psi_ReCenter = TMath::ATan2(mQ2VecRanA.Y(),mQ2VecRanA.X())/2.0;
  double delta_Psi = 0.0;
  double Psi_Shift;

  for(Int_t i_shift = 0; i_shift < recoEP::mNumShiftOrder; ++i_shift) // Shift Order loop
  {
    int bin_Cos = p_mTpcQ2FullCos[mVzSign][i_shift]->FindBin((double)mRunIndex,(double)mCent9);
    float mean_Cos = p_mTpcQ2FullCos[mVzSign][i_shift]->GetBinContent(bin_Cos);

    int bin_Sin = p_mTpcQ2FullSin[mVzSign][i_shift]->FindBin((double)mRunIndex,(double)mCent9);
    float mean_Sin = p_mTpcQ2FullSin[mVzSign][i_shift]->GetBinContent(bin_Sin);

    delta_Psi += 0.5*(2.0/((double)i_shift+1.0))*(-1.0*mean_Sin*TMath::Cos(2.0*((double)i_shift+1.0)*Psi_ReCenter)+mean_Cos*TMath::Sin(2.0*((double)i_shift+1.0)*Psi_ReCenter));
  }

  double Psi_Shift_raw = Psi_ReCenter + delta_Psi;
  Psi_Shift = AngleShift(Psi_Shift_raw);

  return Psi_Shift;
}

double StTpcEpManager::calShiftAngle2RanB()
{
  double Psi_ReCenter = TMath::ATan2(mQ2VecRanB.Y(),mQ2VecRanB.X())/2.0;
  double delta_Psi = 0.0;
  double Psi_Shift;

  for(Int_t i_shift = 0; i_shift < recoEP::mNumShiftOrder; ++i_shift) // Shift Order loop
  {
    int bin_Cos = p_mTpcQ2FullCos[mVzSign][i_shift]->FindBin((double)mRunIndex,(double)mCent9);
    float mean_Cos = p_mTpcQ2FullCos[mVzSign][i_shift]->GetBinContent(bin_Cos);

    int bin_Sin = p_mTpcQ2FullSin[mVzSign][i_shift]->FindBin((double)mRunIndex,(double)mCent9);
    float mean_Sin = p_mTpcQ2FullSin[mVzSign][i_shift]->GetBinContent(bin_Sin);

    delta_Psi += 0.5*(2.0/((double)i_shift+1.0))*(-1.0*mean_Sin*TMath::Cos(2.0*((double)i_shift+1.0)*Psi_ReCenter)+mean_Cos*TMath::Sin(2.0*((double)i_shift+1.0)*Psi_ReCenter));
  }

  double Psi_Shift_raw = Psi_ReCenter + delta_Psi;
  Psi_Shift = AngleShift(Psi_Shift_raw);

  return Psi_Shift;
}

double StTpcEpManager::calShiftAngle2Full()
{
  double Psi_ReCenter = TMath::ATan2(mQ2VecFull.Y(),mQ2VecFull.X())/2.0;
  double delta_Psi = 0.0;
  double Psi_Shift;

  for(Int_t i_shift = 0; i_shift < recoEP::mNumShiftOrder; ++i_shift) // Shift Order loop
  {
    int bin_Cos = p_mTpcQ2FullCos[mVzSign][i_shift]->FindBin((double)mRunIndex,(double)mCent9);
    float mean_Cos = p_mTpcQ2FullCos[mVzSign][i_shift]->GetBinContent(bin_Cos);

    int bin_Sin = p_mTpcQ2FullSin[mVzSign][i_shift]->FindBin((double)mRunIndex,(double)mCent9);
    float mean_Sin = p_mTpcQ2FullSin[mVzSign][i_shift]->GetBinContent(bin_Sin);

    delta_Psi += 0.5*(2.0/((double)i_shift+1.0))*(-1.0*mean_Sin*TMath::Cos(2.0*((double)i_shift+1.0)*Psi_ReCenter)+mean_Cos*TMath::Sin(2.0*((double)i_shift+1.0)*Psi_ReCenter));
  }

  double Psi_Shift_raw = Psi_ReCenter + delta_Psi;
  Psi_Shift = AngleShift(Psi_Shift_raw);

  return Psi_Shift;
}

double StTpcEpManager::calShiftAngle2Full(StPicoTrack *picoTrack)
{
  TVector2 QVector_sub = mQ2VecFull;
  if(passTrackEpFull(picoTrack))
  {
    double wgt = getWeight(picoTrack);
    QVector_sub = mQ2VecFull - wgt*(calq2Vector(picoTrack) - getReCenterParFull());
  }
  double Psi_ReCenter = TMath::ATan2(QVector_sub.Y(),QVector_sub.X())/2.0;
  double delta_Psi = 0.0;
  double Psi_Shift;

  for(Int_t i_shift = 0; i_shift < recoEP::mNumShiftOrder; ++i_shift) // Shift Order loop
  {
    int bin_Cos = p_mTpcQ2FullCos[mVzSign][i_shift]->FindBin((double)mRunIndex,(double)mCent9);
    float mean_Cos = p_mTpcQ2FullCos[mVzSign][i_shift]->GetBinContent(bin_Cos);

    int bin_Sin = p_mTpcQ2FullSin[mVzSign][i_shift]->FindBin((double)mRunIndex,(double)mCent9);
    float mean_Sin = p_mTpcQ2FullSin[mVzSign][i_shift]->GetBinContent(bin_Sin);

    delta_Psi += 0.5*(2.0/((double)i_shift+1.0))*(-1.0*mean_Sin*TMath::Cos(2.0*((double)i_shift+1.0)*Psi_ReCenter)+mean_Cos*TMath::Sin(2.0*((double)i_shift+1.0)*Psi_ReCenter));
  }

  double Psi_Shift_raw = Psi_ReCenter + delta_Psi;
  Psi_Shift = AngleShift(Psi_Shift_raw);

  return Psi_Shift;
}
//---------------------------------------------------------------------------------
void StTpcEpManager::readResolution()
{
  string InPutFile = Form("StRoot/StEventPlaneUtility/Resolution/file_%s_Resolution.root",recoEP::mBeamEnergy[mEnergy].c_str());
  mInPutFile_Res = TFile::Open(InPutFile.c_str());

  // calculate sub event plane resolution
  TProfile *p_mTpcSubRes2 = (TProfile*)mInPutFile_Res->Get("p_mTpcSubRes2");
  for(int i_cent = 0; i_cent < 9; ++i_cent)
  {
    const double resRaw = p_mTpcSubRes2->GetBinContent(p_mTpcSubRes2->FindBin(i_cent));
    const double errRaw = p_mTpcSubRes2->GetBinError(p_mTpcSubRes2->FindBin(i_cent));
    if(resRaw > 0)
    {
      mTpcSubRes2Val[i_cent] = TMath::Sqrt(resRaw);
      mTpcSubRes2Err[i_cent] = errRaw/(2.0*TMath::Sqrt(resRaw));
    }
    // cout << "i_cent = " << i_cent << ", resRaw = " << resRaw << ", resSub = " << mTpcSubRes2Val[i_cent] << " +/- " << mTpcSubRes2Err[i_cent] << endl;
  }

  // calculate full event plane resolution
  TProfile *p_mTpcRanRes2 = (TProfile*)mInPutFile_Res->Get("p_mTpcRanRes2");
  for(int i_cent = 0; i_cent < 9; ++i_cent)
  {
    const double resRaw = p_mTpcRanRes2->GetBinContent(p_mTpcRanRes2->FindBin(i_cent));
    const double errRaw = p_mTpcRanRes2->GetBinError(p_mTpcRanRes2->FindBin(i_cent));
    if(resRaw > 0)
    {
      const double resSub = TMath::Sqrt(resRaw);
      const double errSub = errRaw/(2.0*TMath::Sqrt(resRaw));

      TF1 *f_res = new TF1("f_res",Resolution_TpcFull,0,10,0);
      const double chiSub = f_res->GetX(resSub);
      const double errChiSub = errSub/f_res->Derivative(chiSub);
      const double chiFull = chiSub*TMath::Sqrt(2.0);
      mTpcFullRes2Val[i_cent] = f_res->Eval(chiFull);
      mTpcFullRes2Err[i_cent] = f_res->Derivative(chiFull)*errChiSub*TMath::Sqrt(2.0);
    }
    // cout << "i_cent = " << i_cent << ", resRaw = " << resRaw << ", resFull = " << mTpcFullRes2Val[i_cent] << " +/- " << mTpcFullRes2Err[i_cent] << endl;
  }
}

double StTpcEpManager::getRes2Sub(int Cent9)
{
  return mTpcSubRes2Val[Cent9];
}

double StTpcEpManager::getRes2SubErr(int Cent9)
{
  return mTpcSubRes2Err[Cent9];
}

double StTpcEpManager::getRes2Full(int Cent9)
{
  return mTpcFullRes2Val[Cent9];
}

double StTpcEpManager::getRes2FullErr(int Cent9)
{
  return mTpcFullRes2Err[Cent9];
}
//---------------------------------------------------------------------------------
TVector2 StTpcEpManager::getQVector(int nEP) // east/west
{
  TVector2 QVector(-99.9,-99.9);
  if(nEP == 0) QVector = mQ2VecEast;
  if(nEP == 1) QVector = mQ2VecWest;
  if(nEP == 2) QVector = mQ2VecFull;
  if(nEP == 3) QVector = mQ2VecRanA;
  if(nEP == 4) QVector = mQ2VecRanB;

  return QVector;
}

TVector2 StTpcEpManager::getQVectorRaw(int nEP)
{
  TVector2 QVector(-99.9,-99.9);
  if(nEP == 0) QVector = mQ2VecEastRaw;
  if(nEP == 1) QVector = mQ2VecWestRaw;
  if(nEP == 2) QVector = mQ2VecFullRaw;

  return QVector;
}

int StTpcEpManager::getNumTrack(int nEP)
{
  int Q2Counter = -1;
  if(nEP == 0) Q2Counter = mQ2CounterEast;
  if(nEP == 1) Q2Counter = mQ2CounterWest;
  if(nEP == 2) Q2Counter = mQ2CounterFull;
  if(nEP == 3) Q2Counter = mQ2CounterFull_East;
  if(nEP == 4) Q2Counter = mQ2CounterFull_West;

  return Q2Counter;
}
//---------------------------------------------------------------------------------
