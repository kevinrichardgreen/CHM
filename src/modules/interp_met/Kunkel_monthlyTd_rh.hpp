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
#include <cstdlib>
#include <string>
#include <cmath>
#include <math.h>

#include <meteoio/MeteoIO.h>

/**
 * \ingroup modules rh met
 * @{
 * \class Kunkel_monthlyTd_rh
 * Monthly-variable linear lapse rate adjustment for relative humidity based upon Kunkel (1989).
 * RH is lapsed via dew point temperatures.
 *
 * **Depends:**
 * -  Air temperature "t" [\f$  {}^\circ C \f$]
 *
 * **Depends from Met:**
 * - Relative Humidity "rh" [\f$ % \f$]
 *
 * **Provides:**
 * - Relative humidity "rh" [\f$ % \f$]
 *
 * **Configuration keys:**
 * - None
 *
 * **Reference:**
 * Kunkel, K. E. (1989). Simple procedures for extrapolation of humidity variables in the mountainous western United States.
 * Journal of Climate, 2(7), 656–669.
 *
 * @}
 */
class Kunkel_monthlyTd_rh : public module_base
{
REGISTER_MODULE_HPP(Kunkel_monthlyTd_rh);
public:
    Kunkel_monthlyTd_rh(config_file cfg);
    ~Kunkel_monthlyTd_rh();
    virtual void run(mesh_elem& face);
    virtual void init(mesh& domain);
    struct data : public face_info
    {
        interpolation interp;
    };
};
