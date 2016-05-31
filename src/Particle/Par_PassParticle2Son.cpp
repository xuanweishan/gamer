#include "Copyright.h"
#include "GAMER.h"

#ifdef PARTICLE




//-------------------------------------------------------------------------------------------------------
// Function    :  Par_PassParticle2Son
// Description :  Pass particles from father to sons 
//
// Note        :  1. After calling this function, father patch will have no particles (NPar == 0)
//                   --> Particles should always reside in "leaf" patches
//                2. This function should always be called after new son patches are allocated
//                   --> Invoked by "Init_Refine, Refine, & LB_Refine_AllocateNewPatch"
//                3. This function can also be used even when son patches already have particles
//                   --> The case where particles just cross a coarse-fine boundary (from coarse to fine)
//                   --> Invoked by "Par_PassParticle2Sibling_AllPatchAtThisLevel", which is invoked
//                       in EvolveLevel after the velocity correction in KDK
//
// Parameter   :  FaLv  : Father's refinement level
//                FaPID : Father's patch ID
//-------------------------------------------------------------------------------------------------------
void Par_PassParticle2Son( const int FaLv, const int FaPID )
{

   const int NPar    = amr->patch[0][FaLv][FaPID]->NPar;
   const int SonPID0 = amr->patch[0][FaLv][FaPID]->son;
   const int SonLv   = FaLv + 1;


// nothing to do if father has no son or no home particles
   if ( SonPID0 == -1  ||  NPar == 0  ||  FaLv == TOP_LEVEL )     return;


// 1. allocate the new particle list for each son
   long *NewListForSon[8]; 
   int   NNewForSon[8];
   for (int LocalID=0; LocalID<8; LocalID++)    
   {
      NewListForSon[LocalID] = new long [NPar];    // this is the maximum array size required
      NNewForSon   [LocalID] = 0;
   }


// 2. find the home patches at SonLv for all particles
   const int     Octant[2][2][2] = {  { {0,1},{2,4} }, { {3,6},{5,7} }  };    // LocalID of sons at different octants
   const double *FaCen           = amr->patch[0][SonLv][SonPID0+7]->EdgeL;    // central coordinates of FaPID
   const real   *ParPos[3]       = { amr->Par->PosX, amr->Par->PosY, amr->Par->PosZ };
   long ParID;
   int  ijk[3], LocalID;

   for (int p=0; p<NPar; p++)
   {
      ParID = amr->patch[0][FaLv][FaPID]->ParList[p];

      for (int d=0; d<3; d++)    ijk[d] = ( ParPos[d][ParID] < FaCen[d] ) ? 0 : 1;

      LocalID = Octant[ ijk[2] ][ ijk[1] ][ ijk[0] ];
      NewListForSon[LocalID][ NNewForSon[LocalID] ++ ] = ParID;
   } // for (int p=0; p<NPar; p++)


// 3. pass particles to each son
   int SonPID;
   for (int LocalID=0; LocalID<8; LocalID++)
   {
      SonPID = SonPID0 + LocalID;

//###NOTE : No OpenMP since AddParticle will modify amr->Par->NPar_Lv[]
#     ifdef DEBUG_PARTICLE
      amr->patch[0][SonLv][SonPID]->AddParticle( NNewForSon[LocalID], NewListForSon[LocalID], &amr->Par->NPar_Lv[SonLv],
                                                 ParPos, amr->Par->NPar, __FUNCTION__ );
#     else
      amr->patch[0][SonLv][SonPID]->AddParticle( NNewForSon[LocalID], NewListForSon[LocalID], &amr->Par->NPar_Lv[SonLv] );
#     endif
   }


// 4. remove particles in the father patch
//###NOTE : No OpenMP since RemoveParticle will modify amr->Par->NPar_Lv[]
   const bool RemoveAllParticle = true;
   amr->patch[0][FaLv][FaPID]->RemoveParticle( 0, NULL, &amr->Par->NPar_Lv[FaLv], RemoveAllParticle );


// free memory
   for (int LocalID=0; LocalID<8; LocalID++)    delete [] NewListForSon[LocalID];

} // FUNCTION : Par_PassParticle2Son



#endif // #ifdef PARTICLE