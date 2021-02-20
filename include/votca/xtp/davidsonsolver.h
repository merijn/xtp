/*
 * Copyright 2009-2020 The VOTCA Development Team (http://www.votca.org)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#pragma once
#ifndef VOTCA_XTP_DAVIDSONSOLVER_H
#define VOTCA_XTP_DAVIDSONSOLVER_H

// Standard includes
#include <chrono>
#include <iostream>
#include <stdexcept>

// Third party includes
#include <boost/format.hpp>

// Local VOTCA includes
#include "eigen.h"
#include "logger.h"

using boost::format;
using std::flush;

namespace votca {
namespace xtp {

/**
* \brief Use Davidson algorithm to solve A*V=E*V

**/

class DavidsonSolver {

 public:
  DavidsonSolver(Logger &log);

  void set_iter_max(Index N) { this->_iter_max = N; }
  void set_max_search_space(Index N) { this->_max_search_space = N; }
  void set_tolerance(std::string tol);
  void set_correction(std::string method);
  void set_size_update(std::string update_size);
  void set_matrix_type(std::string mt);

  Eigen::ComputationInfo info() const { return _info; }
  Eigen::VectorXd eigenvalues() const { return this->_eigenvalues; }
  Eigen::MatrixXd eigenvectors() const { return this->_eigenvectors; }
  Eigen::MatrixXd residues() const { return this->_res; }
  Index num_iterations() const { return this->_i_iter; }

  template <typename MatrixReplacement>
  void solve(const MatrixReplacement &A, Index neigen,
             Index size_initial_guess = 0) {

    if (_max_search_space < neigen) {
      _max_search_space = neigen * 5;
    }
    std::chrono::time_point<std::chrono::system_clock> start =
        std::chrono::system_clock::now();
    Index op_size = A.rows();

    checkOptions(op_size);
    printOptions(op_size);

    // initial guess size
    if (size_initial_guess == 0) {
      size_initial_guess = 2 * neigen;
    }

    // get the diagonal of the operator
    this->_Adiag = A.diagonal();

    // target the lowest diagonal element
    ProjectedSpace proj = initProjectedSpace(neigen, size_initial_guess);
    RitzEigenPair rep;
    XTP_LOG(Log::error, _log)
        << TimeStamp() << " iter\tSearch Space\tNorm" << flush;

    for (_i_iter = 0; _i_iter < _iter_max; _i_iter++) {

      updateProjection(A, proj);

      rep = getRitzEigenPairs(A, proj);

      bool converged = checkConvergence(rep, proj, neigen);

      printIterationData(rep, proj, neigen);

      bool last_iter = _i_iter == (_iter_max - 1);

      if (converged) {
        storeConvergedData(rep, neigen);
        break;
      } else if (last_iter) {
        storeNotConvergedData(rep, proj.root_converged, neigen);
        break;
      }
      Index extension_size=extendProjection(rep, proj);
      bool do_restart = (proj.search_space() > _max_search_space);

      if (do_restart) {
        restart(rep, proj, size_initial_guess,extension_size);
      }

    }

    printTiming(start);
    std::cout<<((A*eigenvectors()).eval()-eigenvectors()*eigenvalues().asDiagonal()).colwise().norm()<<std::endl;
Eigen::MatrixXd identity=Eigen::MatrixXd::Identity(A.rows(),A.cols());
    Eigen::MatrixXd Hmat=A*identity;
    Eigen::EigenSolver<Eigen::MatrixXd> ups(Hmat);
    Eigen::VectorXd values=ups.eigenvalues().real();
      std::sort(values.data(), values.data() + values.size(),
            [&](double i1, double i2) { return std::abs(i1) < std::abs(i2); });
    std::cout<<values.transpose()<<std::endl;
  }

 private:
  Logger &_log;
  Index _iter_max = 50;
  Index _i_iter = 0;
  double _tol = 1E-4;
  Index _max_search_space = 0;
  Eigen::VectorXd _Adiag;

  enum CORR { DPR, OLSEN };
  CORR _davidson_correction = CORR::DPR;

  enum UPDATE { MIN, SAFE, MAX };
  UPDATE _davidson_update = UPDATE::SAFE;

  enum MATRIX_TYPE { SYMM, HAM };
  MATRIX_TYPE _matrix_type = MATRIX_TYPE::SYMM;

  Eigen::VectorXd _eigenvalues;
  Eigen::MatrixXd _eigenvectors;
  Eigen::VectorXd _res;
  Eigen::ComputationInfo _info = Eigen::ComputationInfo::NoConvergence;

  struct RitzEigenPair {
    Eigen::VectorXd lambda;  // eigenvalues
    Eigen::MatrixXd q;       // Ritz (or harmonic Ritz) eigenvectors
    Eigen::MatrixXd U;       // eigenvectors of the small subspace
    Eigen::MatrixXd res;     // residues of the pairs
    Eigen::ArrayXd res_norm() const {
      return res.colwise().norm();
    }  // norm of the residues
  };

  struct ProjectedSpace {
    Eigen::MatrixXd V;   // basis of vectors
    Eigen::MatrixXd AV;  // A * V
    Eigen::MatrixXd T;   // V.T * A * V
    Index search_space() const {
      return V.cols();
    };                  // size of the projection i.e. number of cols in V
    Index size_update;  // size update ...
    std::vector<bool> root_converged;  // keep track of which root have onverged
  };

  template <typename MatrixReplacement>
  void updateProjection(const MatrixReplacement &A,
                        ProjectedSpace &proj) const {

    if (_i_iter == 0) {
      proj.AV = A * proj.V;
      proj.T = proj.V.transpose() * proj.AV;

    } else {
      /* if we use a GS ortho we do not have to recompute
      the entire projection as GS doesn't change the original subspace*/
      Index old_dim = proj.AV.cols();
      Index new_dim = proj.V.cols();
      Index nvec = new_dim - old_dim;
      proj.AV.conservativeResize(Eigen::NoChange, new_dim);
      proj.AV.rightCols(nvec) = A * proj.V.rightCols(nvec);
      proj.T = proj.V.transpose() * proj.AV;
    }
  }

  template <typename MatrixReplacement>
  RitzEigenPair getRitzEigenPairs(const MatrixReplacement &A,
                                  const ProjectedSpace &proj) const {
    // get the ritz vectors
    switch (this->_matrix_type) {
      case MATRIX_TYPE::SYMM: {
        return getRitz(proj);
      }
      case MATRIX_TYPE::HAM: {
        return getHarmonicRitz(A, proj);
      }
    }
    return RitzEigenPair();
  }

  Eigen::MatrixXd qr(const Eigen::MatrixXd &A) const;

  template <typename MatrixReplacement>
  RitzEigenPair getHarmonicRitz(const MatrixReplacement &A,
                                const ProjectedSpace &proj) const {

    /* Compute the Harmonic Ritz vector following
     * Computing Interior Eigenvalues of Large Matrices
     * Ronald B Morgan
     * LINEAR ALGEBRA AND ITS APPLICATIONS 154-156:289-309 (1991)
     * https://cpb-us-w2.wpmucdn.com/sites.baylor.edu/dist/e/71/files/2015/05/InterEvals-1vgdz91.pdf
     */

    RitzEigenPair rep;
    Eigen::MatrixXd B = proj.V.transpose() * (A * proj.AV).eval();

    bool return_eigenvectors = true;
    Eigen::GeneralizedEigenSolver<Eigen::MatrixXd> ges(proj.T, B,
                                                       return_eigenvectors);
    if (ges.info() != Eigen::ComputationInfo::Success) {
      std::cerr << "A\n" << proj.T;
      std::cerr << "B\n" << std::endl;
      throw std::runtime_error("Small generalized eigenvalue problem failed.");
    }

    std::vector<std::pair<Index, Index>> complex_pairs;
    for (Index i = 0; i < ges.eigenvalues().size(); i++) {
      if (ges.eigenvalues()(i).imag() != 0) {
        bool found_partner = false;
        for (auto &pair : complex_pairs) {
          if (pair.second > -1) {
            continue;
          } else {
            bool are_pair = (std::abs(ges.eigenvalues()(pair.first).real() -
                                      ges.eigenvalues()(i).real()) < 1e-9) &&
                            (std::abs(ges.eigenvalues()(pair.first).imag() +
                                      ges.eigenvalues()(i).imag()) < 1e-9);
            if (are_pair) {
              pair.second = i;
              found_partner = true;
            }
          }
        }

        if (!found_partner) {
          complex_pairs.emplace_back(i, -1);
        }
      }
    }
    for (const auto &pair : complex_pairs) {
      std::cout << pair.first << " " << pair.second << std::endl;
    }

    for (const auto &pair : complex_pairs) {
      if (pair.second < 0) {
        throw std::runtime_error("Eigenvalue:" + std::to_string(pair.first) +
                                 " is complex but has no partner.");
      }
    }
    if (!complex_pairs.empty()) {
      XTP_LOG(Log::warning, _log)
          << TimeStamp() << " Found " << complex_pairs.size()
          << " complex pairs in eigenvalue problem" << flush;
    }
    Eigen::VectorXd eigenvalues =
        Eigen::VectorXd::Zero(ges.eigenvalues().size() - complex_pairs.size());
    Eigen::MatrixXd eigenvectors =
        Eigen::MatrixXd::Zero(ges.eigenvectors().rows(),
                              ges.eigenvectors().cols() - complex_pairs.size());

    Index j = 0;
    for (Index i = 0; i < ges.eigenvalues().size(); i++) {
      bool is_second_in_complex_pair =
          std::find_if(complex_pairs.begin(), complex_pairs.end(),
                       [&](const std::pair<Index, Index> &pair) {
                         return pair.second == i;
                       }) != complex_pairs.end();
      if (is_second_in_complex_pair) {
        continue;
      } else {
        eigenvalues(j) = ges.eigenvalues()(i).real();
        eigenvectors.col(j) = ges.eigenvectors().col(i).real();
        eigenvectors.col(j).normalize();
        j++;
      }
    }

    ArrayXl idx = DavidsonSolver::argsort(eigenvalues).reverse();
    // we need the largest values, because this is the inverse value, so
    // reverse list
    rep.U = DavidsonSolver::extract_vectors(eigenvectors, idx);
    rep.lambda = (rep.U.transpose() * proj.T * rep.U).diagonal();
    rep.q = proj.V * rep.U;  // Ritz vectors
    rep.res = proj.AV * rep.U - rep.q * rep.lambda.asDiagonal();  // residues
    return rep;
  }

  RitzEigenPair getRitz(const ProjectedSpace &proj) const;

  Index getSizeUpdate(Index neigen) const;

  void checkOptions(Index operator_size);

  void printOptions(Index operator_size) const;

  void printTiming(
      const std::chrono::time_point<std::chrono::system_clock> &start) const;

  void printIterationData(const RitzEigenPair &rep, const ProjectedSpace &proj,
                          Index neigen) const;

  ArrayXl argsort(const Eigen::VectorXd &V) const;

  Eigen::MatrixXd setupInitialEigenvectors(Index size_initial_guess) const;

  Eigen::MatrixXd extract_vectors(const Eigen::MatrixXd &V,
                                  const ArrayXl &idx) const;

  Eigen::MatrixXd orthogonalize(const Eigen::MatrixXd &V, Index nupdate)const;
  Eigen::MatrixXd gramschmidt(const Eigen::MatrixXd &A, Index nstart)const;

  Eigen::VectorXd computeCorrectionVector(const Eigen::VectorXd &qj,
                                          double lambdaj,
                                          const Eigen::VectorXd &Aqj) const;
  Eigen::VectorXd dpr(const Eigen::VectorXd &r, double lambda) const;
  Eigen::VectorXd olsen(const Eigen::VectorXd &r, const Eigen::VectorXd &x,
                        double lambda) const;

  ProjectedSpace initProjectedSpace(Index neigen,
                                    Index size_initial_guess) const;

  Index extendProjection(const RitzEigenPair &rep, ProjectedSpace &proj);

  bool checkConvergence(const RitzEigenPair &rep, ProjectedSpace &proj,
                        Index neigen);

  void restart(const RitzEigenPair &rep, ProjectedSpace &proj,
               Index size_restart, Index newtestvectors) const;

  void storeConvergedData(const RitzEigenPair &rep, Index neigen);

  void storeNotConvergedData(const RitzEigenPair &rep,
                             std::vector<bool> &root_converged, Index neigen);

  void storeEigenPairs(const RitzEigenPair &rep, Index neigen);
};  // namespace xtp

}  // namespace xtp
}  // namespace votca

#endif  // VOTCA_XTP_DAVIDSONSOLVER_H
