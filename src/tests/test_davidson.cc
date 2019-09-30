
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE davidson_test

#include <boost/test/unit_test.hpp>
#include <iostream>

#include <votca/xtp/davidsonsolver.h>
#include <votca/xtp/eigen.h>
#include <votca/xtp/matrixfreeoperator.h>
#include <votca/xtp/bseoperator_btda.h>

using namespace votca::xtp;
using namespace std;


Eigen::MatrixXd symm_matrix(int N, double eps) {
  Eigen::MatrixXd matrix;
  matrix = eps * Eigen::MatrixXd::Random(N, N);
  Eigen::MatrixXd tmat = matrix.transpose();
  matrix = matrix + tmat;
  return matrix;
}

Eigen::MatrixXd init_matrix(int N, double eps) {
  Eigen::MatrixXd matrix;
  matrix = eps * Eigen::MatrixXd::Random(N, N);
  Eigen::MatrixXd tmat = matrix.transpose();
  matrix = matrix + tmat;

  for (int i = 0; i < N; i++) {
    matrix(i, i) = static_cast<double>(1. + (std::rand() % 1000) / 10.);
  }
  return matrix;
}


BOOST_AUTO_TEST_SUITE(davidson_test)

BOOST_AUTO_TEST_CASE(davidson_full_matrix) {

  int size = 100;
  int neigen = 10;
  double eps = 0.01;
  Eigen::MatrixXd A = init_matrix(size, eps);

  Logger log;
  DavidsonSolver DS(log);
  DS.solve(A, neigen);
  Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> es(A);

  auto lambda = DS.eigenvalues();
  auto lambda_ref = es.eigenvalues().head(neigen);
  bool check_eigenvalues = lambda.isApprox(lambda_ref, 1E-6);

  BOOST_CHECK_EQUAL(check_eigenvalues, 1);
}

BOOST_AUTO_TEST_CASE(davidson_full_matrix_fail) {

  int size = 100;
  int neigen = 10;
  double eps = 0.01;
  Eigen::MatrixXd A = init_matrix(size, eps);

  Logger log;
  DavidsonSolver DS(log);
  DS.set_iter_max(1);
  DS.solve(A, neigen);
  Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> es(A);

  auto lambda = DS.eigenvalues();
  auto lambda_ref = es.eigenvalues().head(neigen);
  bool check_eigenvalues = lambda.isApprox(lambda_ref, 1E-6);

  BOOST_CHECK_EQUAL(check_eigenvalues, 0);
}

class TestOperator : public MatrixFreeOperator {
 public:
  TestOperator(){};
  Eigen::RowVectorXd row(int index) const;
  void set_diag();
  Eigen::VectorXd diag_el;

 private:
};

// constructors
void TestOperator::set_diag() {
  int lsize = this->size();
  diag_el = Eigen::VectorXd::Zero(lsize);
  for (int i = 0; i < lsize; i++) {
    diag_el(i) = static_cast<double>(1. + (std::rand() % 1000) / 10.);
  }
}

//  get a col of the operator
Eigen::RowVectorXd TestOperator::row(int index) const {
  int lsize = this->size();
  Eigen::RowVectorXd row_out = Eigen::RowVectorXd::Zero(lsize);
  for (int j = 0; j < lsize; j++) {
    if (j == index) {
      row_out(j) = diag_el(j);
    } else {
      row_out(j) = 0.01 / std::pow(static_cast<double>(j - index), 2);
    }
  }
  return row_out;
}

BOOST_AUTO_TEST_CASE(davidson_matrix_free) {

  int size = 100;
  int neigen = 10;

  // Create Operator
  TestOperator Aop;
  Aop.set_size(size);
  Aop.set_diag();

  Logger log;
  DavidsonSolver DS(log);
  DS.set_tolerance("normal");
  DS.set_size_update("safe");
  DS.solve(Aop, neigen);

  Eigen::MatrixXd A = Aop.get_full_matrix();
  Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> es(A);

  auto lambda = DS.eigenvalues();
  auto lambda_ref = es.eigenvalues().head(neigen);
  bool check_eigenvalues = lambda.isApprox(lambda_ref, 1E-6);

  BOOST_CHECK_EQUAL(check_eigenvalues, 1);
}


Eigen::VectorXd sort_ev(Eigen::VectorXd ev)
{

  int nev = ev.rows();
  int npos = nev/2;

  Eigen::VectorXd ev_pos = Eigen::VectorXd::Zero(npos);
  int nstored = 0;

  // get only positives
  for (int i=0; i<nev; i++) {
    if (ev(i) > 0) {
      ev_pos(nstored) = ev(i);
      nstored ++;
    }
  }
  
  // sort the epos eigenvalues
  std::sort(ev_pos.data(), ev_pos.data() + ev_pos.size());
  return ev_pos;
}

class BlockOperator: public MatrixFreeOperator {
 public:
  BlockOperator() {};

  void attach_matrix(const Eigen::MatrixXd &mat);
  Eigen::RowVectorXd row(int index) const;
  void set_diag(int diag);
  Eigen::VectorXd diag_el;

 private:
  int _diag;
  int _alpha;
  Eigen::MatrixXd _mat;
};

void BlockOperator::attach_matrix(const Eigen::MatrixXd &mat) {
  _mat = mat;
}

//  get a col of the operator
Eigen::RowVectorXd BlockOperator::row(int index) const {
  return _mat.row(index);
}


BOOST_AUTO_TEST_CASE(davidson_hamiltonian_matrix_free) {

  int size = 60;
  int neigen = 5;
  Logger log;

  // Create Operator
  BlockOperator Rop;
  Rop.set_size(size);
  Eigen::MatrixXd rmat = init_matrix(size,0.01);
  Rop.attach_matrix(rmat);
  

  BlockOperator Cop;
  Cop.set_size(size);
  Eigen::MatrixXd cmat = symm_matrix(size,0.01);
  Cop.attach_matrix(cmat);

// create Hamiltonian operator
  HamiltonianOperator<BlockOperator,BlockOperator> Hop(Rop,Cop);

  DavidsonSolver DS(log);
  DS.set_tolerance("normal");
  DS.set_size_update("max");
  DS.set_ortho("QR");
  DS.set_matrix_type("HAM");
  DS.solve(Hop, neigen);
  auto lambda = DS.eigenvalues().real();
  std::sort(lambda.data(), lambda.data() + lambda.size());  

  Eigen::MatrixXd H = Hop.get_full_matrix();
  Eigen::EigenSolver<Eigen::MatrixXd> es(H);
  auto lambda_ref = sort_ev(es.eigenvalues().real());
  bool check_eigenvalues = lambda.isApprox(lambda_ref.head(neigen), 1E-6);

  if (!check_eigenvalues) {
    cout << "Davidson not converged after " << DS.num_iterations() 
      << " iterations" << endl;
    cout << "Reference eigenvalues" << endl;
    cout << lambda_ref.head(neigen) << endl;
    cout << "Davidson eigenvalues" << endl;
    cout << lambda << endl;
    cout << "Residue norms" << endl;
    cout << DS.residues() << endl;
  }
  else {
    cout << "Davidson converged in "  << DS.num_iterations() 
      << " iterations" << endl;
  }

  BOOST_CHECK_EQUAL(check_eigenvalues, 1);

}


BOOST_AUTO_TEST_SUITE_END()