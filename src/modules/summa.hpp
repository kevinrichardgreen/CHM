//
// Canadian Hydrological Model - The Canadian Hydrological Model (CHM) is a novel
// modular unstructured mesh based approach for hydrological modelling
// Copyright (C) 2018 Christopher Marsh
//
// This file is part of Canadian Hydrological Model.
//
// Canadian Hydrological Model is free software: you can redistribute it and/or
// modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Canadian Hydrological Model is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Canadian Hydrological Model.  If not, see
// <http://www.gnu.org/licenses/>.
//

#pragma once

#include "logger.hpp"
#include "triangulation.hpp"
#include "module_base.hpp"
#include <meteoio/MeteoIO.h>
#include <cstdlib>
#include <string>

#include <cmath>
#define _USE_MATH_DEFINES
#include <math.h>

/**
 * \addtogroup modules
 * @{
 * \class summa
 * \brief Template header file for new module implementations
 *
 *
 * Depends:
 *
 *
 * Provides:
 *
 */
class summa : public module_base
{
REGISTER_MODULE_HPP(summa);
private:
  double module_variable;
public:
  summa(config_file cfg);
  ~summa();
  virtual void run(mesh_elem& face);
};

/**
@}
*/
