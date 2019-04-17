/*
 *            Copyright 2009-2019 The VOTCA Development Team
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

#ifndef __VOTCA_XTP_POLARSITE_H
#define __VOTCA_XTP_POLARSITE_H

#include <votca/xtp/eigen.h>
#include <votca/xtp/staticsite.h>

namespace votca {
namespace xtp {

/**
\brief Class to represent Atom/Site in electrostatic+polarisation

 The units are atomic units, e.g. Bohr, Hartree.By default a PolarSite cannot be
polarised.
*/
class PolarSite : public StaticSite {

 public:
  struct data {
    int id;
    char* element;
    double posX;
    double posY;
    double posZ;

    int rank;

    double multipoleQ00;
    double multipoleQ11c;
    double multipoleQ11s;
    double multipoleQ10;
    double multipoleQ20;
    double multipoleQ21c;
    double multipoleQ21s;
    double multipoleQ22c;
    double multipoleQ22s;

    double pxx;
    double pxy;
    double pxz;
    double pyy;
    double pyz;
    double pzz;

    double fieldX;
    double fieldY;
    double fieldZ;

    double dipoleX;
    double dipoleY;
    double dipoleZ;

    double dipoleXOld;
    double dipoleYOld;
    double dipoleZOld;

    double eigendamp;
    double phiU;

    operator StaticSite::data() {
      StaticSite::data d2;
      d2.id = id;
      d2.element = element;
      d2.posX = posX;
      d2.posY = posY;
      d2.posZ = posZ;

      d2.rank = rank;

      d2.multipoleQ00 = multipoleQ00;
      d2.multipoleQ11c = multipoleQ11c;
      d2.multipoleQ11s = multipoleQ11s;
      d2.multipoleQ10 = multipoleQ10;
      d2.multipoleQ20 = multipoleQ20;
      d2.multipoleQ21c = multipoleQ21c;
      d2.multipoleQ21s = multipoleQ21s;
      d2.multipoleQ22c = multipoleQ22c;
      d2.multipoleQ22s = multipoleQ22s;
      return d2;
    }
  };

  PolarSite(int id, std::string element, Eigen::Vector3d pos);

  PolarSite(int id, std::string element)
      : PolarSite(id, element, Eigen::Vector3d::Zero()){};

  PolarSite(CptTable& table, const std::size_t& idx) : StaticSite(table, idx) {
    ReadFromCpt(table, idx);
  }

  PolarSite(data& d);

  void setPolarisation(const Eigen::Matrix3d pol) {
    _Ps = pol;
    Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> es;
    es.computeDirect(_Ps, Eigen::EigenvaluesOnly);
    _eigendamp = es.eigenvalues().maxCoeff();
  }

  void ResetInduction() {
    PhiU = 0.0;
    _inducedDipole = Eigen::Vector3d::Zero();
    _inducedDipole_old = Eigen::Vector3d::Zero();
    _localinducedField = Eigen::Vector3d::Zero();
  }

  // MULTIPOLES DEFINITION

  Eigen::Vector3d getDipole() const {
    Eigen::Vector3d dipole = _multipole.segment<3>(1);
    dipole += _inducedDipole;
    return dipole;
  }

  void Rotate(const Eigen::Matrix3d& R, const Eigen::Vector3d& ref_pos) {
    StaticSite::Rotate(R, ref_pos);
    _Ps = R * _Ps * R.transpose();
  }

  void Induce(double wSOR);

  double InductionWork() const {
    return -0.5 * _inducedDipole.transpose() * getField();
  }

  void SetupCptTable(CptTable& table) const;
  void WriteToCpt(CptTable& table, const std::size_t& idx) const;
  void WriteData(data& d) const;

  void ReadFromCpt(CptTable& table, const std::size_t& idx);
  void ReadData(data& d);

  std::string identify() const { return "polarsite"; }

  friend std::ostream& operator<<(std::ostream& out, const PolarSite& site) {
    out << site.getId() << " " << site.getElement() << " " << site.getRank();
    out << " " << site.getPos().x() << "," << site.getPos().y() << ","
        << site.getPos().z() << "\n";
    return out;
  }

 private:
  std::string writePolarisation() const;

  Eigen::Matrix3d _Ps = Eigen::Matrix3d::Zero();
  ;
  Eigen::Vector3d _localinducedField = Eigen::Vector3d::Zero();
  Eigen::Vector3d _inducedDipole = Eigen::Vector3d::Zero();
  Eigen::Vector3d _inducedDipole_old = Eigen::Vector3d::Zero();
  double _eigendamp = 0.0;

  double PhiU = 0.0;  // Electric potential (due to indu.)
};

}  // namespace xtp
}  // namespace votca

#endif
