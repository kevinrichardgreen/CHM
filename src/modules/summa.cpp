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

#include "summa.hpp"
REGISTER_MODULE_CPP(summa);

summa::summa(config_file cfg)
        : module_base("summa", parallel::data, cfg)
{


}


void summa::init(mesh& domain)
{

#pragma omp parallel for
  for (size_t i = 0; i < domain->size_faces(); i++)
  {
    auto face = domain->face(i);

    // Summa class in third_party/summa/summa_interface.hpp
    data* d = face->make_module_data<data>(ID);
    Summa* S = &(d->S);

    // Set up each triangle's data here


  }


}

void summa::run(mesh_elem& face)
{
  // Get the face's data
  data* d = face->get_module_data<data>(ID);
  // And pointer to the SUMMA cpp class
  Summa* S = &(d->S);


}

summa::~summa()
{


}
