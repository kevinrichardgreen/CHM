

module cppwrap_summa


  !======= Inclusions ===========
  use, intrinsic :: iso_c_binding
  USE nrtype
  use data_types

  ! privacy
  implicit none
  public::SolveCoupledEM


contains

  ! **********************************************************************************************************
  ! public subroutine SolveCoupledEM: solving coupled energy-mass equations for one timestep
  ! **********************************************************************************************************
  subroutine SolveCoupledEM(&
  							dt_init,			 & ! intent(in):    initial time step
  							veg_fluxflag,		 & ! intent(in):    flag to indicate if we are computing fluxes over vegetation
                 			! data structures (input)
                 			handle_type,         & ! intent(in):    local classification of soil veg etc. for each HRU
                 			handle_attr,         & ! intent(in):    local attributes for each HRU
                 			handle_forc,         & ! intent(in):    model forcing data
                 			handle_mpar,         & ! intent(in):    model parameters
                 			handle_bvar,         & ! intent(in):    basin-average model variables
                 			! data structures (input-output)
                 			handle_indx,         & ! intent(inout): model indices
                 			handle_prog,         & ! intent(inout): model prognostic variables for a local HRU
                 			handle_diag,         & ! intent(inout): model diagnostic variables for a local HRU
                 			handle_flux,         & ! intent(inout): model fluxes for a local HRU
  							err					 & ! intent(out): error control
  							) bind(C,name='SolveCoupledEM')
  
  use coupled_em_module,only:coupled_em

    !======= Declarations =========
    implicit none

    ! calling variables
    real(c_double), intent(inout)		   :: dt_init
    integer(c_int), intent(inout)		   :: veg_fluxflag
    type(c_ptr), intent(in), value         :: handle_type
    type(c_ptr), intent(in), value		   :: handle_attr 
    type(c_ptr), intent(in), value		   :: handle_forc  
    type(c_ptr), intent(in), value		   :: handle_mpar
    type(c_ptr), intent(in), value		   :: handle_bvar
    type(c_ptr), intent(in), value		   :: handle_indx 
    type(c_ptr), intent(in), value		   :: handle_prog
    type(c_ptr), intent(in), value		   :: handle_diag
    type(c_ptr), intent(in), value		   :: handle_flux  
    integer(c_int), intent(out)			   :: err    
    
    ! local variables
    type(var_i), pointer				   :: type_data
    type(var_d), pointer				   :: attr_data
    type(var_d), pointer				   :: forc_data
    type(var_dlength), pointer			   :: mpar_data
    type(var_dlength), pointer			   :: bvar_data 
    type(var_ilength), pointer			   :: indx_data
    type(var_dlength), pointer			   :: prog_data 
    type(var_dlength), pointer			   :: diag_data 
    type(var_dlength), pointer			   :: flux_data  
    logical(lgt)						   :: computeVegFlux
    character(len=256)                     :: message
    integer(8)							   :: hruId

    
    ! getting data
    call c_f_pointer(handle_type, type_data) 
    call c_f_pointer(handle_attr, attr_data)
    call c_f_pointer(handle_forc, forc_data)
    call c_f_pointer(handle_mpar, mpar_data)
    call c_f_pointer(handle_bvar, bvar_data)
    call c_f_pointer(handle_indx, indx_data)
    call c_f_pointer(handle_prog, prog_data)
    call c_f_pointer(handle_diag, diag_data)
    call c_f_pointer(handle_flux, flux_data)
    
    if(veg_fluxflag == 0)then; computeVegFlux = .false.; else; computeVegFlux = .true.; endif
    
    hruId = 1001
    
 	call coupled_em(&
                 ! model control
                 hruId,             & ! intent(in):    hruId
                 dt_init,           & ! intent(inout): initial time step
                 computeVegFlux,    & ! intent(inout): flag to indicate if we are computing fluxes over vegetation
                 ! data structures (input)
                 type_data,         & ! intent(in):    local classification of soil veg etc. for each HRU
                 attr_data,         & ! intent(in):    local attributes for each HRU
                 forc_data,         & ! intent(in):    model forcing data
                 mpar_data,         & ! intent(in):    model parameters
                 bvar_data,         & ! intent(in):    basin-average model variables
                 ! data structures (input-output)
                 indx_data,         & ! intent(inout): model indices
                 prog_data,         & ! intent(inout): model prognostic variables for a local HRU
                 diag_data,         & ! intent(inout): model diagnostic variables for a local HRU
                 flux_data,         & ! intent(inout): model fluxes for a local HRU
                 ! error control
                 err,message)       ! intent(out): error control
    

 end subroutine SolveCoupledEM


end module cppwrap_summa
