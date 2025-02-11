#include <deal.II/lac/solver_cg.h>

#include <cmath>
#include <core/exceptions.h>
#include <core/matrixFreePDE.h>

// solve each time increment
template <int dim, int degree>
void
MatrixFreePDE<dim, degree>::solveIncrement(bool skip_time_dependent)
{
  // log time
  computing_timer.enter_subsection("matrixFreePDE: solveIncrements");
  Timer time;
  char  buffer[200];

  // Get the RHS of the explicit equations
  if (hasExplicitEquation && !skip_time_dependent)
    {
      computeExplicitRHS();
    }

  // solve for each field
  for (unsigned int fieldIndex = 0; fieldIndex < fields.size(); fieldIndex++)
    {
      currentFieldIndex = fieldIndex; // Used in computeLHS()

      // Parabolic (first order derivatives in time) fields
      if (fields[fieldIndex].pdetype == EXPLICIT_TIME_DEPENDENT && !skip_time_dependent)
        {
          updateExplicitSolution(fieldIndex);

          // Apply Boundary conditions
          applyBCs(fieldIndex);

          // Print update to screen and confirm that solution isn't nan
          if (currentIncrement % userInputs.skip_print_steps == 0)
            {
              double solution_L2_norm = solutionSet[fieldIndex]->l2_norm();

              snprintf(buffer,
                       sizeof(buffer),
                       "field '%2s' [explicit solve]: current solution: "
                       "%12.6e, current residual:%12.6e\n",
                       fields[fieldIndex].name.c_str(),
                       solution_L2_norm,
                       residualSet[fieldIndex]->l2_norm());
              pcout << buffer;

              if (!numbers::is_finite(solution_L2_norm))
                {
                  snprintf(buffer,
                           sizeof(buffer),
                           "ERROR: field '%s' solution is NAN. exiting.\n\n",
                           fields[fieldIndex].name.c_str());
                  pcout << buffer;
                  exit(-1);
                }
            }
        }
    }

  // Now, update the non-explicit variables
  // For the time being, this is just the elliptic equations, but implicit
  // parabolic and auxilary equations should also be here
  if (hasNonExplicitEquation)
    {
      bool         nonlinear_iteration_converged = false;
      unsigned int nonlinear_iteration_index     = 0;

      while (!nonlinear_iteration_converged)
        {
          nonlinear_iteration_converged = true;

          // Update residualSet for the non-explicitly updated variables
          computeNonexplicitRHS();

          for (const auto &[fieldIndex, variable] : var_attributes)
            {
              currentFieldIndex = fieldIndex; // Used in computeLHS()

              if ((fields[fieldIndex].pdetype == IMPLICIT_TIME_DEPENDENT &&
                   !skip_time_dependent) ||
                  fields[fieldIndex].pdetype == TIME_INDEPENDENT)
                {
                  if (currentIncrement % userInputs.skip_print_steps == 0 &&
                      variable.is_nonlinear)
                    {
                      snprintf(buffer,
                               sizeof(buffer),
                               "field '%2s' [nonlinear solve]: current "
                               "solution: %12.6e, current residual:%12.6e\n",
                               fields[fieldIndex].name.c_str(),
                               solutionSet[fieldIndex]->l2_norm(),
                               residualSet[fieldIndex]->l2_norm());
                      pcout << buffer;
                    }

                  nonlinear_iteration_converged =
                    updateImplicitSolution(fieldIndex, nonlinear_iteration_index);

                  // Apply Boundary conditions
                  applyBCs(fieldIndex);
                }
              else if (fields[fieldIndex].pdetype == AUXILIARY)
                {
                  if (variable.is_nonlinear || nonlinear_iteration_index == 0)
                    {
                      // If the equation for this field is nonlinear, save the old
                      // solution
                      if (variable.is_nonlinear)
                        {
                          if (fields[fieldIndex].type == SCALAR)
                            {
                              dU_scalar = *solutionSet[fieldIndex];
                            }
                          else
                            {
                              dU_vector = *solutionSet[fieldIndex];
                            }
                        }

                      updateExplicitSolution(fieldIndex);

                      // Apply Boundary conditions
                      applyBCs(fieldIndex);

                      // Print update to screen
                      if (currentIncrement % userInputs.skip_print_steps == 0)
                        {
                          snprintf(buffer,
                                   sizeof(buffer),
                                   "field '%2s' [auxiliary solve]: current solution: "
                                   "%12.6e, current residual:%12.6e\n",
                                   fields[fieldIndex].name.c_str(),
                                   solutionSet[fieldIndex]->l2_norm(),
                                   residualSet[fieldIndex]->l2_norm());
                          pcout << buffer;
                        }

                      // Check to see if this individual variable has converged
                      if (variable.is_nonlinear)
                        {
                          if (userInputs.nonlinear_solver_parameters.getToleranceType(
                                fieldIndex) == ABSOLUTE_SOLUTION_CHANGE)
                            {
                              double diff = NAN;

                              if (fields[fieldIndex].type == SCALAR)
                                {
                                  dU_scalar -= *solutionSet[fieldIndex];
                                  diff = dU_scalar.l2_norm();
                                }
                              else
                                {
                                  dU_vector -= *solutionSet[fieldIndex];
                                  diff = dU_vector.l2_norm();
                                }
                              if (currentIncrement % userInputs.skip_print_steps == 0)
                                {
                                  snprintf(buffer,
                                           sizeof(buffer),
                                           "  field '%2s' [nonlinear solve] current "
                                           "increment: %u, nonlinear "
                                           "iteration: "
                                           "%u, dU: %12.6e\n",
                                           fields[fieldIndex].name.c_str(),
                                           currentIncrement,
                                           nonlinear_iteration_index,
                                           diff);
                                  pcout << buffer;
                                }

                              if (diff > userInputs.nonlinear_solver_parameters
                                           .getToleranceValue(fieldIndex) &&
                                  nonlinear_iteration_index <
                                    userInputs.nonlinear_solver_parameters
                                      .getMaxIterations())
                                {
                                  nonlinear_iteration_converged = false;
                                }
                            }
                          else
                            {
                              AssertThrow(
                                false,
                                FeatureNotImplemented(
                                  "Nonlinear solver tolerances besides ABSOLUTE_CHANGE"));
                            }
                        }
                    }
                }

              // check if solution is nan
              if (!numbers::is_finite(solutionSet[fieldIndex]->l2_norm()))
                {
                  snprintf(buffer,
                           sizeof(buffer),
                           "ERROR: field '%s' solution is NAN. exiting.\n\n",
                           fields[fieldIndex].name.c_str());
                  pcout << buffer;
                  exit(-1);
                }
            }

          nonlinear_iteration_index++;
        }
    }

  if (currentIncrement % userInputs.skip_print_steps == 0)
    {
      pcout << "wall time: " << time.wall_time() << "s\n";
    }
  // log time
  computing_timer.leave_subsection("matrixFreePDE: solveIncrements");
}

// Application of boundary conditions
template <int dim, int degree>
void
MatrixFreePDE<dim, degree>::applyBCs(unsigned int fieldIndex)
{
  // Add Neumann BCs
  if (fields[fieldIndex].hasNeumannBCs)
    {
      // Currently commented out because it isn't working yet
      // applyNeumannBCs();
    }

  // Set the Dirichelet values (hanging node constraints don't need to be distributed
  // every time step, only at output)
  if (fields[fieldIndex].hasDirichletBCs)
    {
      // Apply non-uniform Dirlichlet_BCs to the current field
      if (fields[fieldIndex].hasnonuniformDirichletBCs)
        {
          DoFHandler<dim> *dof_handler    = nullptr;
          dof_handler                     = dofHandlersSet_nonconst.at(currentFieldIndex);
          IndexSet *locally_relevant_dofs = nullptr;
          locally_relevant_dofs = locally_relevant_dofsSet_nonconst.at(currentFieldIndex);
          locally_relevant_dofs->clear();
          DoFTools::extract_locally_relevant_dofs(*dof_handler, *locally_relevant_dofs);
          AffineConstraints<double> *constraintsDirichlet = nullptr;
          constraintsDirichlet = constraintsDirichletSet_nonconst.at(currentFieldIndex);
          constraintsDirichlet->clear();
          constraintsDirichlet->reinit(*locally_relevant_dofs);
          applyDirichletBCs();
          constraintsDirichlet->close();
        }
      // Distribute for Uniform or Non-Uniform Dirichlet BCs
      constraintsDirichletSet[fieldIndex]->distribute(*solutionSet[fieldIndex]);
    }
  solutionSet[fieldIndex]->update_ghost_values();
}

// Explicit time step for matrixfree solve
template <int dim, int degree>
void
MatrixFreePDE<dim, degree>::updateExplicitSolution(unsigned int fieldIndex)
{
  // Explicit-time step each DOF
  // Takes advantage of knowledge that the length of solutionSet and residualSet
  // is an integer multiple of the length of invM for vector variables
  if (fields[fieldIndex].type == SCALAR)
    {
      unsigned int invM_size = invMscalar.locally_owned_size();
      for (unsigned int dof = 0; dof < solutionSet[fieldIndex]->locally_owned_size();
           ++dof)
        {
          solutionSet[fieldIndex]->local_element(dof) =
            invMscalar.local_element(dof % invM_size) *
            residualSet[fieldIndex]->local_element(dof);
        }
    }
  else if (fields[fieldIndex].type == VECTOR)
    {
      unsigned int invM_size = invMvector.locally_owned_size();
      for (unsigned int dof = 0; dof < solutionSet[fieldIndex]->locally_owned_size();
           ++dof)
        {
          solutionSet[fieldIndex]->local_element(dof) =
            invMvector.local_element(dof % invM_size) *
            residualSet[fieldIndex]->local_element(dof);
        }
    }
}

template <int dim, int degree>
bool
MatrixFreePDE<dim, degree>::updateImplicitSolution(unsigned int fieldIndex,
                                                   unsigned int nonlinear_iteration_index)
{
  char buffer[200];

  // Assume convergence criterion is met, unless otherwise proven later on.
  bool nonlinear_iteration_converged = true;

  // Apply Dirichlet BC's. This clears the residual where we want to apply Dirichlet BCs,
  // otherwise the solver sees a positive residual
  constraintsDirichletSet[fieldIndex]->set_zero(*residualSet[fieldIndex]);

  // Grab solver controls
  double tol_value = NAN;
  if (userInputs.linear_solver_parameters.getToleranceType(fieldIndex) ==
      ABSOLUTE_RESIDUAL)
    {
      tol_value = userInputs.linear_solver_parameters.getToleranceValue(fieldIndex);
    }
  else
    {
      tol_value = userInputs.linear_solver_parameters.getToleranceValue(fieldIndex) *
                  residualSet[fieldIndex]->l2_norm();
    }

  SolverControl solver_control(userInputs.linear_solver_parameters.getMaxIterations(
                                 fieldIndex),
                               tol_value);

  // Currently the only allowed solver is SolverCG, the
  // SolverType input variable is a dummy
  SolverCG<dealii::LinearAlgebra::distributed::Vector<double>> solver(solver_control);

  // Solve
  try
    {
      if (fields[fieldIndex].type == SCALAR)
        {
          dU_scalar = 0.0;
          solver.solve(*this,
                       dU_scalar,
                       *residualSet[fieldIndex],
                       IdentityMatrix(solutionSet[fieldIndex]->size()));
        }
      else
        {
          dU_vector = 0.0;
          solver.solve(*this,
                       dU_vector,
                       *residualSet[fieldIndex],
                       IdentityMatrix(solutionSet[fieldIndex]->size()));
        }
    }
  catch (...)
    {
      pcout << "\nWarning: linear solver did not converge as "
               "per set tolerances. consider increasing the "
               "maximum number of iterations or decreasing the "
               "solver tolerance.\n";
    }

  if (var_attributes.at(fieldIndex).is_nonlinear)
    {
      // Now that we have the calculated change in the solution,
      // we need to select a damping coefficient
      double damping_coefficient = NAN;

      if (userInputs.nonlinear_solver_parameters.getBacktrackDampingFlag(fieldIndex))
        {
          dealii::LinearAlgebra::distributed::Vector<double> solutionSet_old =
            *solutionSet[fieldIndex];
          double residual_old = residualSet[fieldIndex]->l2_norm();

          damping_coefficient            = 1.0;
          bool damping_coefficient_found = false;
          while (!damping_coefficient_found)
            {
              if (fields[fieldIndex].type == SCALAR)
                {
                  solutionSet[fieldIndex]->sadd(1.0, damping_coefficient, dU_scalar);
                }
              else
                {
                  solutionSet[fieldIndex]->sadd(1.0, damping_coefficient, dU_vector);
                }

              computeNonexplicitRHS();

              for (const auto &it : *valuesDirichletSet[fieldIndex])
                {
                  if (residualSet[fieldIndex]->in_local_range(it.first))
                    {
                      (*residualSet[fieldIndex])(it.first) = 0.0;
                    }
                }

              double residual_new = residualSet[fieldIndex]->l2_norm();

              if (currentIncrement % userInputs.skip_print_steps == 0)
                {
                  pcout << "    Old residual: " << residual_old
                        << " Damping Coeff: " << damping_coefficient
                        << " New Residual: " << residual_new << "\n";
                }

              // An improved approach would use the
              // Armijo–Goldstein condition to ensure a
              // sufficent decrease in the residual. This way is
              // just scales the residual.
              if ((residual_new <
                   (residual_old * userInputs.nonlinear_solver_parameters
                                     .getBacktrackResidualDecreaseCoeff(fieldIndex))) ||
                  damping_coefficient < 1.0e-4)
                {
                  damping_coefficient_found = true;
                }
              else
                {
                  damping_coefficient *=
                    userInputs.nonlinear_solver_parameters.getBacktrackStepModifier(
                      fieldIndex);
                  *solutionSet[fieldIndex] = solutionSet_old;
                }
            }
        }
      else
        {
          damping_coefficient =
            userInputs.nonlinear_solver_parameters.getDefaultDampingCoefficient(
              fieldIndex);

          if (fields[fieldIndex].type == SCALAR)
            {
              solutionSet[fieldIndex]->sadd(1.0, damping_coefficient, dU_scalar);
            }
          else
            {
              solutionSet[fieldIndex]->sadd(1.0, damping_coefficient, dU_vector);
            }
        }

      if (currentIncrement % userInputs.skip_print_steps == 0)
        {
          double dU_norm = NAN;
          if (fields[fieldIndex].type == SCALAR)
            {
              dU_norm = dU_scalar.l2_norm();
            }
          else
            {
              dU_norm = dU_vector.l2_norm();
            }
          snprintf(buffer,
                   sizeof(buffer),
                   "field '%2s' [linear solve]: initial "
                   "residual:%12.6e, current residual:%12.6e, "
                   "nsteps:%u, tolerance criterion:%12.6e, "
                   "solution: %12.6e, dU: %12.6e\n",
                   fields[fieldIndex].name.c_str(),
                   residualSet[fieldIndex]->l2_norm(),
                   solver_control.last_value(),
                   solver_control.last_step(),
                   solver_control.tolerance(),
                   solutionSet[fieldIndex]->l2_norm(),
                   dU_norm);
          pcout << buffer;
        }

      // Check to see if this individual variable has converged
      if (userInputs.nonlinear_solver_parameters.getToleranceType(fieldIndex) ==
          ABSOLUTE_SOLUTION_CHANGE)
        {
          double diff = NAN;

          if (fields[fieldIndex].type == SCALAR)
            {
              diff = dU_scalar.l2_norm();
            }
          else
            {
              diff = dU_vector.l2_norm();
            }
          if (currentIncrement % userInputs.skip_print_steps == 0)
            {
              snprintf(buffer,
                       sizeof(buffer),
                       "  field '%2s' [nonlinear solve] current increment: %u, nonlinear "
                       "iteration: "
                       "%u, dU: %12.6e\n",
                       fields[fieldIndex].name.c_str(),
                       currentIncrement,
                       nonlinear_iteration_index,
                       diff);
              pcout << buffer;
            }

          if (diff >
                userInputs.nonlinear_solver_parameters.getToleranceValue(fieldIndex) &&
              nonlinear_iteration_index <
                userInputs.nonlinear_solver_parameters.getMaxIterations())
            {
              nonlinear_iteration_converged = false;
            }
          else if (diff >
                   userInputs.nonlinear_solver_parameters.getToleranceValue(fieldIndex))
            {
              pcout << "\nWarning: nonlinear solver did not converge as "
                       "per set tolerances. consider increasing the "
                       "maximum number of iterations or decreasing the "
                       "solver tolerance.\n";
            }
        }
      else
        {
          AssertThrow(false,
                      FeatureNotImplemented(
                        "Nonlinear solver tolerances besides ABSOLUTE_CHANGE"));
        }
    }
  else
    {
      if (nonlinear_iteration_index == 0)
        {
          if (fields[fieldIndex].type == SCALAR)
            {
              *solutionSet[fieldIndex] += dU_scalar;
            }
          else
            {
              *solutionSet[fieldIndex] += dU_vector;
            }

          if (currentIncrement % userInputs.skip_print_steps == 0)
            {
              double dU_norm = NAN;
              if (fields[fieldIndex].type == SCALAR)
                {
                  dU_norm = dU_scalar.l2_norm();
                }
              else
                {
                  dU_norm = dU_vector.l2_norm();
                }
              snprintf(buffer,
                       sizeof(buffer),
                       "field '%2s' [linear solve]: initial "
                       "residual:%12.6e, current residual:%12.6e, "
                       "nsteps:%u, tolerance criterion:%12.6e, "
                       "solution: %12.6e, dU: %12.6e\n",
                       fields[fieldIndex].name.c_str(),
                       residualSet[fieldIndex]->l2_norm(),
                       solver_control.last_value(),
                       solver_control.last_step(),
                       solver_control.tolerance(),
                       solutionSet[fieldIndex]->l2_norm(),
                       dU_norm);
              pcout << buffer;
            }
        }
    }

  return nonlinear_iteration_converged;
}

template class MatrixFreePDE<2, 1>;
template class MatrixFreePDE<3, 1>;

template class MatrixFreePDE<2, 2>;
template class MatrixFreePDE<3, 2>;

template class MatrixFreePDE<3, 3>;
template class MatrixFreePDE<2, 3>;

template class MatrixFreePDE<3, 4>;
template class MatrixFreePDE<2, 4>;

template class MatrixFreePDE<3, 5>;
template class MatrixFreePDE<2, 5>;

template class MatrixFreePDE<3, 6>;
template class MatrixFreePDE<2, 6>;