# PRISMS-PF Application Formulation: dendriticSolidification

This example application implements a simple model of dendritic solidification based on the CHiMaD Benchmark Problem 3, itself based on the model given in the following article: \\
``Multiscale Finite-Difference-Diffusion-Monte-Carlo Method for Simulating Dendritic Solidification'' by M. Plapp and A. Karma, \emph{Journal of Computational Physics}, 165, 592-619 (2000)

This example application examines the non-isothermal solidification of a pure substance. The simulation starts with a circular solid seed in a uniformly undercooled liquid. As this seed grows, two variables are tracked, an order parameter, $\phi$, that denotes whether the material a liquid or solid and a nondimensional temperature,  $u$. The crystal structure of the solid is offset from the simulation frame for generality and to expose more readily any effects of the mesh on the dendrite shape.

\section{Governing Equations}

Consider a free energy density given by:
\begin{equation}
  \Pi = \int_{\Omega}   \left[ \frac{1}{2} W^2(\hat{n})|\nabla \phi|^2+f(\phi,u) \right]   ~dV 
\end{equation}
where $\phi$ is an order parameter for the solid phase and $u$ is the dimensionaless temperature:
\begin{equation}
u = \frac{T - T_m}{L/c_p}
\end{equation}
for temperature $T$, melting temperature $T_m$, latent heat $L$, and specific heat $c_p$. The free energy density, $f(\phi,u)$ is given by a double-well potential:
\begin{equation}
f(\phi,u) = -\frac{1}{2}\phi^2 + \frac{1}{4}\phi^4 + \lambda u \phi \left(1-\frac{2}{3} \phi^2+\frac{1}{5}\phi^4 \right)
\end{equation}
where $\lambda$ is a dimensionless coupling constant. The gradient energy coefficient, $W$, is given by 
\begin{equation}
W(\theta) = W_0 [1+\epsilon_m \cos[m(\theta-\theta_0)]]
\end{equation}
where, $W_0$, $\epsilon_m$, and $\theta_0$ are constants and $\theta$ is the in-plane azimuthal angle, where $\tan(\theta) = \frac{\partial \phi}{\partial y} / \frac{\partial \phi}{\partial x}$.

The evolution equations are:
\begin{gather}
\frac{\partial u}{\partial t} = D \nabla^2 u + \frac{1}{2}  \frac{\partial \phi}{\partial t} \\
\tau(\hat{n}) \frac{\partial \phi}{\partial t} = -\frac{\partial f}{\partial \phi} + \nabla \cdot \left[W^2(\theta) \nabla \phi \right]+  \frac{\partial}{\partial x} \left[ |\nabla \phi|^2 W(\theta) \frac{\partial W(\theta)}{\partial \left( \frac{\partial \phi}{\partial x} \right)} \right] + \frac{\partial}{\partial y} \left[ |\nabla \phi|^2 W(\theta) \frac{\partial W(\theta)}{\partial \left( \frac{\partial \phi}{\partial y} \right)} \right] 
\end{gather}
where
\begin{gather}
\tau(\hat{n}) = \tau_0 [1+\epsilon_m \cos[m(\theta-\theta_0)]] \\
D = \frac{0.6267 \lambda W_0^2}{\tau_0}
\end{gather}

The governing equations can be written more compactly using the variable $\mu$, the driving force for the phase transformation:
\begin{gather}
\frac{\partial u}{\partial t} = D \nabla^2 u + \frac{\mu}{2 \tau} \\
\tau(\hat{n}) \frac{\partial \phi}{\partial t} = \mu \\
\mu = -\frac{\partial f}{\partial \phi} + \nabla \cdot \left[W^2(\theta) \nabla \phi \right]+  \frac{\partial}{\partial x} \left[ |\nabla \phi|^2 W(\theta) \frac{\partial W(\theta)}{\partial \left( \frac{\partial \phi}{\partial x} \right)} \right] + \frac{\partial}{\partial y} \left[ |\nabla \phi|^2 W\theta) \frac{\partial W(\theta)}{\partial \left( \frac{\partial \phi}{\partial y} \right)} \right] 
\end{gather}

The  $\frac{\partial W(\theta)}{\partial \left( \frac{\partial \phi}{\partial x} \right)}$ and $\frac{\partial W(\theta)}{\partial \left( \frac{\partial \phi}{\partial y} \right)}$ expressions can be evaluated using the chain rule, using $\theta$ as an intermediary (i.e. $\frac{\partial W(\theta)}{\partial \left( \frac{\partial \phi}{\partial x} \right)}=\frac{\partial W(\theta)}{\partial \theta} \frac{\partial \theta}{\partial \left( \frac{\partial \phi}{\partial x} \right)}$  and $\frac{\partial W(\theta)}{\partial \left( \frac{\partial \phi}{\partial y} \right)}=\frac{\partial W(\theta)}{\partial \theta} \frac{\partial \theta}{\partial \left( \frac{\partial \phi}{\partial y} \right)}$). Also, the last two terms can be expressed using a divergence operator, allowing them to be grouped with the second term, which will simplify matters later. Carrying out these transformations yields:
\begin{multline}
\mu = \left[ \phi - \lambda u \left(1 - \phi^2 \right) \right] \left(1-\phi^2\right) + \nabla \cdot \bigg[\left(W^2 \frac{\partial \phi}{\partial x} + W_0 \epsilon_m m W(\theta) \sin \left[ m \left(\theta - \theta_0 \right) \right] \frac{\partial \phi}{\partial y}\right)\hat{x}  \\
+ \left(W^2 \frac{\partial \phi}{\partial y} -W_0 \epsilon_m m W(\theta) \sin \left[ m \left(\theta - \theta_0 \right) \right] \frac{\partial \phi}{\partial x}\right) \hat{y} \bigg]
\end{multline}

\section{Model Constants}
$W_0$: Controls the interfacial thickness, default value of 1.0. \\
$\tau_0$: Controls the phase transformation kinetics, default value of 1.0. \\
$\epsilon_m$: T the strength of the anisotropy, default value of 0.05. \\
$D$: The thermal diffusion constant, default value of 1.0. \\
$\Delta: \frac{T_m-T_0}{L/c_p}$: The level of undercooling, default value of 0.75. \\
$\theta_0$: The rotation angle of the anisotropy with respect to the simulation frame, default value of 0.125 ($\sim$7.2$^\circ$).

\section{Time Discretization}
Considering forward Euler explicit time stepping, we have the time discretized kinetics equation:
\begin{gather}
u^{n+1} = u^{n} + \Delta t \left( D  \nabla^2 u^n + \frac{\mu^n}{2 \tau} \right) \\
\phi^{n+1} = \phi^n + \frac{\Delta t \mu^n}{\tau}
\end{gather}
\begin{multline}
\mu^{n+1} =  \left[ \phi^n - \lambda u \left(1 - (\phi^n)^2 \right) \right] \left(1-(\phi^n)^2\right) + \nabla \cdot \bigg[\left(W^2 \frac{\partial \phi^n}{\partial x} + W_0 \epsilon_m m W(\theta^n) \sin \left[ m \left(\theta^n - \theta_0 \right) \right] \frac{\partial \phi^n}{\partial y}\right)\hat{x}  \\
+ \left(W^2 \frac{\partial \phi^n}{\partial y} -W_0 \epsilon_m m W(\theta^n) \sin \left[ m \left(\theta^n - \theta_0 \right) \right] \frac{\partial \phi^n}{\partial x}\right) \hat{y} \bigg]
\end{multline}

\section{Weak Formulation}

\begin{gather}
\int_{\Omega}   w  u^{n+1}  ~dV = \int_{\Omega}   w \underbrace{\left(u^{n} + \frac{\mu^n \Delta t}{2 \tau}\right)}_{r_u} + \nabla w \cdot \underbrace{(-D \Delta t \nabla u^n)}_{r_{ux}} ~dV \\
\int_{\Omega}   w  \phi^{n+1}  ~dV = \int_{\Omega}   w \underbrace{\left(\phi^n + \frac{\Delta t \mu^n}{\tau}\right)}_{r_{\phi}} ~dV 
\end{gather} \small
\begin{multline}
\int_{\Omega}   w  \mu^{n+1}  ~dV = \int_{\Omega}   w \underbrace{\left[ \phi^n - \lambda u \left(1 - (\phi^n)^2 \right) \right] \left(1-(\phi^n)^2\right)}_{r_{\mu}} \\
+ \nabla w \cdot \underbrace{\bigg[-\left(W^2 \frac{\partial \phi^n}{\partial x} + W_0 \epsilon_m m W(\theta^n) \sin \left[ m \left(\theta^n - \theta_0 \right) \right] \frac{\partial \phi^n}{\partial y}\right)\hat{x}
- \left(W^2 \frac{\partial \phi^n}{\partial y} -W_0 \epsilon_m m W(\theta^n) \sin \left[ m \left(\theta^n - \theta_0 \right) \right] \frac{\partial \phi^n}{\partial x}\right) \hat{y} \bigg]}_{r_{\phi x}}  ~dV 
\end{multline}
\normalsize

\vskip 0.25in
The above values of $r_{u}$, $r_{ux}$, $r_{\phi}$, and $r_{\phi x}$ and $r_{\mu}$ are used to define the residuals in the following parameters file: \\
\textit{applications/dendriticSolification/parameters.h}