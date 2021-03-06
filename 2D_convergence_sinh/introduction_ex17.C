//iHat*u + jHat*v + kHat*w

#include <iostream>
#include <algorithm>
#include <math.h>


// Basic include file needed for the mesh functionality.
#include "libmesh.h"
#include "mesh.h"
#include "mesh_generation.h"
#include "exodusII_io.h"
#include "equation_systems.h"
#include "fe.h"
#include "quadrature_gauss.h"
#include "dof_map.h"
#include "sparse_matrix.h"
#include "numeric_vector.h"
#include "dense_matrix.h"
#include "dense_vector.h"
#include "linear_implicit_system.h"
#include "transient_system.h"
#include "dense_submatrix.h"
#include "dense_subvector.h"
#include <tecplot_io.h>
#include "error_vector.h"
#include "exact_solution.h"
#include <petsc_linear_solver.h>
#include "elem.h"
using namespace libMesh;
#include "gmsh_io.h"
#include "assemble.h"
#include <iostream>
#include <fstream>
using namespace std;




// Pointers to dimension-independent functions
Number (*exact_solution_u)(const Point& p,
  const Parameters&, const std::string&, const std::string&);
Gradient (*exact_derivative_u)(const Point& p,
  const Parameters&, const std::string&, const std::string&);

Number (*exact_solution_v)(const Point& p,
  const Parameters&, const std::string&, const std::string&);
Gradient (*exact_derivative_v)(const Point& p,
  const Parameters&, const std::string&, const std::string&);

#if THREED
Number (*exact_solution_w)(const Point& p,
  const Parameters&, const std::string&, const std::string&);
Gradient (*exact_derivative_w)(const Point& p,
  const Parameters&, const std::string&, const std::string&);
#endif

Number (*exact_solution_x)(const Point& p,
  const Parameters&, const std::string&, const std::string&);
Gradient (*exact_derivative_x)(const Point& p,
  const Parameters&, const std::string&, const std::string&);

Number (*exact_solution_y)(const Point& p,
  const Parameters&, const std::string&, const std::string&);
Gradient (*exact_derivative_y)(const Point& p,
  const Parameters&, const std::string&, const std::string&);

#if THREED
Number (*exact_solution_z)(const Point& p,
  const Parameters&, const std::string&, const std::string&);
Gradient (*exact_derivative_z)(const Point& p,
  const Parameters&, const std::string&, const std::string&);
#endif


// Pointers to dimension-independent functions
Number (*exact_solution_p)(const Point& p,
  const Parameters&, const std::string&, const std::string&);
Gradient (*exact_derivative_p)(const Point& p,
  const Parameters&, const std::string&, const std::string&);


// The main program.
int main (int argc, char** argv)
{
  
  // Initialize libMesh.
  LibMeshInit init (argc, argv);

  Mesh mesh;
  EquationSystems equation_systems (mesh);
  read_parameters(equation_systems,argc,argv);

  unsigned int n_timesteps = equation_systems.parameters.get<Real>("n_timesteps");
  unsigned int N_eles=equation_systems.parameters.get<Real>("N_eles");

  Real time     = 0;
  Real end_time     = 0.25;
  Real dt = end_time/n_timesteps;
  equation_systems.parameters.set<Real> ("dt")   = dt;
 
#if !THREED
  MeshTools::Generation::build_square (mesh,
                                       N_eles, N_eles,
                                       0., 1.,
                                       0., 1.,
                                       QUAD4);
#endif
#if THREED
  MeshTools::Generation::build_cube (mesh,
                                       N_eles, N_eles, N_eles,
                                       0., 1.,
                                       0., 1.,
                                       0., 1.,
                                       TET4);
#endif


  equation_systems.parameters.set<Real> ("dt")   = dt;

  #if EXODUS               
  ExodusII_IO exo= ExodusII_IO(mesh);
  #endif
  #if WRITE_TEC
  TecplotIO tec= TecplotIO(equation_systems.get_mesh());
  #endif
  
  TransientLinearImplicitSystem & system = 
    equation_systems.add_system<TransientLinearImplicitSystem> ("Last_non_linear_soln");

  system.add_variable ("s_u", DISP_ORDER,ELEMENT_TYPE);
  system.add_variable ("s_v", DISP_ORDER,ELEMENT_TYPE);
  #if THREED
  system.add_variable ("s_w", DISP_ORDER,ELEMENT_TYPE);
  #endif
  system.add_variable ("s_p", PRES_ORDER,ELEMENT_TYPE_PRESS);
  system.add_variable ("x", VEL_ORDER,ELEMENT_TYPE);
  system.add_variable ("y", VEL_ORDER,ELEMENT_TYPE);
  #if THREED
  system.add_variable ("z", VEL_ORDER,ELEMENT_TYPE);
  #endif

  TransientLinearImplicitSystem & result =   equation_systems.add_system<TransientLinearImplicitSystem> ("result");
  result.add_variable ("u", DISP_ORDER,ELEMENT_TYPE);
  result.add_variable ("v", DISP_ORDER,ELEMENT_TYPE);
  #if THREED
  result.add_variable ("w", DISP_ORDER,ELEMENT_TYPE);
  #endif

  TransientLinearImplicitSystem & reference =   equation_systems.add_system<TransientLinearImplicitSystem> ("reference");
  reference.add_variable ("ref_u", DISP_ORDER,ELEMENT_TYPE);
  reference.add_variable ("ref_v", DISP_ORDER,ELEMENT_TYPE);
  #if THREED
  reference.add_variable ("ref_w", DISP_ORDER,ELEMENT_TYPE);
  #endif
  equation_systems.init ();
  equation_systems.print_info();
  mesh.print_info();

  equation_systems.parameters.set<unsigned int>("linear solver maximum iterations") = 2500;
  equation_systems.parameters.set<Real>        ("linear solver tolerance") = TOLERANCE;
  
#if PETSC_MUMPS
  PetscLinearSolver<Number>* petsc_linear_solver =dynamic_cast<PetscLinearSolver<Number>*>(system.get_linear_solver());
  PC pc = petsc_linear_solver->pc();
  int ierr = PCSetType(pc, PC_TYPE);
  CHKERRABORT(libMesh::COMM_WORLD,ierr);
  ierr = PCFactorSetMatSolverPackage(pc,SOLVER_NAME);
  CHKERRABORT(libMesh::COMM_WORLD,ierr);
	
	//PetscOptionsInsertString("-ksp_view");
	
#endif

//Set up exact solution
#if ANAL_2D
exact_solution_u = &exact_2D_solution_u;
exact_derivative_u = &exact_2D_derivative_u;

exact_solution_v = &exact_2D_solution_v;
exact_derivative_v = &exact_2D_derivative_v;

#if THREED
exact_solution_w = &exact_2D_solution_w;
exact_derivative_w = &exact_2D_derivative_w;
#endif

exact_solution_x = &exact_2D_solution_x;
exact_derivative_x = &exact_2D_derivative_x;

exact_solution_y = &exact_2D_solution_y;
exact_derivative_y = &exact_2D_derivative_y;

#if THREED
exact_solution_z = &exact_2D_solution_z;
exact_derivative_z = &exact_2D_derivative_z;
#endif

exact_solution_p = &exact_2D_solution_p;
exact_derivative_p = &exact_2D_derivative_p;

ExactSolution exact_sol_u(equation_systems);
exact_sol_u.attach_exact_value(exact_solution_u);
exact_sol_u.attach_exact_deriv(exact_derivative_u);

ExactSolution exact_sol_v(equation_systems);
exact_sol_v.attach_exact_value(exact_solution_v);
exact_sol_v.attach_exact_deriv(exact_derivative_v);

#if THREED
ExactSolution exact_sol_w(equation_systems);
exact_sol_w.attach_exact_value(exact_solution_w);
exact_sol_w.attach_exact_deriv(exact_derivative_w);
#endif

ExactSolution exact_sol_x(equation_systems);
exact_sol_x.attach_exact_value(exact_solution_x);
exact_sol_x.attach_exact_deriv(exact_derivative_x);

ExactSolution exact_sol_y(equation_systems);
exact_sol_y.attach_exact_value(exact_solution_y);
exact_sol_y.attach_exact_deriv(exact_derivative_y);

#if THREED
ExactSolution exact_sol_z(equation_systems);
exact_sol_z.attach_exact_value(exact_solution_z);
exact_sol_z.attach_exact_deriv(exact_derivative_z);
#endif

ExactSolution exact_sol_p(equation_systems);
exact_sol_p.attach_exact_value(exact_solution_p);
exact_sol_p.attach_exact_deriv(exact_derivative_p);

std::string u ("s_u");
std::string v ("s_v");
#if THREED
std::string w ("s_w");
#endif
std::string p ("s_p");
std::string x ("x");
std::string y ("y");
#if THREED
std::string z ("z");
#endif

#endif


Real T_L2_error_disp=0;
Real INF_H1_error_disp=0;
Real T_L2_error_vel=0;
Real T_Hdiv_error_vel=0;
Real INF_L2_error_p=0;


 equation_systems.parameters.set<Real> ("time") = 0;
 equation_systems.parameters.set<Real> ("dt") = dt;
 equation_systems.parameters.set<Real>("progress") = 0;
 equation_systems.parameters.set<unsigned int>("step") = 0; 

 
 system.assemble_before_solve=false;
 system.update();
 assemble_stiffness(equation_systems,"Last_non_linear_soln");

//This is what the for loop should be, now we are missing the last step !
//For some starnge reason the solver does sometimes not solve the last step. E.g when T=2,NT=3. It works when T=2, NT=4 ????
 for (unsigned int t_step=1; t_step<=n_timesteps; ++t_step)
  //for (unsigned int t_step=1; t_step<n_timesteps; ++t_step)
  {

    time += dt;
    equation_systems.parameters.set<Real> ("time") = time;
    double progress = (t_step+0.000000001) / (n_timesteps+0.000000001);
    equation_systems.parameters.set<Real>("progress") = progress;
    equation_systems.parameters.set<unsigned int>("step") = t_step; 

    std::cout << "\n\n*** Solving time step " << t_step << ", time = " << time <<  ", progress = " << progress << " ***" << std::endl;

    *system.old_local_solution = *system.current_local_solution;

  #if PETSC_MUMPS
      petsc_linear_solver =dynamic_cast<PetscLinearSolver<Number>*>(system.get_linear_solver());
      pc = petsc_linear_solver->pc();
      ierr = PCSetType(pc, PC_TYPE);
      CHKERRABORT(libMesh::COMM_WORLD,ierr);
      ierr = PCFactorSetMatSolverPackage(pc,SOLVER_NAME);
      CHKERRABORT(libMesh::COMM_WORLD,ierr);
    #endif

  //  equation_systems.get_system("Last_non_linear_soln").solve();


	system.rhs->zero();
	assemble_rhs(equation_systems,"Last_non_linear_soln");
	system.update();
	system.solve();
  

    // How many iterations were required to solve the linear system?
    std::cout<<"Number of iterations: "<<system.n_linear_iterations()<<std::endl;        
    // What was the final residual of the linear system?
    std::cout<<"Residual: "<< system.final_linear_residual()<<std::endl;

//To fix the strange solver problem set solution to zero (true analytical solution if solver fails

	/*
if(system.n_linear_iterations()<1 ){
system.solution->zero();
system.current_local_solution->zero(); 
system.update(); 

std::cout<< " Zeroed solution " << std::endl;

}
*/

    //Update the mesh position
    Mesh::node_iterator it_node = mesh.nodes_begin();
    const Mesh::node_iterator it_last_node = mesh.nodes_end();
    for ( ; it_node != it_last_node ; ++it_node)
    {
      Node* node_temp = *it_node;
      #if THREED
      for (unsigned int d = 0; d < 3; ++d) {
      #endif
      #if !THREED
      for (unsigned int d = 0; d < 2; ++d) {
      #endif
         unsigned int dest_dof = node_temp->dof_number(result.number(), d, 0);
        unsigned int dest_dof_system = node_temp->dof_number(result.number(), d, 0);
        Real value = (*node_temp)(d) + system.current_solution(dest_dof_system);
        result.current_local_solution->set(dest_dof, value);
        result.solution->set(dest_dof, value);

        reference.current_local_solution->set(dest_dof, (*node_temp)(d));
        reference.solution->set(dest_dof, (*node_temp)(d));
      }
      
      test(4);
			
    }



     #if EXODUS               
    std::stringstream file_name;
    file_name << equation_systems.parameters.get<std::string>("result_file_name");
    file_name << std::setw(2) << std::setfill('0') << t_step;
    file_name << ".e-s.";
    file_name << std::setw(3) << std::setfill('0') << t_step+1;
    exo.write_timestep(file_name.str(), equation_systems,t_step+1,time); 
    std::cout<<"exodus "<< file_name.str() <<std::endl;
   exo.write_element_data(equation_systems);
    #endif 
 

#if ANAL_2D

  Real H1_semi_error_disp, Hdiv_semi_error_vel;
  Real L2_error_press, L2_error_displacement, L2_error_velocity;

  assemble_error (H1_semi_error_disp,Hdiv_semi_error_vel,L2_error_press,L2_error_displacement,L2_error_velocity,equation_systems,"Last_non_linear_soln");

  Real H1_error_disp= pow( pow(L2_error_displacement,2)+pow(H1_semi_error_disp,2),0.5);
  Real Hdiv_error_vel=pow( pow(L2_error_velocity,2)+pow(Hdiv_semi_error_vel,2),0.5);

  std::cout<< "H1   u "<<H1_error_disp<<std::endl;
  std::cout<< "L2   u "<<L2_error_displacement<<std::endl;
  std::cout<< "L2   z "<<L2_error_velocity<<std::endl;
  std::cout<< "Hdiv z "<<Hdiv_error_vel<<std::endl;
  std::cout<< "L2   p "<<L2_error_press<<std::endl;


//#if TIME
  T_L2_error_disp+=dt*L2_error_displacement*L2_error_displacement;
  T_L2_error_vel+=dt*L2_error_velocity*L2_error_velocity;
  T_Hdiv_error_vel+=dt*Hdiv_error_vel*Hdiv_error_vel;
	
	
	if(INF_H1_error_disp<H1_error_disp ){
	  INF_H1_error_disp=H1_error_disp;	
	}
	
	if(INF_L2_error_p<L2_error_press ){
	  INF_L2_error_p=L2_error_press;	
	}
	
  std::cout<< "TL2   u "<<T_L2_error_disp<<std::endl;
  std::cout<< "INFH1   u "<<INF_H1_error_disp<<std::endl;
  std::cout<< "TL2   z "<<T_L2_error_vel<<std::endl;
  std::cout<< "THdiv z "<<T_Hdiv_error_vel<<std::endl;
  std::cout<< "INFL2    p "<<INF_L2_error_p<<std::endl;
//#endif

//Write the results to text(.mat) file
ofstream outFile;
outFile.open (equation_systems.parameters.get<std::string>("output_file_name").c_str());
std::cout<<"Write to  "<< equation_systems.parameters.get<std::string>("output_file_name") <<std::endl;


outFile << n_timesteps << " "<< N_eles << " " << pow(T_L2_error_disp,0.5) << " "<<  INF_H1_error_disp  << " " <<  pow(T_L2_error_vel,0.5) << " " << pow(T_Hdiv_error_vel,0.5) << " " << INF_L2_error_p << "\n";


outFile.close();    

#endif
      

 }

  std::cout<<"All done"<<std::endl;


  return 0;
}


#include "assemble.h"
#include "exact_functions.cpp"
#include "read_options.cpp"
#include "test.cpp"
#include "assemble_error.cpp"
#include "assemble_stiffness.cpp"
#include "assemble_rhs.cpp"
#include "read_parameters.cpp"
