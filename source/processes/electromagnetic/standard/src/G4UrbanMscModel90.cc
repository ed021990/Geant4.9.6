//
// ********************************************************************
// * License and Disclaimer                                           *
// *                                                                  *
// * The  Geant4 software  is  copyright of the Copyright Holders  of *
// * the Geant4 Collaboration.  It is provided  under  the terms  and *
// * conditions of the Geant4 Software License,  included in the file *
// * LICENSE and available at  http://cern.ch/geant4/license .  These *
// * include a list of copyright holders.                             *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.  Please see the license in the file  LICENSE  and URL above *
// * for the full disclaimer and the limitation of liability.         *
// *                                                                  *
// * This  code  implementation is the result of  the  scientific and *
// * technical work of the GEANT4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************
//
// $Id: G4UrbanMscModel90.cc 66592 2012-12-23 09:34:55Z vnivanch $
//
// -------------------------------------------------------------------
//
// GEANT4 Class file
//
//
// File name:   G4UrbanMscModel90
//
// Author:        V.Ivanchenko clone Laszlo Urban model
//
// Creation date: 07.12.2007
//
// Modifications:
//
//

// Class Description:
//
// Implementation of the model of multiple scattering based on
// H.W.Lewis Phys Rev 78 (1950) 526 and others

// -------------------------------------------------------------------
//


//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#include "G4UrbanMscModel90.hh"
#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"
#include "G4Electron.hh"

#include "G4LossTableManager.hh"
#include "G4ParticleChangeForMSC.hh"

#include "G4Poisson.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

using namespace std;

G4UrbanMscModel90::G4UrbanMscModel90(const G4String& nam)
  : G4VMscModel(nam)
{
  taubig        = 8.0;
  tausmall      = 1.e-20;
  taulim        = 1.e-6;
  currentTau    = taulim;
  tlimitminfix  = 1.e-6*mm;            
  stepmin       = tlimitminfix;
  smallstep     = 1.e10;
  currentRange  = 0. ;
  frscaling2    = 0.25;
  frscaling1    = 1.-frscaling2;
  tlimit        = 1.e10*mm;
  tlimitmin     = 10.*tlimitminfix;            
  nstepmax      = 25.;
  geombig       = 1.e50*mm;
  geommin       = 1.e-3*mm;
  geomlimit     = geombig;
  presafety     = 0.*mm;
  Zeff          = 1.;
  particle      = 0;
  theManager    = G4LossTableManager::Instance(); 
  firstStep     = true; 
  inside        = false;  
  insideskin    = false;

  skindepth = skin*stepmin;

  mass = proton_mass_c2;
  charge = 1.0;
  currentKinEnergy = currentRange = currentRadLength = masslimite = masslimitmu 
    = lambda0 = lambdaeff = tPathLength = zPathLength = par1 = par2 = par3 = 0;

  currentMaterialIndex = -1;
  fParticleChange = 0;
  couple = 0;
  G4cout << "### G4UrbanMscModel90 is obsolete and will be removed for "
	 << "the next Geant4 version" << G4endl;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4UrbanMscModel90::~G4UrbanMscModel90()
{}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void G4UrbanMscModel90::Initialise(const G4ParticleDefinition* p,
				   const G4DataVector&)
{
  skindepth = skin*stepmin;

  // set values of some data members
  SetParticle(p);

  if(p->GetPDGMass() < MeV) {
    G4cout << "### WARNING: G4UrbanMscModel90 model is used for " 
	   << p->GetParticleName() << " !!! " << G4endl;
    G4cout << "###          This model should be used only for heavy particles" 
	   << G4endl;
  }


  fParticleChange = GetParticleChangeForMSC(p);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4double G4UrbanMscModel90::ComputeCrossSectionPerAtom( 
                             const G4ParticleDefinition* part,
                                   G4double KineticEnergy,
                                   G4double AtomicNumber,G4double,
				   G4double, G4double)
{
  const G4double sigmafactor = twopi*classic_electr_radius*classic_electr_radius;
  const G4double epsfactor = 2.*electron_mass_c2*electron_mass_c2*
                            Bohr_radius*Bohr_radius/(hbarc*hbarc);
  const G4double epsmin = 1.e-4 , epsmax = 1.e10;

  const G4double Zdat[15] = { 4.,  6., 13., 20., 26., 29., 32., 38., 47.,
                             50., 56., 64., 74., 79., 82. };

  const G4double Tdat[22] = { 100*eV,  200*eV,  400*eV,  700*eV,
                               1*keV,   2*keV,   4*keV,   7*keV,
			      10*keV,  20*keV,  40*keV,  70*keV,
                             100*keV, 200*keV, 400*keV, 700*keV,
                               1*MeV,   2*MeV,   4*MeV,   7*MeV,
			      10*MeV,  20*MeV};

  // corr. factors for e-/e+ lambda for T <= Tlim
          G4double celectron[15][22] =
          {{1.125,1.072,1.051,1.047,1.047,1.050,1.052,1.054,
            1.054,1.057,1.062,1.069,1.075,1.090,1.105,1.111,
            1.112,1.108,1.100,1.093,1.089,1.087            },
           {1.408,1.246,1.143,1.096,1.077,1.059,1.053,1.051,
            1.052,1.053,1.058,1.065,1.072,1.087,1.101,1.108,
            1.109,1.105,1.097,1.090,1.086,1.082            },
           {2.833,2.268,1.861,1.612,1.486,1.309,1.204,1.156,
            1.136,1.114,1.106,1.106,1.109,1.119,1.129,1.132,
            1.131,1.124,1.113,1.104,1.099,1.098            },
           {3.879,3.016,2.380,2.007,1.818,1.535,1.340,1.236,
            1.190,1.133,1.107,1.099,1.098,1.103,1.110,1.113,
            1.112,1.105,1.096,1.089,1.085,1.098            },
           {6.937,4.330,2.886,2.256,1.987,1.628,1.395,1.265,
            1.203,1.122,1.080,1.065,1.061,1.063,1.070,1.073,
            1.073,1.070,1.064,1.059,1.056,1.056            },
           {9.616,5.708,3.424,2.551,2.204,1.762,1.485,1.330,
            1.256,1.155,1.099,1.077,1.070,1.068,1.072,1.074,
            1.074,1.070,1.063,1.059,1.056,1.052            },
           {11.72,6.364,3.811,2.806,2.401,1.884,1.564,1.386,
            1.300,1.180,1.112,1.082,1.073,1.066,1.068,1.069,
            1.068,1.064,1.059,1.054,1.051,1.050            },
           {18.08,8.601,4.569,3.183,2.662,2.025,1.646,1.439,
            1.339,1.195,1.108,1.068,1.053,1.040,1.039,1.039,
            1.039,1.037,1.034,1.031,1.030,1.036            },
           {18.22,10.48,5.333,3.713,3.115,2.367,1.898,1.631,
            1.498,1.301,1.171,1.105,1.077,1.048,1.036,1.033,
            1.031,1.028,1.024,1.022,1.021,1.024            },
           {14.14,10.65,5.710,3.929,3.266,2.453,1.951,1.669,
            1.528,1.319,1.178,1.106,1.075,1.040,1.027,1.022,
            1.020,1.017,1.015,1.013,1.013,1.020            },
           {14.11,11.73,6.312,4.240,3.478,2.566,2.022,1.720,
            1.569,1.342,1.186,1.102,1.065,1.022,1.003,0.997,
            0.995,0.993,0.993,0.993,0.993,1.011            },
           {22.76,20.01,8.835,5.287,4.144,2.901,2.219,1.855,
            1.677,1.410,1.224,1.121,1.073,1.014,0.986,0.976,
            0.974,0.972,0.973,0.974,0.975,0.987            },
           {50.77,40.85,14.13,7.184,5.284,3.435,2.520,2.059,
            1.837,1.512,1.283,1.153,1.091,1.010,0.969,0.954,
            0.950,0.947,0.949,0.952,0.954,0.963            },
           {65.87,59.06,15.87,7.570,5.567,3.650,2.682,2.182,
            1.939,1.579,1.325,1.178,1.108,1.014,0.965,0.947,
            0.941,0.938,0.940,0.944,0.946,0.954            },
           {55.60,47.34,15.92,7.810,5.755,3.767,2.760,2.239,
            1.985,1.609,1.343,1.188,1.113,1.013,0.960,0.939,
            0.933,0.930,0.933,0.936,0.939,0.949            }};
	    
           G4double cpositron[15][22] = {
           {2.589,2.044,1.658,1.446,1.347,1.217,1.144,1.110,
            1.097,1.083,1.080,1.086,1.092,1.108,1.123,1.131,
            1.131,1.126,1.117,1.108,1.103,1.100            },
           {3.904,2.794,2.079,1.710,1.543,1.325,1.202,1.145,
            1.122,1.096,1.089,1.092,1.098,1.114,1.130,1.137,
            1.138,1.132,1.122,1.113,1.108,1.102            },
           {7.970,6.080,4.442,3.398,2.872,2.127,1.672,1.451,
            1.357,1.246,1.194,1.179,1.178,1.188,1.201,1.205,
            1.203,1.190,1.173,1.159,1.151,1.145            },
           {9.714,7.607,5.747,4.493,3.815,2.777,2.079,1.715,
            1.553,1.353,1.253,1.219,1.211,1.214,1.225,1.228,
            1.225,1.210,1.191,1.175,1.166,1.174            },
           {17.97,12.95,8.628,6.065,4.849,3.222,2.275,1.820,
            1.624,1.382,1.259,1.214,1.202,1.202,1.214,1.219,
            1.217,1.203,1.184,1.169,1.160,1.151            },
           {24.83,17.06,10.84,7.355,5.767,3.707,2.546,1.996,
            1.759,1.465,1.311,1.252,1.234,1.228,1.238,1.241,
            1.237,1.222,1.201,1.184,1.174,1.159            },
           {23.26,17.15,11.52,8.049,6.375,4.114,2.792,2.155,
            1.880,1.535,1.353,1.281,1.258,1.247,1.254,1.256,
            1.252,1.234,1.212,1.194,1.183,1.170            },
           {22.33,18.01,12.86,9.212,7.336,4.702,3.117,2.348,
            2.015,1.602,1.385,1.297,1.268,1.251,1.256,1.258,
            1.254,1.237,1.214,1.195,1.185,1.179            },
           {33.91,24.13,15.71,10.80,8.507,5.467,3.692,2.808,
            2.407,1.873,1.564,1.425,1.374,1.330,1.324,1.320,
            1.312,1.288,1.258,1.235,1.221,1.205            },
           {32.14,24.11,16.30,11.40,9.015,5.782,3.868,2.917,
            2.490,1.925,1.596,1.447,1.391,1.342,1.332,1.327,
            1.320,1.294,1.264,1.240,1.226,1.214            },
           {29.51,24.07,17.19,12.28,9.766,6.238,4.112,3.066,
            2.602,1.995,1.641,1.477,1.414,1.356,1.342,1.336,
            1.328,1.302,1.270,1.245,1.231,1.233            },
           {38.19,30.85,21.76,15.35,12.07,7.521,4.812,3.498,
            2.926,2.188,1.763,1.563,1.484,1.405,1.382,1.371,
            1.361,1.330,1.294,1.267,1.251,1.239            },
           {49.71,39.80,27.96,19.63,15.36,9.407,5.863,4.155,
            3.417,2.478,1.944,1.692,1.589,1.480,1.441,1.423,
            1.409,1.372,1.330,1.298,1.280,1.258            },
           {59.25,45.08,30.36,20.83,16.15,9.834,6.166,4.407,
            3.641,2.648,2.064,1.779,1.661,1.531,1.482,1.459,
            1.442,1.400,1.354,1.319,1.299,1.272            },
           {56.38,44.29,30.50,21.18,16.51,10.11,6.354,4.542,
            3.752,2.724,2.116,1.817,1.692,1.554,1.499,1.474,
            1.456,1.412,1.364,1.328,1.307,1.282            }};

  //data/corrections for T > Tlim  
  G4double Tlim = 10.*MeV;
  G4double beta2lim = Tlim*(Tlim+2.*electron_mass_c2)/
                      ((Tlim+electron_mass_c2)*(Tlim+electron_mass_c2));
  G4double bg2lim   = Tlim*(Tlim+2.*electron_mass_c2)/
                      (electron_mass_c2*electron_mass_c2);

  G4double sig0[15] = {0.2672*barn,  0.5922*barn, 2.653*barn,  6.235*barn,
                      11.69*barn  , 13.24*barn  , 16.12*barn, 23.00*barn ,
		      35.13*barn  , 39.95*barn  , 50.85*barn, 67.19*barn ,
                      91.15*barn  , 104.4*barn  , 113.1*barn};
		      		       
  G4double hecorr[15] = {120.70, 117.50, 105.00, 92.92, 79.23,  74.510,  68.29,
                          57.39,  41.97,  36.14, 24.53, 10.21,  -7.855, -16.84,
			 -22.30};

  G4double sigma;
  SetParticle(part);

  G4double Z23 = 2.*log(AtomicNumber)/3.; Z23 = exp(Z23);

  // correction if particle .ne. e-/e+
  // compute equivalent kinetic energy
  // lambda depends on p*beta ....

  G4double eKineticEnergy = KineticEnergy;

  if(mass > electron_mass_c2)
  {
    G4double TAU = KineticEnergy/mass ;
    G4double c = mass*TAU*(TAU+2.)/(electron_mass_c2*(TAU+1.)) ;
    G4double w = c-2. ;
    G4double tau = 0.5*(w+sqrt(w*w+4.*c)) ;
    eKineticEnergy = electron_mass_c2*tau ;
  }

  G4double ChargeSquare = charge*charge;

  G4double eTotalEnergy = eKineticEnergy + electron_mass_c2 ;
  G4double beta2 = eKineticEnergy*(eTotalEnergy+electron_mass_c2)
                                 /(eTotalEnergy*eTotalEnergy);
  G4double bg2   = eKineticEnergy*(eTotalEnergy+electron_mass_c2)
                                 /(electron_mass_c2*electron_mass_c2);

  G4double eps = epsfactor*bg2/Z23;

  if     (eps<epsmin)  sigma = 2.*eps*eps;
  else if(eps<epsmax)  sigma = log(1.+2.*eps)-2.*eps/(1.+2.*eps);
  else                 sigma = log(2.*eps)-1.+1./eps;

  sigma *= ChargeSquare*AtomicNumber*AtomicNumber/(beta2*bg2);

  // interpolate in AtomicNumber and beta2 
  G4double c1,c2,cc1,cc2,corr;

  // get bin number in Z
  G4int iZ = 14;
  while ((iZ>=0)&&(Zdat[iZ]>=AtomicNumber)) iZ -= 1;
  if (iZ==14)                               iZ = 13;
  if (iZ==-1)                               iZ = 0 ;

  G4double Z1 = Zdat[iZ];
  G4double Z2 = Zdat[iZ+1];
  G4double ratZ = (AtomicNumber-Z1)*(AtomicNumber+Z1)/
                  ((Z2-Z1)*(Z2+Z1));

  if(eKineticEnergy <= Tlim) 
  {
    // get bin number in T (beta2)
    G4int iT = 21;
    while ((iT>=0)&&(Tdat[iT]>=eKineticEnergy)) iT -= 1;
    if(iT==21)                                  iT = 20;
    if(iT==-1)                                  iT = 0 ;

    //  calculate betasquare values
    G4double T = Tdat[iT],   E = T + electron_mass_c2;
    G4double b2small = T*(E+electron_mass_c2)/(E*E);

    T = Tdat[iT+1]; E = T + electron_mass_c2;
    G4double b2big = T*(E+electron_mass_c2)/(E*E);
    G4double ratb2 = (beta2-b2small)/(b2big-b2small);

    if (charge < 0.)
    {
       c1 = celectron[iZ][iT];
       c2 = celectron[iZ+1][iT];
       cc1 = c1+ratZ*(c2-c1);

       c1 = celectron[iZ][iT+1];
       c2 = celectron[iZ+1][iT+1];
       cc2 = c1+ratZ*(c2-c1);

       corr = cc1+ratb2*(cc2-cc1);

       sigma *= sigmafactor/corr;
    }
    else              
    {
       c1 = cpositron[iZ][iT];
       c2 = cpositron[iZ+1][iT];
       cc1 = c1+ratZ*(c2-c1);

       c1 = cpositron[iZ][iT+1];
       c2 = cpositron[iZ+1][iT+1];
       cc2 = c1+ratZ*(c2-c1);

       corr = cc1+ratb2*(cc2-cc1);

       sigma *= sigmafactor/corr;
    }
  }
  else
  {
    c1 = bg2lim*sig0[iZ]*(1.+hecorr[iZ]*(beta2-beta2lim))/bg2;
    c2 = bg2lim*sig0[iZ+1]*(1.+hecorr[iZ+1]*(beta2-beta2lim))/bg2;
    if((AtomicNumber >= Z1) && (AtomicNumber <= Z2))
      sigma = c1+ratZ*(c2-c1) ;
    else if(AtomicNumber < Z1)
      sigma = AtomicNumber*AtomicNumber*c1/(Z1*Z1);
    else if(AtomicNumber > Z2)
      sigma = AtomicNumber*AtomicNumber*c2/(Z2*Z2);
  }
  return sigma;

}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void G4UrbanMscModel90::StartTracking(G4Track* track)
{
  SetParticle(track->GetDynamicParticle()->GetDefinition());
  firstStep = true; 
  inside = false;
  insideskin = false;
  tlimit = geombig;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4double G4UrbanMscModel90::ComputeTruePathLengthLimit(
                             const G4Track& track,
			     G4double& currentMinimalStep)
{
  tPathLength = currentMinimalStep;
  const G4DynamicParticle* dp = track.GetDynamicParticle();
  G4StepPoint* sp = track.GetStep()->GetPreStepPoint();
  G4StepStatus stepStatus = sp->GetStepStatus();
  couple = track.GetMaterialCutsCouple();
  SetCurrentCouple(couple); 
  currentMaterialIndex = couple->GetIndex();
  currentKinEnergy = dp->GetKineticEnergy();
  currentRange =  GetRange(particle,currentKinEnergy,couple);
  lambda0 = GetTransportMeanFreePath(particle,currentKinEnergy);

  // stop here if small range particle
  if(inside || tPathLength < tlimitminfix) { 
    return ConvertTrueToGeom(tPathLength, currentMinimalStep); 
  }
  
  if(tPathLength > currentRange) { tPathLength = currentRange; }

  presafety = sp->GetSafety();
  /*
  G4cout << "G4UrbanMscModel90::ComputeTruePathLengthLimit tPathLength= " 
  	 <<tPathLength<<" safety= " << presafety
       << " range= " <<currentRange<<G4endl;
  */
  // far from geometry boundary
  if(currentRange < presafety)
    {
      inside = true;
      return ConvertTrueToGeom(tPathLength, currentMinimalStep);  
    }

  // standard  version
  //
  if (steppingAlgorithm == fUseDistanceToBoundary)
    {
      //compute geomlimit and presafety 
      geomlimit = ComputeGeomLimit(track, presafety, currentRange);
   
      // is far from boundary
      if(currentRange <= presafety)
	{
	  inside = true;
	  return ConvertTrueToGeom(tPathLength, currentMinimalStep);   
	}

      smallstep += 1.;
      insideskin = false;

      if(firstStep || stepStatus == fGeomBoundary)
	{

	  if(firstStep) smallstep = 1.e10;
	  else  smallstep = 1.;

	  // facrange scaling in lambda 
	  // not so strong step restriction above lambdalimit
	  G4double facr = facrange;
	  if(lambda0 > lambdalimit)
	    facr *= frscaling1+frscaling2*lambda0/lambdalimit;

	  // constraint from the physics
	  if (currentRange > lambda0) tlimit = facr*currentRange;
	  else                        tlimit = facr*lambda0;

	  // constraint from the geometry (if tlimit above is too big)
	  G4double tgeom = geombig; 
	  if(geomlimit > geommin)
	    {
	      if(stepStatus == fGeomBoundary)  
		tgeom = geomlimit/facgeom;
	      else
		tgeom = 2.*geomlimit/facgeom;
	    }

	  //define stepmin here (it depends on lambda!)
	  //rough estimation of lambda_elastic/lambda_transport
	  G4double rat = currentKinEnergy/MeV ;
	  rat = 1.e-3/(rat*(10.+rat)) ;
	  //stepmin ~ lambda_elastic
	  stepmin = rat*lambda0;
	  skindepth = skin*stepmin;

	  //define tlimitmin
	  tlimitmin = lambda0/nstepmax;
	  if(tlimitmin < stepmin) tlimitmin = 1.01*stepmin;
	  if(tlimitmin < tlimitminfix) tlimitmin = tlimitminfix;

	  //lower limit for tlimit
	  if(tlimit < tlimitmin) tlimit = tlimitmin;

	  //check against geometry limit
	  if(tlimit > tgeom) tlimit = tgeom;
	}

      //if track starts far from boundaries increase tlimit!
      if(tlimit < facsafety*presafety) tlimit = facsafety*presafety ;

      //  G4cout << "tgeom= " << tgeom << " geomlimit= " << geomlimit  
      //     << " tlimit= " << tlimit << " presafety= " << presafety << G4endl;

      // shortcut
      if((tPathLength < tlimit) && (tPathLength < presafety))
	return ConvertTrueToGeom(tPathLength, currentMinimalStep);   

      G4double tnow = tlimit;
      // optimization ...
      if(geomlimit < geombig) tnow = max(tlimit,facsafety*geomlimit);
   
      // step reduction near to boundary
      if(smallstep < skin)
	{
	  tnow = stepmin;
	  insideskin = true;
	}
      else if(geomlimit < geombig)
	{
	  if(geomlimit > skindepth)
	    {
	      if(tnow > geomlimit-0.999*skindepth)
		tnow = geomlimit-0.999*skindepth;
	    }
	  else
	    {
	      insideskin = true;
	      if(tnow > stepmin) tnow = stepmin;
	    }
	}

      if(tnow < stepmin) tnow = stepmin;

      if(tPathLength > tnow) tPathLength = tnow ; 
    }
    // for 'normal' simulation with or without magnetic field 
    //  there no small step/single scattering at boundaries
  else if(steppingAlgorithm == fUseSafety)
    {
      // compute presafety again if presafety <= 0 and no boundary
      // i.e. when it is needed for optimization purposes
      if((stepStatus != fGeomBoundary) && (presafety < tlimitminfix)) 
	presafety = ComputeSafety(sp->GetPosition(),tPathLength); 

      // is far from boundary
      if(currentRange < presafety)
        {
          inside = true;
          return ConvertTrueToGeom(tPathLength, currentMinimalStep);  
        }

      if(firstStep || stepStatus == fGeomBoundary)
	{ 
	  // facrange scaling in lambda 
	  // not so strong step restriction above lambdalimit
	  G4double facr = facrange;
	  if(lambda0 > lambdalimit)
	    facr *= frscaling1+frscaling2*lambda0/lambdalimit;

	  // constraint from the physics
	  if (currentRange > lambda0) tlimit = facr*currentRange;
	  else                        tlimit = facr*lambda0;

	  //lower limit for tlimit
	  tlimitmin = std::max(tlimitminfix,lambda0/nstepmax);
	  if(tlimit < tlimitmin) tlimit = tlimitmin;
	}

      //if track starts far from boundaries increase tlimit!
      if(tlimit < facsafety*presafety) tlimit = facsafety*presafety ;

      if(tPathLength > tlimit) tPathLength = tlimit;
    }
  
  // version similar to 7.1 (needed for some experiments)
  else
    {
      if (stepStatus == fGeomBoundary)
	{
	  if (currentRange > lambda0) tlimit = facrange*currentRange;
	  else                        tlimit = facrange*lambda0;

	  if(tlimit < tlimitmin) tlimit = tlimitmin;
	  if(tPathLength > tlimit) tPathLength = tlimit;
	}
    }
  //  G4cout << "tPathLength= " << tPathLength << "  geomlimit= " << geomlimit 
  //	 << " currentMinimalStep= " << currentMinimalStep << G4endl;
  return ConvertTrueToGeom(tPathLength, currentMinimalStep);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4double G4UrbanMscModel90::ComputeGeomPathLength(G4double)
{
  firstStep = false; 
  lambdaeff = lambda0;
  par1 = -1. ;  
  par2 = par3 = 0. ;  

  //  do the true -> geom transformation
  zPathLength = tPathLength;

  // z = t for very small tPathLength
  if(tPathLength < tlimitminfix) return zPathLength;

  // this correction needed to run MSC with eIoni and eBrem inactivated
  // and makes no harm for a normal run
  if(tPathLength > currentRange)
    tPathLength = currentRange ;

  G4double tau   = tPathLength/lambda0 ;

  if ((tau <= tausmall) || insideskin) {
    zPathLength  = tPathLength;
    if(zPathLength > lambda0) zPathLength = lambda0;
    return zPathLength;
  }

  G4double zmean = tPathLength;
  if (tPathLength < currentRange*dtrl) {
    if(tau < taulim) zmean = tPathLength*(1.-0.5*tau) ;
    else             zmean = lambda0*(1.-exp(-tau));
  } else if(currentKinEnergy < mass || tPathLength == currentRange) {
    par1 = 1./currentRange ;
    par2 = 1./(par1*lambda0) ;
    par3 = 1.+par2 ;
    if(tPathLength < currentRange)
      zmean = (1.-exp(par3*log(1.-tPathLength/currentRange)))/(par1*par3) ;
    else
      zmean = 1./(par1*par3) ;
  } else {
    G4double T1 = GetEnergy(particle,currentRange-tPathLength,couple);
    G4double lambda1 = GetTransportMeanFreePath(particle,T1);

    par1 = (lambda0-lambda1)/(lambda0*tPathLength) ;
    par2 = 1./(par1*lambda0) ;
    par3 = 1.+par2 ;
    zmean = (1.-exp(par3*log(lambda1/lambda0)))/(par1*par3) ;
  }

  zPathLength = zmean ;

  //  sample z
  if(samplez)
  {
    const G4double  ztmax = 0.99, onethird = 1./3. ;
    G4double zt = zmean/tPathLength ;

    if (tPathLength > stepmin && zt < ztmax)              
    {
      G4double u,cz1;
      if(zt >= onethird)
      {
        G4double cz = 0.5*(3.*zt-1.)/(1.-zt) ;
        cz1 = 1.+cz ;
        G4double u0 = cz/cz1 ;
        G4double grej ;
        do {
            u = exp(log(G4UniformRand())/cz1) ;
            grej = exp(cz*log(u/u0))*(1.-u)/(1.-u0) ;
           } while (grej < G4UniformRand()) ;
      }
      else
      {
        cz1 = 1./zt-1.;
        u = 1.-exp(log(G4UniformRand())/cz1) ;
      }
      zPathLength = tPathLength*u ;
    }
  }

  if(zPathLength > lambda0) zPathLength = lambda0;

  return zPathLength;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4double G4UrbanMscModel90::ComputeTrueStepLength(G4double geomStepLength)
{
  // step defined other than transportation 
  if(geomStepLength == zPathLength && tPathLength <= currentRange)
    { return tPathLength; }

  // t = z for very small step
  zPathLength = geomStepLength;
  tPathLength = geomStepLength;
  if(geomStepLength < tlimitminfix) return tPathLength;
  
  // recalculation
  if((geomStepLength > lambda0*tausmall) && !insideskin)
  {
    if(par1 <  0.)
      tPathLength = -lambda0*log(1.-geomStepLength/lambda0) ;
    else 
    {
      if(par1*par3*geomStepLength < 1.)
        tPathLength = (1.-exp(log(1.-par1*par3*geomStepLength)/par3))/par1 ;
      else 
        tPathLength = currentRange;
    }  
  }
  if(tPathLength < geomStepLength) tPathLength = geomStepLength;

  return tPathLength;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4double G4UrbanMscModel90::ComputeTheta0(G4double trueStepLength,
                                        G4double KineticEnergy)
{
  // for all particles take the width of the central part
  //  from a  parametrization similar to the Highland formula
  // ( Highland formula: Particle Physics Booklet, July 2002, eq. 26.10)
  const G4double c_highland = 13.6*MeV ;
  G4double betacp = sqrt(currentKinEnergy*(currentKinEnergy+2.*mass)*
                         KineticEnergy*(KineticEnergy+2.*mass)/
                      ((currentKinEnergy+mass)*(KineticEnergy+mass)));
  G4double y = trueStepLength/currentRadLength;
  G4double theta0 = c_highland*std::abs(charge)*sqrt(y)/betacp;
           y = log(y);
           theta0 *= sqrt(1.+y*(0.105+0.0035*y));

  //correction for small Zeff (based on high energy
  // proton scattering  data)
  // see G.Shen at al. Phys.Rev.D20(1979) p.1584
  theta0 *= 1.-0.24/(Zeff*(Zeff+1.));

  return theta0;

}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4ThreeVector& 
G4UrbanMscModel90::SampleScattering(const G4ThreeVector& oldDirection,
				    G4double safety)
{
  fDisplacement.set(0.0,0.0,0.0);
  G4double kineticEnergy = currentKinEnergy;
  if (tPathLength > currentRange*dtrl) {
    kineticEnergy = GetEnergy(particle,currentRange-tPathLength,couple);
  } else {
    kineticEnergy -= tPathLength*GetDEDX(particle,currentKinEnergy,couple);
  }
  if((kineticEnergy <= eV) || (tPathLength <= tlimitminfix) ||
     (tPathLength/tausmall < lambda0) ) { return fDisplacement; }

  G4double cth  = SampleCosineTheta(tPathLength,kineticEnergy);
  // protection against 'bad' cth values
  if(std::fabs(cth) > 1.) { return fDisplacement; }

  const G4double checkEnergy = GeV;
  if(kineticEnergy > checkEnergy && cth < 0.0 
     && tPathLength < taulim*lambda0) {
    G4ExceptionDescription ed;
    ed << particle->GetParticleName()
       << " E(MeV)= " << kineticEnergy/MeV
       << " Step(mm)= " << tPathLength/mm
       << " in " << CurrentCouple()->GetMaterial()->GetName()
       << " CosTheta= " << cth 
       << " is too big - the angle is set to zero" << G4endl;
    G4Exception("G4UrbanMscModel90::SampleScattering","em0004",JustWarning,
                ed,"");
    return fDisplacement;
  }

  G4double sth  = sqrt((1.0 - cth)*(1.0 + cth));
  G4double phi  = twopi*G4UniformRand();
  G4double dirx = sth*cos(phi);
  G4double diry = sth*sin(phi);

  // G4ThreeVector oldDirection = dynParticle->GetMomentumDirection();
  G4ThreeVector newDirection(dirx,diry,cth);
  newDirection.rotateUz(oldDirection);
  fParticleChange->ProposeMomentumDirection(newDirection);

  if (latDisplasment && safety > tlimitminfix) {

    G4double r = SampleDisplacement();
/*
    G4cout << "G4UrbanMscModel90::SampleSecondaries: e(MeV)= " << kineticEnergy
	   << " sinTheta= " << sth << " r(mm)= " << r
           << " trueStep(mm)= " << truestep 
           << " geomStep(mm)= " << zPathLength
           << G4endl;
*/
    if(r > 0.)
      {
        G4double latcorr = LatCorrelation();
        if(latcorr > r) latcorr = r;

        // sample direction of lateral displacement
        // compute it from the lateral correlation
        G4double Phi = 0.;
        if(std::abs(r*sth) < latcorr) {
          Phi  = twopi*G4UniformRand();
        } else {
          G4double psi = std::acos(latcorr/(r*sth));
          if(G4UniformRand() < 0.5) Phi = phi+psi;
          else                      Phi = phi-psi;
	}

        dirx = std::cos(Phi);
        diry = std::sin(Phi);

        fDisplacement.set(r*dirx,r*diry,0.0);
        fDisplacement.rotateUz(oldDirection);
      }
  }
  return fDisplacement;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4double G4UrbanMscModel90::SampleCosineTheta(G4double trueStepLength,
					    G4double KineticEnergy)
{
  G4double cth = 1. ;
  G4double tau = trueStepLength/lambda0 ;

  Zeff = couple->GetMaterial()->GetTotNbOfElectPerVolume()/
         couple->GetMaterial()->GetTotNbOfAtomsPerVolume() ;

  if(insideskin)
  {
    //no scattering, single or plural scattering
    G4double mean = trueStepLength/stepmin ;

    G4int n = G4Poisson(mean);
    if(n > 0)
    {
      G4double tm = KineticEnergy/electron_mass_c2;
      // ascr - screening parameter
      G4double ascr = exp(log(Zeff)/3.)/(137.*sqrt(tm*(tm+2.)));
      G4double ascr1 = 1.+0.5*ascr*ascr;
      G4double bp1=ascr1+1.;
      G4double bm1=ascr1-1.;
      // single scattering from screened Rutherford x-section
      G4double ct,st,phi;
      G4double sx=0.,sy=0.,sz=0.;
      for(G4int i=1; i<=n; i++)
      {
        ct = ascr1-bp1*bm1/(2.*G4UniformRand()+bm1);
        if(ct < -1.) ct = -1.;
        if(ct >  1.) ct =  1.; 
        st = sqrt(1.-ct*ct);
        phi = twopi*G4UniformRand();
        sx += st*cos(phi);
        sy += st*sin(phi);
        sz += ct;
      }
        cth = sz/sqrt(sx*sx+sy*sy+sz*sz);
    }
  }
  else
  {
    if(trueStepLength >= currentRange*dtrl) {
      if(par1*trueStepLength < 1.)
	tau = -par2*log(1.-par1*trueStepLength) ;
      // for the case if ioni/brems are inactivated
      // see the corresponding condition in ComputeGeomPathLength 
      else if(1.-KineticEnergy/currentKinEnergy > taulim)
	tau = taubig ;
    }
    currentTau = tau ;
    lambdaeff = trueStepLength/currentTau;
    currentRadLength = couple->GetMaterial()->GetRadlen();

    if (tau >= taubig) cth = -1.+2.*G4UniformRand();
    else if (tau >= tausmall)
    {
      G4double b,bx,b1,ebx,eb1;
      G4double prob = 0., qprob = 1. ;
      G4double a = 1., ea = 0., eaa = 1.;
      G4double xmean1 = 1., xmean2 = 0.;
      G4double xsi = 3.;

      G4double theta0 = ComputeTheta0(trueStepLength,KineticEnergy);

      // protexction for very small angles
      if(theta0*theta0 < tausmall) return cth;

      G4double sth = sin(0.5*theta0);
      a = 0.25/(sth*sth);

      G4double xmeanth = exp(-tau);

      G4double c = 3. ;         
      G4double c1 = c-1.;

      G4double x0 = 1.-xsi/a ;
      if(x0 < 0.)
      {
        // 1 model function
        b = exp(tau);
        bx = b-1.;
        b1 = b+1.;
        ebx=exp((c1)*log(bx)) ;
        eb1=exp((c1)*log(b1)) ;
      }
      else
      {
        //empirical tail parameter 
        // based some exp. data
        c = 2.40-0.027*exp(2.*log(Zeff)/3.);

        if(c == 2.) c = 2.+taulim ;
        if(c <= 1.) c = 1.+taulim ;
        c1 = c-1.;

        ea = exp(-xsi) ;
        eaa = 1.-ea ;
        xmean1 = 1.-(1.-(1.+xsi)*ea)/(eaa*a) ; 

        // from the continuity of the 1st derivative at x=x0
        b = 1.+(c-xsi)/a ;

        b1 = b+1. ;
        bx = c/a ;
        eb1=exp((c1)*log(b1)) ;
        ebx=exp((c1)*log(bx)) ;
        xmean2 = (x0*eb1+ebx-(eb1*bx-b1*ebx)/(c-2.))/(eb1-ebx) ;

        G4double f1x0 = a*ea/eaa ;
        G4double f2x0 = c1*eb1*ebx/(eb1-ebx)/exp(c*log(bx)) ;

        // from continuity at x=x0
        prob = f2x0/(f1x0+f2x0) ;

        // from xmean = xmeanth
        qprob = (f1x0+f2x0)*xmeanth/(f2x0*xmean1+f1x0*xmean2) ;
      }

      // sampling of costheta
      if (G4UniformRand() < qprob)
      {
        if (G4UniformRand() < prob)
           cth = 1.+log(ea+G4UniformRand()*eaa)/a ;
        else
           cth = b-b1*bx/exp(log(ebx-G4UniformRand()*(ebx-eb1))/c1) ;
      }
      else
      {
        cth = -1.+2.*G4UniformRand();
      }
    }
  }  

  return cth ;
}
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4double G4UrbanMscModel90::SampleDisplacement()
{
  const G4double kappa = 2.5;
  const G4double kappapl1 = kappa+1.;
  const G4double kappami1 = kappa-1.;
  G4double rmean = 0.0;
  if ((currentTau >= tausmall) && !insideskin) {
    if (currentTau < taulim) {
      rmean = kappa*currentTau*currentTau*currentTau*
             (1.-kappapl1*currentTau*0.25)/6. ;

    } else {
      G4double etau = 0.0;
      if (currentTau<taubig) etau = exp(-currentTau);
      rmean = -kappa*currentTau;
      rmean = -exp(rmean)/(kappa*kappami1);
      rmean += currentTau-kappapl1/kappa+kappa*etau/kappami1;
    }
    if (rmean>0.) rmean = 2.*lambdaeff*sqrt(rmean/3.0);
    else          rmean = 0.;
  }

  // protection against z > t ...........................
  if(rmean > 0.) {
    G4double zt = (tPathLength-zPathLength)*(tPathLength+zPathLength);
    if(zt <= 0.)
      rmean = 0.;
    else if(rmean*rmean > zt)
      rmean = sqrt(zt);
  }
  return rmean;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4double G4UrbanMscModel90::LatCorrelation()
{
  const G4double kappa = 2.5;
  const G4double kappami1 = kappa-1.;

  G4double latcorr = 0.;
  if((currentTau >= tausmall) && !insideskin)
  {
    if(currentTau < taulim)
      latcorr = lambdaeff*kappa*currentTau*currentTau*
                (1.-(kappa+1.)*currentTau/3.)/3.;
    else
    {
      G4double etau = 0.;
      if(currentTau < taubig) etau = exp(-currentTau);
      latcorr = -kappa*currentTau;
      latcorr = exp(latcorr)/kappami1;
      latcorr += 1.-kappa*etau/kappami1 ;
      latcorr *= 2.*lambdaeff/3. ;
    }
  }

  return latcorr;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void G4UrbanMscModel90::SampleSecondaries(std::vector<G4DynamicParticle*>*,
					  const G4MaterialCutsCouple*,
					  const G4DynamicParticle*,
					  G4double,
					  G4double)
{}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
