// setNonlinearEqInitialGuess() method for MatrixFreePDE class
#include <deal.II/lac/solver_cg.h>
#include <deal.II/matrix_free/evaluation_flags.h>

#include <cmath>
#include <core/matrixFreePDE.h>

// solve each time increment
template <int dim, int degree>
void
MatrixFreePDE<dim, degree>::setNonlinearEqInitialGuess()
{
  // log time
  computing_timer.enter_subsection("matrixFreePDE: setNonlinearEqInitialGuess");
  Timer time;
  time.start();
  char buffer[200];

  for (const auto &[fieldIndex, variable] : var_attributes)
    {
      if ((variable.eq_type == TIME_INDEPENDENT) && variable.is_nonlinear &&
          userInputs.nonlinear_solver_parameters.getLaplaceInitializationFlag(fieldIndex))
        {
          currentFieldIndex = fieldIndex; // Used in computeLaplaceLHS()

          computeLaplaceRHS(fieldIndex);

          for (const auto &it : *valuesDirichletSet[fieldIndex])
            {
              if (residualSet[fieldIndex]->in_local_range(it.first))
                {
                  (*residualSet[fieldIndex])(it.first) = 0.0;
                }
            }

          // solver controls
          double tol_value = NAN;
          if (userInputs.linear_solver_parameters.getToleranceType(fieldIndex) ==
              ABSOLUTE_RESIDUAL)
            {
              tol_value =
                userInputs.linear_solver_parameters.getToleranceValue(fieldIndex);
            }
          else
            {
              tol_value =
                userInputs.linear_solver_parameters.getToleranceValue(fieldIndex) *
                residualSet[fieldIndex]->l2_norm();
            }

          SolverControl solver_control(
            userInputs.linear_solver_parameters.getMaxIterations(fieldIndex),
            tol_value);

          // Currently the only allowed solver is SolverCG, the SolverType input
          // variable is a dummy
          SolverCG<dealii::LinearAlgebra::distributed::Vector<double>> solver(
            solver_control);

          // solve
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
              pcout << "\nWarning: implicit solver did not converge as per set "
                       "tolerances. consider increasing maxSolverIterations or "
                       "decreasing solverTolerance.\n";
            }

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
                       "field '%2s' [laplace solve for initial guess]: initial "
                       "residual:%12.6e, current residual:%12.6e, nsteps:%u, "
                       "tolerance criterion:%12.6e, solution: %12.6e, dU: %12.6e\n",
                       fields[fieldIndex].name.c_str(),
                       residualSet[fieldIndex]->l2_norm(),
                       solver_control.last_value(),
                       solver_control.last_step(),
                       solver_control.tolerance(),
                       solutionSet[fieldIndex]->l2_norm(),
                       dU_norm);
              pcout << buffer << "\n";
            }
        }
    }

  if (currentIncrement % userInputs.skip_print_steps == 0)
    {
      pcout << "wall time: " << time.wall_time() << "s\n";
    }
  // log time
  computing_timer.leave_subsection("matrixFreePDE: setNonlinearEqInitialGuess");
}

template <int dim, int degree>
void
MatrixFreePDE<dim, degree>::computeLaplaceRHS(unsigned int fieldIndex)
{
  // log time
  computing_timer.enter_subsection("matrixFreePDE: computeLaplaceRHS");

  // call to integrate and assemble while clearing residual vecotrs
  matrixFreeObject.cell_loop(&MatrixFreePDE<dim, degree>::getLaplaceRHS,
                             this,
                             *residualSet[fieldIndex],
                             *solutionSet[fieldIndex],
                             true);

  // end log
  computing_timer.leave_subsection("matrixFreePDE: computeLaplaceRHS");
}

template <int dim, int degree>
void
MatrixFreePDE<dim, degree>::getLaplaceRHS(
  const MatrixFree<dim, double>                            &data,
  dealii::LinearAlgebra::distributed::Vector<double>       &dst,
  const dealii::LinearAlgebra::distributed::Vector<double> &src,
  const std::pair<unsigned int, unsigned int>              &cell_range) const
{
  FEEvaluation<dim, degree> mat(data);

  dealii::EvaluationFlags::EvaluationFlags laplace_flags =
    dealii::EvaluationFlags::gradients;
  // loop over all "cells"
  for (unsigned int cell = cell_range.first; cell < cell_range.second; ++cell)
    {
      mat.reinit(cell);
      mat.read_dof_values(src);
      mat.evaluate(laplace_flags);
      for (unsigned int q = 0; q < mat.n_q_points; ++q)
        {
          mat.submit_gradient(mat.get_gradient(q), q);
        }
      mat.integrate(laplace_flags);
      mat.distribute_local_to_global(dst);
    }
}

template <int dim, int degree>
void
MatrixFreePDE<dim, degree>::getLaplaceLHS(
  const MatrixFree<dim, double>                            &data,
  dealii::LinearAlgebra::distributed::Vector<double>       &dst,
  const dealii::LinearAlgebra::distributed::Vector<double> &src,
  const std::pair<unsigned int, unsigned int>              &cell_range) const
{
  FEEvaluation<dim, degree> mat(data);

  dealii::EvaluationFlags::EvaluationFlags laplace_flags =
    dealii::EvaluationFlags::gradients;
  // loop over all "cells"
  for (unsigned int cell = cell_range.first; cell < cell_range.second; ++cell)
    {
      mat.reinit(cell);
      mat.read_dof_values(src);
      mat.evaluate(laplace_flags);
      for (unsigned int q = 0; q < mat.n_q_points; ++q)
        {
          mat.submit_gradient(-mat.get_gradient(q), q);
        }
      mat.integrate(laplace_flags);
      mat.distribute_local_to_global(dst);
    }
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