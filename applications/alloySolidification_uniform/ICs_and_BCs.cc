//SPDX-FileCopyrightText: © 2025 PRISMS Phase Field Laboratory at the University of Michigan <prisms-pf@umich.edu>
//SPDX-License-Identifier: GNU Lesser General Public Version 2.1

// ===========================================================================
// FUNCTION FOR INITIAL CONDITIONS
// ===========================================================================

template <int dim, int degree>
void
customPDE<dim, degree>::setInitialCondition([[maybe_unused]] const Point<dim>  &p,
                                            [[maybe_unused]] const unsigned int index,
                                            [[maybe_unused]] double            &scalar_IC,
                                            [[maybe_unused]] Vector<double>    &vector_IC)
{
  // ---------------------------------------------------------------------
  // ENTER THE INITIAL CONDITIONS HERE
  // ---------------------------------------------------------------------
  // Enter the function describing conditions for the fields at point "p".
  // Use "if" statements to set the initial condition for each variable
  // according to its variable index

  // The initial condition is two circles/spheres defined
  // by a hyperbolic tangent function. The center of each circle/sphere is
  // given by "center" and its radius is given by "rad".

  double center[2] = {0.0, 0.0};
  double rad       = 22.0 * d0inW * W;

  scalar_IC = 0;

  // "tanh" profile
  double dist = 0.0;
  for (unsigned int dir = 0; dir < dim; dir++)
    {
      dist += (p[dir] - center[dir]) * (p[dir] - center[dir]);
    }
  dist        = std::sqrt(dist);
  double phi0 = -std::tanh((dist - rad) / (std::sqrt(2)));

  // Initial condition for the order parameter field
  if (index == 0)
    {
      scalar_IC = phi0;
    }

  // Initial condition for the concentration field (normalized over cl0)
  else if (index == 1)
    {
      double eu0 = 1.0 - (1.0 - k) * Omega;
      scalar_IC  = 0.5 * cl0 * eu0 * (1.0 + k - (1.0 - k) * phi0);
    }
  // Initial condition xi (not important)
  else
    {
      scalar_IC = 0.0;
    }
  // --------------------------------------------------------------------------
}

// ===========================================================================
// FUNCTION FOR NON-UNIFORM DIRICHLET BOUNDARY CONDITIONS
// ===========================================================================

template <int dim, int degree>
void
customPDE<dim, degree>::setNonUniformDirichletBCs(
  [[maybe_unused]] const Point<dim>  &p,
  [[maybe_unused]] const unsigned int index,
  [[maybe_unused]] const unsigned int direction,
  [[maybe_unused]] const double       time,
  [[maybe_unused]] double            &scalar_BC,
  [[maybe_unused]] Vector<double>    &vector_BC)
{
  // --------------------------------------------------------------------------
  // ENTER THE NON-UNIFORM DIRICHLET BOUNDARY CONDITIONS HERE
  // --------------------------------------------------------------------------
  // Enter the function describing conditions for the fields at point "p".
  // Use "if" statements to set the boundary condition for each variable
  // according to its variable index. This function can be left blank if there
  // are no non-uniform Dirichlet boundary conditions. For BCs that change in
  // time, you can access the current time through the variable "time". The
  // boundary index can be accessed via the variable "direction", which starts
  // at zero and uses the same order as the BC specification in parameters.in
  // (i.e. left = 0, right = 1, bottom = 2, top = 3, front = 4, back = 5).

  // -------------------------------------------------------------------------
}
