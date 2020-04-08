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

#include "apdft.h"
#include <votca/xtp/density_integration.h>
#include <votca/xtp/orbitals.h>
#include <votca/xtp/vxc_grid.h>

namespace votca {
namespace xtp {

void APDFT::Initialize(tools::Property &user_options) {

  // get pre-defined default options from VOTCASHARE/xtp/xml/apdft.xml and merge
  // it with the user input
  LoadDefaultsAndUpdateWithUserOptions("xtp", user_options);

  _grid_accuracy =
      _options.ifExistsReturnElseThrowRuntimeError<std::string>(".grid");
  _orbfile =
      _options.ifExistsReturnElseThrowRuntimeError<std::string>(".input");
  _outputfile =
      _options.ifExistsReturnElseThrowRuntimeError<std::string>(".output");
  std::string statestring =
      _options.ifExistsReturnElseThrowRuntimeError<std::string>(".state");
  _state.FromString(statestring);
}

bool APDFT::Evaluate() {

  Orbitals orb;
  orb.ReadFromCpt(_orbfile);
  AOBasis basis = orb.SetupDftBasis();
  Vxc_Grid grid;
  grid.GridSetup(_grid_accuracy, orb.QMAtoms(), basis);

  DensityIntegration<Vxc_Grid> integration(grid);

  integration.IntegrateDensity(orb.DensityMatrixFull(_state));
  std::vector<double> potential_values;
  potential_values.reserve(orb.QMAtoms().size());
  for (const auto &atom : orb.QMAtoms()) {
    potential_values.push_back(integration.IntegratePotential(atom.getPos()));
  }

  std::fstream outfile;
  outfile.open(_outputfile, std::fstream::out);
  outfile << "AtomId, Element, Potential[Hartree]" << std::endl;
  for (Index i = 0; i < orb.QMAtoms().size(); i++) {
    outfile << orb.QMAtoms()[i].getId() << " " << orb.QMAtoms()[i].getElement()
            << " " << std::setprecision(14) << potential_values[i] << std::endl;
  }
  outfile.close();
  return true;
}

}  // namespace xtp
}  // namespace votca
