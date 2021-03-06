// Standard library includes.
#include <string>
#include <sstream>
#include <stdlib.h>
using std::string ;
                                                                                
// Loci includes.
#include <Loci.h>

// Fluid physics library includes.
#include "eos.h"
using fluidPhysics::EOS ;

// StreamUns includes.
#include "cavitationInitialCondition.h"
#include "sciTypes.h"
                                                                                
namespace streamUns {

  // Rule to read the initial condition for Alpha from a restart file.
  class AlphaInitialConditionRestart : public pointwise_rule {
    private:
      const_param<CavitationInitialCondition> cavitationInitialCondition ;
      const_param<string> restartNum ;
      store<real> Alpha_ic ;
    public:
                                                                                
      // Define input and output.
      AlphaInitialConditionRestart() {
        name_store("cavitationInitialCondition",cavitationInitialCondition) ;
        name_store("restartNum",restartNum) ;
        name_store("restart::Alpha_ic",Alpha_ic) ;
        input("cavitationInitialCondition,restartNum") ;
        output("restart::Alpha_ic") ;
        constraint("restart,geom_cells") ;
        disable_threading() ;
      }

      // Assign Alpha for a single cell.
      void calculate(Entity cell) { Alpha_ic[cell]=cavitationInitialCondition->alpha() ; }
                                                                                
      // Read the Alpha values from the file restart/Alpha.hdf5
      void compute(const sequence &seq) {
        ostringstream oss ; oss << "restart/Alpha_hdf5." << *restartNum ;
        string fileName=oss.str() ;
        ifstream test(fileName.c_str()) ;
        if(test){
          test.close() ;
          if(Loci::MPI_rank==0) cout << "Reading Alpha initial "
            << "condition from restart file: " << fileName << endl ;
          entitySet dom=entitySet(seq) ;
          hid_t fileID=Loci::hdf5OpenFile(fileName.c_str(),H5F_ACC_RDONLY,
            H5P_DEFAULT);
          Loci::readContainer(fileID,"Alpha",Alpha_ic.Rep(),dom) ;
          Loci::hdf5CloseFile(fileID) ;
        }else{
          if(Loci::MPI_rank==0) cout << "WARNING: No Alpha restart file. Using "
            << "Alpha initial condition value from .vars file." << endl ;

          // Check that Alpha was provided by the user.
          if(!cavitationInitialCondition->IsAlphaDefined())
            cerr << "ERROR: Initial condition for Alpha required!" << endl ;
                                                                                
          // Loop through the sequence of cells.
          do_loop(seq,this) ;
        }
      }
  } ;
                                                                                
  register_rule<AlphaInitialConditionRestart>
    registerAlphaInitialConditionRestart ;

  // Class for writing scalar restart data.
  class RestartScalar : public pointwise_rule {
    private:
      string variableName,constraintName ;
      const_param<int> timeStepNum ;
      const_store<real> variable ;
      const_param<int> restart_modulo ;
      param<bool> OUTPUT ;
    public:

      // Define input and output.
      RestartScalar(const char *variableName,const char *constraintName) :
        variableName(variableName),constraintName(constraintName) {
        string fullVariableName=variableName+string("{n,it}") ;
        string fullConstraintName=constraintName+string("{n,it}") ;
        name_store("ncycle{n}",timeStepNum) ;
        name_store(fullVariableName,variable) ;
        name_store("restart_modulo{n,it}",restart_modulo) ;
        name_store("OUTPUT{n,it}",OUTPUT) ;
        input(fullVariableName) ;
        input("ncycle{n},restart_modulo{n,it}") ;
        output("OUTPUT{n,it}") ;
        conditional("do_restart{n,it}") ;
        constraint("geom_cells{n,it}") ;
        constraint(fullConstraintName) ;
      }

      // Write the cell data to the restart file. Note the use of a temporary
      // store<real> to ensure that we only write out data for geom_cells. This
      // is required since .Rep() method that writes out the data writes out
      // all the data including periodic cells.
      void compute(const sequence &seq) {
        unsigned int fileExtension=(*restart_modulo!=0)? (*timeStepNum)%
          (*restart_modulo):(*timeStepNum) ;
        ostringstream oss ; oss << "restart/" << variableName << "_hdf5."
          << fileExtension ;
        string fileName=oss.str() ;
        if(Loci::MPI_rank==0) cout << "Writing restart file: " << fileName
          << endl ;
        hid_t file_id=Loci::hdf5CreateFile(fileName.c_str(),H5F_ACC_TRUNC,
          H5P_DEFAULT, H5P_DEFAULT) ;
        store<real> tempVariable ; tempVariable.allocate(entitySet(seq)) ;
        for(sequence::const_iterator si=seq.begin();si!=seq.end();++si)
          tempVariable[*si]=variable[*si] ;
        Loci::writeContainer(file_id,variableName,tempVariable.Rep()) ;
        Loci::hdf5CloseFile(file_id) ;
      }
  } ;

// Macro which is used to create a restart output class for each scalar.
#define RESTART_SCALAR(X,Y) class Restart_##X : public RestartScalar {\
                              public:\
                                Restart_##X() : RestartScalar(#X,#Y){}\
                            } ;\
                            register_rule<Restart_##X> registerRestart_##X ;

  // Create the scalar restart classes. For scalars that have no contraint,
  // just pass in UNIVERSE.
  RESTART_SCALAR(Alpha,cavitationModel) ;

}
