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
//
// **********************************************************************
// *                                                                    *
// *                    GEANT 4 xray_telescope advanced example         *
// *                                                                    *
// * MODULE:            XrayTel.cc                            main file *     
// * -------                                                            *
// *                                                                    *
// * Version:           0.4                                             *
// * Date:              06/11/00                                        *
// * Author:            R Nartallo                                      *
// * Organisation:      ESA/ESTEC, Noordwijk, THe Netherlands           *
// *                                                                    *
// **********************************************************************
//
// HISTORY
// -------
//
// The development of this advanced example is based on earlier work 
// carried out by a team of Geant4 collaborators  to simulate the Chandra
// and XMM X-ray observatories. The authors involved in those models are
// J Apostolakis, P Arce, S Giani, F Lei, R Nartallo, S Magni,
// P Truscott, L Urban
//
// **********************************************************************
// 
// CHANGE HISTORY
// --------------
//
// 05.12.2001 R. Nartallo
// -Removed XM vis option
//
// 07.11.2001 M.G. Pia
// - Modified the analysis management
// - Small design iteration
//
// 30.11.2000 R. Nartallo, A. Pfeiffer
// - Implementation of analysis manager code for histograming
//
// 15.11.2000 R. Nartallo
// - Minor changes proposed by F. Lei to implement the GPS module now 
//   replacing the standard particle gun 
// - Remove commented lines related to old histograming code
//
// 06.11.2000 R.Nartallo
// - First implementation of X-ray Telescope advanced example.
// - Based on Chandra and XMM models
// - Lines for using GAG and the histogram manager are commented out.
//
//
// **********************************************************************

#include "G4RunManager.hh"
#include "G4UImanager.hh"
#include "XrayTelDetectorConstruction.hh"
#include "XrayTelPhysicsList.hh"

#ifdef G4VIS_USE
  #include "G4VisExecutive.hh"
#endif

#ifdef G4UI_USE
  #include "G4UIExecutive.hh"
#endif

#include "XrayTelRunAction.hh"
#include "XrayTelSteppingAction.hh"
#include "XrayTelPrimaryGeneratorAction.hh"
//#include <iostream.h>
#include <vector>

int main( int argc, char** argv )
{
  // Construct the default run manager
  G4RunManager * runManager = new G4RunManager;

  // set mandatory initialization classes
  runManager->SetUserInitialization(new XrayTelDetectorConstruction ) ;
  runManager->SetUserInitialization(new XrayTelPhysicsList);

  // set mandatory user action class
  runManager->SetUserAction(new XrayTelPrimaryGeneratorAction);
  runManager->SetUserAction(new XrayTelRunAction);
  runManager->SetUserAction(new XrayTelSteppingAction);

#ifdef G4VIS_USE
  // visualization manager
  G4VisManager* visManager = new G4VisExecutive;
  visManager->Initialize();    
#endif

  //Initialize G4 kernel
  runManager->Initialize();
    
  // get the pointer to the User Interface manager 
  G4UImanager *UImanager = G4UImanager::GetUIpointer();  
  if ( argc==1 ){
#ifdef G4UI_USE
      G4UIExecutive* ui = new G4UIExecutive(argc, argv);
      ui->SessionStart();
      delete ui;
#endif
  }
  else {
    // Create a pointer to the User Interface manager 
    G4String command = "/control/execute ";
    for (int i=2; i<=argc; i++) {
      G4String macroFileName = argv[i-1];
      UImanager->ApplyCommand(command+macroFileName);
    }
  }                                  

  // job termination
#ifdef G4VIS_USE
  delete visManager;
#endif
  delete runManager;
  return 0;
}
