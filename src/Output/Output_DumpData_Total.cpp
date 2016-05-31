#include "Copyright.h"
#include "GAMER.h"
#include "CUFLU.h"
#ifdef GRAVITY
#include "CUPOT.h"
#endif



/*======================================================================================================
Procedure for outputting new variables:
1. Add the new variables into one of the header sections
2. Add the corresponding (i) fread (ii) CompareVar in "Init_Restart_v2/Load_Parameter_After_2000"
======================================================================================================*/




//-------------------------------------------------------------------------------------------------------
// Function    :  Output_DumpData_Total
// Description :  Output all simulation data in the binary form, which can be used as a restart file
//
// Parameter   :  FileName : Name of the output file
//-------------------------------------------------------------------------------------------------------
void Output_DumpData_Total( const char *FileName )
{  

// output data in the HDF5 format
#  ifdef SUPPORT_HDF5
   if ( OPT__OUTPUT_TOTAL == OUTPUT_FORMAT_HDF5 )
   {
      Output_DumpData_Total_HDF5( FileName );
      return;
   }
#  endif


   if ( MPI_Rank == 0 )    Aux_Message( stdout, "%s (DumpID = %d) ...\n", __FUNCTION__, DumpID );


// check the synchronization
   for (int lv=1; lv<NLEVEL; lv++)
      if ( NPatchTotal[lv] != 0 )   Mis_CompareRealValue( Time[0], Time[lv], __FUNCTION__, true );


// check if the targeted file already exists
   if ( Aux_CheckFileExist(FileName)  &&  MPI_Rank == 0 )
      Aux_Message( stderr, "WARNING : file \"%s\" already exists and will be overwritten !!\n", FileName );


// get the total number of patches that have no son
   int NDataPatch_Local[NLEVEL] = { 0 };
   int NDataPatch_Total[NLEVEL];

   for (int lv=0; lv<NLEVEL; lv++)    
   for (int PID=0; PID<amr->NPatchComma[lv][1]; PID++) 
   {
      if ( amr->patch[0][lv][PID]->son == -1 )  NDataPatch_Local[lv] ++;
   }

   MPI_Reduce( NDataPatch_Local, NDataPatch_Total, NLEVEL, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD );


   FILE *File;

   if ( MPI_Rank == 0 )
   {
      File = fopen( FileName, "wb" );


//    check the size of different data types
//    =================================================================================================
      const int size_bool   = sizeof( bool   );
      const int size_int    = sizeof( int    );
      const int size_uint   = sizeof( uint   );
      const int size_long   = sizeof( long   );
      const int size_ulong  = sizeof( ulong  );
      const int size_real   = sizeof( real   );
      const int size_double = sizeof( double );

      if ( size_int != size_uint )
         Aux_Error( ERROR_INFO, "sizeof(int) = %d != sizeof(uint) = %d !!\n", size_int, size_uint );

      if ( size_long != size_ulong )
         Aux_Error( ERROR_INFO, "sizeof(long) = %d != sizeof(ulong) = %d !!\n", size_long, size_ulong );


//    set the size of different headers (in bytes)
//    =================================================================================================
      const long HeaderSize_Format      =  256;
      const long HeaderSize_Makefile    =  256;
      const long HeaderSize_Constant    =  512;
      const long HeaderSize_Parameter   = 1024;
      const long HeaderSize_SimuInfo    = 1024;

      const long HeaderOffset_Format    = 0;    // it must be zero
      const long HeaderOffset_Makefile  = HeaderOffset_Format    + HeaderSize_Format;
      const long HeaderOffset_Constant  = HeaderOffset_Makefile  + HeaderSize_Makefile;
      const long HeaderOffset_Parameter = HeaderOffset_Constant  + HeaderSize_Constant;
      const long HeaderOffset_SimuInfo  = HeaderOffset_Parameter + HeaderSize_Parameter;

      const long HeaderSize_Total       = HeaderOffset_SimuInfo  + HeaderSize_SimuInfo;


//    fill in the entire header in advance (just for clarification)
//    =================================================================================================
      char *OutputBuf = new char [HeaderSize_Total];

      for (int t=0; t<HeaderSize_Total; t++)   OutputBuf[t] = 'b';   // 'b' is chosen arbitrarily ...

      fwrite( OutputBuf, sizeof(char), HeaderSize_Total, File );

      delete [] OutputBuf;


//    a. output the information of data format
//    =================================================================================================
      const long FormatVersion = 2001;
      const long CheckCode     = 123456789;

      fseek( File, HeaderOffset_Format, SEEK_SET );

      fwrite( &FormatVersion,             sizeof(long),                    1,             File );
      fwrite( &HeaderSize_Format,         sizeof(long),                    1,             File );
      fwrite( &HeaderSize_Makefile,       sizeof(long),                    1,             File );
      fwrite( &HeaderSize_Constant,       sizeof(long),                    1,             File );
      fwrite( &HeaderSize_Parameter,      sizeof(long),                    1,             File );
      fwrite( &HeaderSize_SimuInfo,       sizeof(long),                    1,             File );
      fwrite( &CheckCode,                 sizeof(long),                    1,             File );
      fwrite( &size_bool,                 sizeof(int),                     1,             File );
      fwrite( &size_int,                  sizeof(int),                     1,             File );
      fwrite( &size_long,                 sizeof(int),                     1,             File );
      fwrite( &size_real,                 sizeof(int),                     1,             File );
      fwrite( &size_double,               sizeof(int),                     1,             File );


//    b. output the simulation options and parameters defined in the Makefile
//    =================================================================================================
      if ( ftell(File) > HeaderOffset_Makefile )
         Aux_Error( ERROR_INFO, "Current file position (%ld) > correct offset (%ld) !!\n",
                    ftell(File), HeaderOffset_Makefile );

      fseek( File, HeaderOffset_Makefile, SEEK_SET );

#     ifdef MODEL
      const int  model               = MODEL;
#     else
      const int  model               = NULL_INT;
#     endif

#     ifdef GRAVITY
      const bool gravity             = true;
#     else
      const bool gravity             = false;
#     endif

#     ifdef POT_SCHEME
      const int  pot_scheme          = POT_SCHEME;
#     else
      const int  pot_scheme          = NULL_INT;
#     endif

#     ifdef INDIVIDUAL_TIMESTEP
      const bool individual_timestep = true;
#     else
      const bool individual_timestep = false;
#     endif

#     ifdef COMOVING
      const bool comoving            = true;
#     else
      const bool comoving            = false;
#     endif

#     ifdef FLU_SCHEME
      const int  flu_scheme          = FLU_SCHEME;
#     else
      const int  flu_scheme          = NULL_INT;
#     endif

#     ifdef LR_SCHEME
      const int  lr_scheme           = LR_SCHEME;
#     else
      const int  lr_scheme           = NULL_INT;
#     endif

#     ifdef RSOLVER
      const int  rsolver             = RSOLVER;
#     else
      const int  rsolver             = NULL_INT;
#     endif

#     ifdef GPU
      const bool gpu                 = true;
#     else
      const bool gpu                 = false;
#     endif

#     ifdef GAMER_OPTIMIZATION
      const bool gamer_optimization  = true;
#     else
      const bool gamer_optimization  = false;
#     endif

#     ifdef GAMER_DEBUG
      const bool gamer_debug         = true;
#     else
      const bool gamer_debug         = false;
#     endif

#     ifdef TIMING
      const bool timing              = true;
#     else
      const bool timing              = false;
#     endif

#     ifdef TIMING_SOLVER
      const bool timing_solver       = true;
#     else
      const bool timing_solver       = false;
#     endif

#     ifdef INTEL
      const bool intel               = true;
#     else
      const bool intel               = false;
#     endif

#     ifdef FLOAT8
      const bool float8              = true;
#     else
      const bool float8              = false;
#     endif

#     ifdef SERIAL
      const bool serial              = true;
#     else
      const bool serial              = false;
#     endif

#     ifdef LOAD_BALANCE
      const int load_balance         = LOAD_BALANCE;
#     else
      const int load_balance         = 0;
#     endif

#     ifdef OVERLAP_MPI
      const bool overlap_mpi         = true;
#     else
      const bool overlap_mpi         = false;
#     endif

#     ifdef OPENMP
      const bool openmp              = true;
#     else
      const bool openmp              = false;
#     endif

#     ifdef GPU
      const int  gpu_arch            = GPU_ARCH;
#     else
      const int  gpu_arch            = NULL_INT;
#     endif

#     ifdef STORE_POT_GHOST
      const bool store_pot_ghost     = true;
#     else
      const bool store_pot_ghost     = false;
#     endif

#     ifdef UNSPLIT_GRAVITY
      const bool unsplit_gravity     = true;
#     else
      const bool unsplit_gravity     = false;
#     endif

#     ifdef PARTICLE
      const bool particle            = true;
#     else
      const bool particle            = false;
#     endif

#     ifdef NPASSIVE
      const int npassive             = NPASSIVE;
#     else
      const int npassive             = NULL_INT;
#     endif

#     ifdef CONSERVE_MASS
      const bool conserve_mass       = true;
#     else
      const bool conserve_mass       = false;
#     endif

#     ifdef LAPLACIAN_4TH
      const bool laplacian_4th       = true;
#     else
      const bool laplacian_4th       = false;
#     endif

#     ifdef QUARTIC_SELF_INTERACTION
      const bool self_interaction    = true;
#     else
      const bool self_interaction    = false;
#     endif

#     ifdef LAOHU
      const bool laohu               = true;
#     else
      const bool laohu               = false;
#     endif

#     ifdef SUPPORT_HDF5
      const bool support_hdf5        = true;
#     else
      const bool support_hdf5        = false;
#     endif

      const int nlevel               = NLEVEL;
      const int max_patch            = MAX_PATCH;

      fwrite( &model,                     sizeof(int),                     1,             File );
      fwrite( &gravity,                   sizeof(bool),                    1,             File );
      fwrite( &pot_scheme,                sizeof(int),                     1,             File );
      fwrite( &individual_timestep,       sizeof(bool),                    1,             File );
      fwrite( &comoving,                  sizeof(bool),                    1,             File );
      fwrite( &flu_scheme,                sizeof(int),                     1,             File );
      fwrite( &lr_scheme,                 sizeof(int),                     1,             File );
      fwrite( &rsolver,                   sizeof(int),                     1,             File );
      fwrite( &gpu,                       sizeof(bool),                    1,             File );
      fwrite( &gamer_optimization,        sizeof(bool),                    1,             File );
      fwrite( &gamer_debug,               sizeof(bool),                    1,             File );
      fwrite( &timing,                    sizeof(bool),                    1,             File );
      fwrite( &timing_solver,             sizeof(bool),                    1,             File );
      fwrite( &intel,                     sizeof(bool),                    1,             File );
      fwrite( &float8,                    sizeof(bool),                    1,             File );
      fwrite( &serial,                    sizeof(bool),                    1,             File );
      fwrite( &load_balance,              sizeof(int),                     1,             File );
      fwrite( &overlap_mpi,               sizeof(bool),                    1,             File );
      fwrite( &openmp,                    sizeof(bool),                    1,             File );
      fwrite( &gpu_arch,                  sizeof(int),                     1,             File );
      fwrite( &nlevel,                    sizeof(int),                     1,             File );
      fwrite( &max_patch,                 sizeof(int),                     1,             File );
      fwrite( &store_pot_ghost,           sizeof(bool),                    1,             File );
      fwrite( &unsplit_gravity,           sizeof(bool),                    1,             File );
      fwrite( &particle,                  sizeof(bool),                    1,             File );
      fwrite( &npassive,                  sizeof(int),                     1,             File );
      fwrite( &conserve_mass,             sizeof(bool),                    1,             File );
      fwrite( &laplacian_4th,             sizeof(bool),                    1,             File );
      fwrite( &self_interaction,          sizeof(bool),                    1,             File );
      fwrite( &laohu,                     sizeof(bool),                    1,             File );
      fwrite( &support_hdf5,              sizeof(bool),                    1,             File );


//    c. output the symbolic constants defined in "Macro.h, CUPOT.h, and CUFLU.h"
//    =================================================================================================
      if ( ftell(File) > HeaderOffset_Constant )
         Aux_Error( ERROR_INFO, "Current file position (%ld) > correct offset (%ld) !!\n",
                    ftell(File), HeaderOffset_Constant );

      fseek( File, HeaderOffset_Constant, SEEK_SET );

      const int    ncomp                 = NCOMP;
      const int    patch_size            = PATCH_SIZE;
#     ifdef MIN_PRES_DENS
      const double min_value             = MIN_PRES_DENS;
#     elif defined MIN_PRES
      const double min_value             = MIN_PRES;
#     else
      const double min_value             = NULL_REAL;
#     endif
      const int    flu_ghost_size        = FLU_GHOST_SIZE;

#     ifdef GRAVITY
      const int    pot_ghost_size        = POT_GHOST_SIZE; 
      const int    gra_ghost_size        = GRA_GHOST_SIZE;
#     else
      const int    pot_ghost_size        = NULL_INT;
      const int    gra_ghost_size        = NULL_INT;
#     endif

#     if ( defined MIN_PRES  ||  defined MIN_PRES_DENS ) 
      const bool   enforce_positive      = true;
#     else
      const bool   enforce_positive      = false;
#     endif

#     ifdef CHAR_RECONSTRUCTION
      const bool   char_reconstruction   = true;
#     else
      const bool   char_reconstruction   = false;
#     endif

#     ifdef CHECK_INTERMEDIATE
      const int    check_intermediate    = CHECK_INTERMEDIATE;
#     else
      const int    check_intermediate    = NULL_INT;
#     endif

#     ifdef HLL_NO_REF_STATE
      const bool   hll_no_ref_state      = true;
#     else
      const bool   hll_no_ref_state      = false;
#     endif

#     ifdef HLL_INCLUDE_ALL_WAVES
      const bool   hll_include_all_waves = true;
#     else
      const bool   hll_include_all_waves = false;
#     endif

#     ifdef WAF_DISSIPATE
      const bool   waf_dissipate         = true;
#     else
      const bool   waf_dissipate         = false;
#     endif

#     ifdef MAX_ERROR
      const double max_error             = MAX_ERROR;
#     else
      const double max_error             = NULL_REAL;
#     endif

      const int    flu_block_size_x      = FLU_BLOCK_SIZE_X;
      const int    flu_block_size_y      = FLU_BLOCK_SIZE_Y;

#     ifdef USE_PSOLVER_10TO14
      const bool   use_psolver_10to14    = true;
#     else
      const bool   use_psolver_10to14    = false;
#     endif

#     ifdef POT_BLOCK_SIZE_X
      const int   pot_block_size_x       = POT_BLOCK_SIZE_X;
#     else
      const int   pot_block_size_x       = NULL_INT;
#     endif

#     ifdef POT_BLOCK_SIZE_Z
      const int   pot_block_size_z       = POT_BLOCK_SIZE_Z;
#     else
      const int   pot_block_size_z       = NULL_INT;
#     endif

#     ifdef GRA_BLOCK_SIZE_Z
      const int   gra_block_size_z       = GRA_BLOCK_SIZE_Z;
#     else
      const int   gra_block_size_z       = NULL_INT;
#     endif

      fwrite( &ncomp,                     sizeof(int),                     1,             File );
      fwrite( &patch_size,                sizeof(int),                     1,             File );
      fwrite( &min_value,                 sizeof(double),                  1,             File );
      fwrite( &flu_ghost_size,            sizeof(int),                     1,             File );
      fwrite( &pot_ghost_size,            sizeof(int),                     1,             File );
      fwrite( &gra_ghost_size,            sizeof(int),                     1,             File );
      fwrite( &enforce_positive,          sizeof(bool),                    1,             File );
      fwrite( &char_reconstruction,       sizeof(bool),                    1,             File );
      fwrite( &check_intermediate,        sizeof(int),                     1,             File );
      fwrite( &hll_no_ref_state,          sizeof(bool),                    1,             File );
      fwrite( &hll_include_all_waves,     sizeof(bool),                    1,             File );
      fwrite( &waf_dissipate,             sizeof(bool),                    1,             File );
      fwrite( &max_error,                 sizeof(double),                  1,             File );
      fwrite( &flu_block_size_x,          sizeof(int),                     1,             File );
      fwrite( &flu_block_size_y,          sizeof(int),                     1,             File );
      fwrite( &use_psolver_10to14,        sizeof(bool),                    1,             File );
      fwrite( &pot_block_size_x,          sizeof(int),                     1,             File );
      fwrite( &pot_block_size_z,          sizeof(int),                     1,             File );
      fwrite( &gra_block_size_z,          sizeof(int),                     1,             File );


//    d. output the simulation parameters recorded in the file "Input__Parameter"
//    *** we always typecast "enum" to "int" when outputting data to ensure the data size ***
//    =================================================================================================
      if ( ftell(File) > HeaderOffset_Parameter )
         Aux_Error( ERROR_INFO, "Current file position (%ld) > correct offset (%ld) !!\n",
                    ftell(File), HeaderOffset_Parameter );

      fseek( File, HeaderOffset_Parameter, SEEK_SET );

      const int    mpi_nrank               = MPI_NRank;
      const int    mpi_nrank_x[3]          = { MPI_NRank_X[0], MPI_NRank_X[1], MPI_NRank_X[2] };    
      const int    opt__output_total       = (int)OPT__OUTPUT_TOTAL;
      const int    opt__output_part        = (int)OPT__OUTPUT_PART;
      const int    opt__output_mode        = (int)OPT__OUTPUT_MODE;
      const int    opt__flu_int_scheme     = (int)OPT__FLU_INT_SCHEME;
      const int    opt__ref_flu_int_scheme = (int)OPT__REF_FLU_INT_SCHEME;

#     ifndef COMOVING
      const double OMEGA_M0                = NULL_REAL;
      const double DT__MAX_DELTA_A         = NULL_REAL;
#     endif

#     ifdef GRAVITY
      const int    opt__pot_int_scheme     = (int)OPT__POT_INT_SCHEME;
      const int    opt__rho_int_scheme     = (int)OPT__RHO_INT_SCHEME;
      const int    opt__gra_int_scheme     = (int)OPT__GRA_INT_SCHEME;
      const int    opt__ref_pot_int_scheme = (int)OPT__REF_POT_INT_SCHEME;
#     else
      const double DT__GRAVITY             = NULL_REAL;
      const double NEWTON_G                = NULL_REAL;
      const int    POT_GPU_NPGROUP         = NULL_INT;
      const bool   OPT__OUTPUT_POT         = NULL_BOOL;
      const bool   OPT__GRA_P5_GRADIENT    = NULL_BOOL;
      const double SOR_OMEGA               = NULL_REAL;
      const int    SOR_MAX_ITER            = NULL_INT;
      const int    SOR_MIN_ITER            = NULL_INT;
      const double MG_TOLERATED_ERROR      = NULL_REAL;
      const int    MG_MAX_ITER             = NULL_INT;
      const int    MG_NPRE_SMOOTH          = NULL_INT;
      const int    MG_NPOST_SMOOTH         = NULL_INT;
      const int    opt__pot_int_scheme     = NULL_INT;
      const int    opt__rho_int_scheme     = NULL_INT;
      const int    opt__gra_int_scheme     = NULL_INT;
      const int    opt__ref_pot_int_scheme = NULL_INT;
#     endif // #ifdef GRAVITY

#     ifdef LOAD_BALANCE
      const double lb_wli_max              = amr->LB->WLI_Max;
#     else
      const double lb_wli_max              = NULL_REAL;
#     endif

#     if ( MODEL == HYDRO )
      const int    opt__lr_limiter         = (int)OPT__LR_LIMITER;
      const int    opt__waf_limiter        = (int)OPT__WAF_LIMITER;
      const int    opt__corr_unphy_scheme  = (int)OPT__CORR_UNPHY_SCHEME;
#     else
#     if ( MODEL == MHD )
#     warning : WAIT MHD !!!
#     endif
      const bool   OPT__FLAG_PRES_GRADIENT = NULL_BOOL;
      const double GAMMA                   = NULL_REAL;
      const double MINMOD_COEFF            = NULL_REAL;
      const double EP_COEFF                = NULL_REAL;
      const int    opt__lr_limiter         = NULL_INT;
      const int    opt__waf_limiter        = NULL_INT;
      const int    opt__corr_unphy_scheme  = NULL_INT;
#     endif

#     if ( MODEL != ELBDM )
      const double DT__PHASE               = NULL_REAL;
      const bool   OPT__FLAG_ENGY_DENSITY  = NULL_BOOL;
      const bool   OPT__INT_PHASE          = NULL_BOOL;
      const double ELBDM_MASS              = NULL_REAL;
      const double ELBDM_PLANCK_CONST      = NULL_REAL;
#     endif

      fwrite( &BOX_SIZE,                  sizeof(double),                  1,             File );
      fwrite(  NX0_TOT,                   sizeof(int),                     3,             File );
      fwrite( &mpi_nrank,                 sizeof(int),                     1,             File );
      fwrite(  mpi_nrank_x,               sizeof(int),                     3,             File );
      fwrite( &OMP_NTHREAD,               sizeof(int),                     1,             File );
      fwrite( &END_T,                     sizeof(double),                  1,             File );
      fwrite( &END_STEP,                  sizeof(long),                    1,             File );
      fwrite( &OMEGA_M0,                  sizeof(double),                  1,             File );
      fwrite( &DT__FLUID,                 sizeof(double),                  1,             File );
      fwrite( &DT__GRAVITY,               sizeof(double),                  1,             File );
      fwrite( &DT__PHASE,                 sizeof(double),                  1,             File );
      fwrite( &DT__MAX_DELTA_A,           sizeof(double),                  1,             File );
      fwrite( &OPT__ADAPTIVE_DT,          sizeof(bool),                    1,             File );
      fwrite( &OPT__DT_USER,              sizeof(bool),                    1,             File );
      fwrite( &REGRID_COUNT,              sizeof(int),                     1,             File );
      fwrite( &FLAG_BUFFER_SIZE,          sizeof(int),                     1,             File );
      fwrite( &MAX_LEVEL,                 sizeof(int),                     1,             File );
      fwrite( &OPT__FLAG_RHO,             sizeof(bool),                    1,             File );
      fwrite( &OPT__FLAG_RHO_GRADIENT,    sizeof(bool),                    1,             File );
      fwrite( &OPT__FLAG_PRES_GRADIENT,   sizeof(bool),                    1,             File );
      fwrite( &OPT__FLAG_ENGY_DENSITY,    sizeof(bool),                    1,             File );
      fwrite( &OPT__FLAG_USER,            sizeof(bool),                    1,             File );
      fwrite( &lb_wli_max,                sizeof(double),                  1,             File );
      fwrite( &GAMMA,                     sizeof(double),                  1,             File );
      fwrite( &MINMOD_COEFF,              sizeof(double),                  1,             File );
      fwrite( &EP_COEFF,                  sizeof(double),                  1,             File );
      fwrite( &opt__lr_limiter,           sizeof(int),                     1,             File );
      fwrite( &opt__waf_limiter,          sizeof(int),                     1,             File );
      fwrite( &ELBDM_MASS,                sizeof(double),                  1,             File );
      fwrite( &ELBDM_PLANCK_CONST,        sizeof(double),                  1,             File );
      fwrite( &FLU_GPU_NPGROUP,           sizeof(int),                     1,             File );
      fwrite( &GPU_NSTREAM,               sizeof(int),                     1,             File );
      fwrite( &OPT__FIXUP_FLUX,           sizeof(bool),                    1,             File );
      fwrite( &OPT__FIXUP_RESTRICT,       sizeof(bool),                    1,             File );
      fwrite( &OPT__OVERLAP_MPI,          sizeof(bool),                    1,             File );
      fwrite( &NEWTON_G,                  sizeof(double),                  1,             File );
      fwrite( &SOR_OMEGA,                 sizeof(double),                  1,             File );
      fwrite( &SOR_MAX_ITER,              sizeof(int),                     1,             File );
      fwrite( &SOR_MIN_ITER,              sizeof(int),                     1,             File );
      fwrite( &MG_MAX_ITER,               sizeof(int),                     1,             File );
      fwrite( &MG_NPRE_SMOOTH,            sizeof(int),                     1,             File );
      fwrite( &MG_NPOST_SMOOTH,           sizeof(int),                     1,             File );
      fwrite( &MG_TOLERATED_ERROR,        sizeof(double),                  1,             File );
      fwrite( &POT_GPU_NPGROUP,           sizeof(int),                     1,             File );
      fwrite( &OPT__GRA_P5_GRADIENT,      sizeof(bool),                    1,             File );
      fwrite( &OPT__INT_TIME,             sizeof(bool),                    1,             File );
      fwrite( &OPT__INT_PHASE,            sizeof(bool),                    1,             File );
      fwrite( &opt__flu_int_scheme,       sizeof(int),                     1,             File );
      fwrite( &opt__pot_int_scheme,       sizeof(int),                     1,             File );
      fwrite( &opt__rho_int_scheme,       sizeof(int),                     1,             File );
      fwrite( &opt__gra_int_scheme,       sizeof(int),                     1,             File );
      fwrite( &opt__ref_flu_int_scheme,   sizeof(int),                     1,             File );
      fwrite( &opt__ref_pot_int_scheme,   sizeof(int),                     1,             File );
      fwrite( &opt__output_total,         sizeof(int),                     1,             File );
      fwrite( &opt__output_part,          sizeof(int),                     1,             File );
      fwrite( &OPT__OUTPUT_TEST_ERROR,    sizeof(bool),                    1,             File );
      fwrite( &OPT__OUTPUT_BASE,          sizeof(bool),                    1,             File );
      fwrite( &OPT__OUTPUT_POT,           sizeof(bool),                    1,             File );
      fwrite( &opt__output_mode,          sizeof(int),                     1,             File );
      fwrite( &OUTPUT_STEP,               sizeof(int),                     1,             File );
      fwrite( &OUTPUT_DT,                 sizeof(double),                  1,             File );
      fwrite( &OUTPUT_PART_X,             sizeof(double),                  1,             File );
      fwrite( &OUTPUT_PART_Y,             sizeof(double),                  1,             File );
      fwrite( &OUTPUT_PART_Z,             sizeof(double),                  1,             File );
      fwrite( &OPT__TIMING_BALANCE,       sizeof(bool),                    1,             File );
      fwrite( &OPT__OUTPUT_BASEPS,        sizeof(bool),                    1,             File );
      fwrite( &OPT__CORR_UNPHY,           sizeof(bool),                    1,             File );
      fwrite( &opt__corr_unphy_scheme,    sizeof(int),                     1,             File );


//    e. output the simulation information
//    =================================================================================================
      if ( ftell(File) > HeaderOffset_SimuInfo )
         Aux_Error( ERROR_INFO, "Current file position (%ld) > correct offset (%ld) !!\n",
                    ftell(File), HeaderOffset_SimuInfo );

      fseek( File, HeaderOffset_SimuInfo, SEEK_SET );

#     ifndef GRAVITY
      const double AveDensity = NULL_REAL;
#     endif

      fwrite( &CheckCode,                 sizeof(long),                    1,             File );
      fwrite( &DumpID,                    sizeof(int),                     1,             File );
      fwrite( Time,                       sizeof(double),             NLEVEL,             File );
      fwrite( &Step,                      sizeof(long),                    1,             File );
      fwrite( NPatchTotal,                sizeof(int),                NLEVEL,             File );
      fwrite( NDataPatch_Total,           sizeof(int),                NLEVEL,             File );
      fwrite( AdvanceCounter,             sizeof(long),               NLEVEL,             File );
      fwrite( &AveDensity,                sizeof(double),                  1,             File );


//    move the file position indicator to the end of the header ==> prepare to output patch data
      if ( ftell(File) > HeaderSize_Total )
         Aux_Error( ERROR_INFO, "Current file position (%ld) > correct offset (%ld) !!\n",
                    ftell(File), HeaderSize_Total );

      fseek( File, HeaderSize_Total, SEEK_SET );

      fclose( File );

   } // if ( MPI_Rank == 0 )


// f. output the simulation data
// =================================================================================================
   for (int lv=0; lv<NLEVEL; lv++)
   {
      for (int TargetMPIRank=0; TargetMPIRank<MPI_NRank; TargetMPIRank++)
      {
         if ( MPI_Rank == TargetMPIRank )
         {
            File = fopen( FileName, "ab" );

            for (int PID=0; PID<amr->NPatchComma[lv][1]; PID++)
            {
//             f1. output the patch information 
//             (the father <-> son information will be re-constructed during the restart)
               fwrite(  amr->patch[0][lv][PID]->corner, sizeof(int), 3, File );
               fwrite( &amr->patch[0][lv][PID]->son,    sizeof(int), 1, File );


//             f2. output the patch data only if it has no son
               if ( amr->patch[0][lv][PID]->son == -1 )
               {
//                f2-1. output the fluid variables
                  fwrite( amr->patch[ amr->FluSg[lv] ][lv][PID]->fluid, sizeof(real), 
                          PATCH_SIZE*PATCH_SIZE*PATCH_SIZE*NCOMP, File );

#                 ifdef GRAVITY
//                f2-2. output the gravitational potential
                  if ( OPT__OUTPUT_POT )
                  fwrite( amr->patch[ amr->PotSg[lv] ][lv][PID]->pot,   sizeof(real), 
                          PATCH_SIZE*PATCH_SIZE*PATCH_SIZE,       File );
#                 endif 
               } // if ( amr->patch[0][lv][PID]->son == -1 )
            } // for (int PID=0; PID<amr->NPatchComma[lv][1]; PID++)

            fclose( File );

         } // if ( MPI_Rank == TargetMPIRank )

         MPI_Barrier( MPI_COMM_WORLD );

      } // for (int TargetMPIRank=0; TargetMPIRank<MPI_NRank; TargetMPIRank++)
   } // for (int lv=0; lv<NLEVEL; lv++)


   if ( MPI_Rank == 0 )    Aux_Message( stdout, "%s (DumpID = %d) ... done\n", __FUNCTION__, DumpID );

} // FUNCTION : Output_DumpData_Total

