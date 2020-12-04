/*
 *            Copyright 2009-2020 The VOTCA Development Team
 *                       (http://www.votca.org)
 *
 *      Licensed under the Apache License, Version 2.0 (the "License")
 *
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#pragma once
#ifndef VOTCA_XTP_GW_H
#define VOTCA_XTP_GW_H

// Local VOTCA includes
#include "logger.h"
#include "orbitals.h"
#include "rpa.h"
#include "sigma_base.h"
#include "threecenter.h"

namespace votca {
namespace xtp {

class GW {
 public:
  GW(Logger& log, TCMatrix_gwbse& Mmn, const Eigen::MatrixXd& vxc,
     const Eigen::VectorXd& dft_energies)
      : _log(log),
        _Mmn(Mmn),
        _vxc(vxc),
        _dft_energies(dft_energies),
        _rpa(log, Mmn){};

  struct options {
    Index homo;
    Index qpmin;
    Index qpmax;
    Index rpamin;
    Index rpamax;
    double eta;
    double g_sc_limit;
    Index g_sc_max_iterations;
    double gw_sc_limit;
    Index gw_sc_max_iterations;
    double shift = 0;
    double ScaHFX = 0.0;
    std::string sigma_integration;
    Index reset_3c;  // how often the 3c integrals in iterate should be
                     // rebuilt
    std::string qp_solver;
    double qp_solver_alpha = 0.75;
    Index qp_grid_steps;            // Number of grid points
    double qp_grid_spacing;         // Spacing of grid points in Ha
    Index gw_mixing_order;          // mixing order
    double gw_mixing_alpha;         // mixing alpha, also linear mixing
    std::string quadrature_scheme;  // Kind of Gaussian-quadrature scheme to use
    Index order;   // only needed for complex integration sigma CDA
    double alpha;  // smooth tail in complex integration sigma CDA
    double qp_mae_tol; //To prove that we learn the curve we use the MAE 
    double qp_fixedpoint_tol; // Tolerance to be reach when usign Atkin method for fixed point solver
    Index qp_training_points = 5;  // Number of starting training point to use for Kernel Regression method
    double qp_spread;  // Spread of laplacian kernel for Kernel Regression method
     std::string qp_test_points; //Other options are even and odd 
     double qp_grid_hartree;  // How many hartree on the left and right of
                                   // the pre-shooted center of the grid
  };

  void configure(const options& opt);

  Eigen::VectorXd getGWAResults() const;
  // Calculates the diagonal elements up to self consistency
  void CalculateGWPerturbation();

  // Calculated offdiagonal elements as well
  void CalculateHQP();

  Eigen::MatrixXd getHQP() const;

  // Diagonalize QP particle Hamiltonian
  Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> DiagonalizeQPHamiltonian()
      const;

  void PlotSigma(std::string filename, Index steps, double spacing,
                 std::string states) const;

  Eigen::VectorXd RPAInputEnergies() const {
    return _rpa.getRPAInputEnergies();
  }

 private:
  Index _qptotal;

  Eigen::MatrixXd _Sigma_x;
  Eigen::MatrixXd _Sigma_c;

  options _opt;

  std::unique_ptr<Sigma_base> _sigma = nullptr;
  Logger& _log;
  TCMatrix_gwbse& _Mmn;
  const Eigen::MatrixXd& _vxc;
  const Eigen::VectorXd& _dft_energies;

  RPA _rpa;
  // small class which calculates f(w) with and df/dw(w)
  // f=Sigma_c(w)+offset-w
  // offset= e_dft+Sigma_x-Vxc
  class QPFunc {
   public:
    QPFunc(Index gw_level, const Sigma_base& sigma, double offset)
        : _gw_level(gw_level), _offset(offset), _sigma_c_func(sigma){};
    std::pair<double, double> operator()(double frequency) const {
      std::pair<double, double> result;
      result.first = value(frequency);
      result.second = deriv(frequency);

      return result;
    }
    double value(double frequency) const {
      return _sigma_c_func.CalcCorrelationDiagElement(_gw_level, frequency) +
             _offset - frequency;
    }
    double deriv(double frequency) const {
      return _sigma_c_func.CalcCorrelationDiagElementDerivative(_gw_level,
                                                                frequency) -
             1.0;
    }

   private:
    Index _gw_level;
    double _offset;
    const Sigma_base& _sigma_c_func;
  };

  double SolveQP_Bisection(double lowerbound, double f_lowerbound,
                           double upperbound, double f_upperbound,
                           const QPFunc& f) const;
  

  Eigen::VectorXd Laplacian_Kernel(double x1, Eigen::VectorXd x2,
                                   double sigma) const;
    
  boost::optional<double> SolveQP_KernelRegression(double intercept0,
                                             double frequency0,
                                             Index gw_level) const;
  double CalcHomoLumoShift(Eigen::VectorXd frequencies) const;
  Eigen::VectorXd ScissorShift_DFTlevel(
      const Eigen::VectorXd& dft_energies) const;
  void PrintQP_Energies(const Eigen::VectorXd& qp_diag_energies) const;
  void PrintGWA_Energies() const;

  Eigen::VectorXd SolveQP(const Eigen::VectorXd& frequencies) const;
  boost::optional<double> SolveQP_Grid(double intercept0, double frequency0,
                                       Index gw_level) const;
  boost::optional<double> SolveQP_FixedPoint(double intercept0,
                                             double frequency0,
                                             Index gw_level) const;
  boost::optional<double> SolveQP_Linearisation(double intercept0,
                                                double frequency0,
                                                Index gw_level) const;
  bool Converged(const Eigen::VectorXd& e1, const Eigen::VectorXd& e2,
                 double epsilon) const;
};
}  // namespace xtp
}  // namespace votca

#endif  // VOTCA_XTP_GW_H
