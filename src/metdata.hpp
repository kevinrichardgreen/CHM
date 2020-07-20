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

// spatial tree  -- cgal includes
#include <CGAL/Simple_cartesian.h>
#include <CGAL/Kd_tree.h>
#include <CGAL/algorithm.h>
#include <CGAL/Fuzzy_sphere.h>
#include <CGAL/Search_traits_2.h>
#include <CGAL/Orthogonal_k_neighbor_search.h>
#include <CGAL/Splitters.h>
#include <CGAL/Euclidean_distance.h>

//std includes
#include <string>
#include <set>
#include <unordered_set>
#include <vector>

//boost includes
#include <boost/function.hpp>
#include <boost/date_time/posix_time/posix_time.hpp> // for boost::posix

//Gdal includes
#include <ogr_spatialref.h>

// vtk includes for station output into vtk format
#include <vtkXMLPolyDataWriter.h>
#include <vtkPoints.h>
#include <vtkStringArray.h>
#include <vtkPolyData.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkPointData.h>

//CHM includes
#include "exception.hpp"
#include "logger.hpp"
#include "station.hpp"
#include "netcdf.hpp"
#include "timeseries.hpp"
#include "triangulation.hpp"
#include "filter_base.hpp"
/**
 * Main meteorological data coordinator. Opens from a variety of sources and ensures that each virtual station has this timestep's information
 * regardless of the source data type.
 */
class metdata
{
  public:

    /// Holds the metadata for an ascii file to load
    ///
    class ascii_metdata
    {
      public:
        ascii_metdata()
        {
            path = "";
            id = "";

            latitude = longitude = elevation = -9999;
        }

        double latitude, longitude, elevation;
        std::string path;
        std::string id;


        //if we use text file inputs, each station can have its own filer (ie., winds at different heights). So we need to save the filter
        //and run it on a per-station config.
        std::vector<boost::shared_ptr<filter_base>> filters;
    };


//    metdata() = delete;

    metdata(std::string mesh_proj4);

    ~metdata();

    /// Loads a netcdf file. Must be a 2D structured grid of stations. Expects times to be in UTC+0
    /// @param path
    /// @param filters
    void load_from_netcdf(const std::string& path, std::map<std::string, boost::shared_ptr<filter_base> > filters = {});

    /// Loads the standard ascii timeseries. Needs to be in UTC+0
    /// @param path
    /// @param filters
    /// @param utc_offset Positive offset going west. So the normal UTC-6 would be UTC_offset:6
    void load_from_ascii(std::vector<ascii_metdata> stations, int utc_offset);

    void write_stations_to_ptv(const std::string& path);

    /**
      * Returns a set of stations within the search radius (meters) centered on the point x,y
    * @param x
    * @param y
    * @param radius
    * @return List stations that satisfy search criterion
    */
    std::vector< std::shared_ptr<station> > get_stations_in_radius(double x, double y,double radius);

    /**
     * Returns the nearest station to x,y. Ignores elevation
     * @param x
     * @param y
     * @param N Number neighbors to find
     * @return
     */
    std::vector< std::shared_ptr<station> > nearest_station(double x, double y,unsigned int N=1);

    /// Return a list of stations for a point x,y corresponding to a search radius, or nearest station
    boost::function< std::vector< std::shared_ptr<station> > ( double, double) > get_stations;

    /// Number of stations
    /// @return
    size_t nstations();

    /// Number of timesteps
    /// @return
    size_t n_timestep();

    std::shared_ptr<station> at(size_t idx);

    boost::posix_time::ptime current_time();
    boost::posix_time::ptime start_time();
    boost::posix_time::ptime end_time();

    std::string current_time_str();
    std::string start_time_str();
    std::string end_time_str();

    /// Subsets all timeseries to begin at [start, end]. For ascii, the underlying timeseries is modified.
    /// For nc, internal offsets are computed to start, end.
    /// This updates the internal start and end times, as well as resets the current time to be = start
    /// @param start
    /// @param end
    void subset(boost::posix_time::ptime start, boost::posix_time::ptime end);

    /// Returns the start and endtime of the timeseries.
    std::pair<boost::posix_time::ptime,boost::posix_time::ptime> start_end_time();

    /// Check that all ascii stations have the same start/end times
    /// @return
    void check_ts_consistency();

    /// Timestep duration. Use .dt_seconds() to total seconds
    /// @return
    boost::posix_time::time_duration dt();

    size_t dt_seconds();

    /// Populates the stations' with the next timesteps' value.
    /// @return False if no more timesteps
    bool next();

    /// Removes a subset of stations from the  station list
    /// @param stations The set of station IDs to remove
    void prune_stations(std::unordered_set<std::string>& station_ids);

    /**
    * List all (including module provided) variables. If ascii files are loaded, this includes variables present in one 1 met file.
    * \return set containing a list of variable names
    */
    std::set<std::string> list_variables();

    std::vector< std::shared_ptr<station>>& stations();

  private:

    struct ascii_data
    {
        //if we use text file inputs, each station can have its own filer (ie., winds at different heights). So we need to save the filter
        //and run it on a per-station config.
        std::vector<boost::shared_ptr<filter_base>> filters;

        std::string id;
        // these are loaded into by metdata. Essentially this becomes like the old station
        timeseries _obs;
        timeseries::iterator _itr;
    };

    /// Advances 1 timestep in the netcdf files
    bool next_nc();


    /// Advances 1 timestep from the ascii timeseries
    /// @return
    bool next_ascii();

    /// For all the stations loaded from ascii files, find the latest start time, and the earliest end time that is consistent across all stations
    /// @return
    std::pair<boost::posix_time::ptime, boost::posix_time::ptime> find_unified_start_end();


    // NetCDF specific variables
    // -----------------------------------
        //if we use netcdf, store it here
        std::unique_ptr<netcdf> _nc;

        //if we use netcdf, we need to save the filters and run it once every timestep.
        std::map<std::string, boost::shared_ptr<filter_base>>_netcdf_filters;

        std::set<std::string> _provides_from_nc_filters;

        // if false, we are using ascii files
        bool _use_netcdf;

    // -----------------------------------
    // ASCII met data specific variables

        //Essentially what the old stations turned into.
        // Holds all the met data to init that stations + the underlying timeseries data
        // Mapped w/ stations ID -> metdata
        std::map<std::string, std::unique_ptr<ascii_data>> _ascii_stations;

    // -----------------------------------

    // This is a different approach than how stations used to work
    // Now, they only hold the current timestep, which is refilled every model timestep by metdata
    // Our sources of data are netcdf, or ascii (or whatever in the future)
    std::vector< std::shared_ptr<station>> _stations;


    // Total number of stations
    size_t _nstations;

    bool is_first_timestep;

    //number of timesteps
    size_t _n_timesteps;

    boost::posix_time::ptime _start_time, _end_time;
    boost::posix_time::ptime _current_ts;
    boost::posix_time::time_duration _dt;

    // computes the dt
    void compute_dt();

    //all variables provided by met + filter
    std::set<std::string> _variables;

    //holds the proj4 string of the mesh. we need this to be able to reproject input data to the mesh
    std::string _mesh_proj4;
    bool _is_geographic; // geographic mesh that requires further reprojection?

    // spatial searching data structure
    typedef CGAL::Simple_cartesian<double> Kernel;
    typedef boost::tuple<Kernel::Point_2, std::shared_ptr<station> >  Point_and_station;
    typedef CGAL::Search_traits_2<Kernel> Traits_base;
    typedef CGAL::Search_traits_adapter<Point_and_station,
        CGAL::Nth_of_tuple_property_map<0, Point_and_station>,
        Traits_base>                                              Traits;


    //Sliding_midpoint spatial search tree. Better stability for searching with the coordinate systems we use
    typedef CGAL::Sliding_midpoint<Traits> Splitter;
    typedef CGAL::Kd_tree<Traits,Splitter> Tree;

    // used for get_stations_in_radius
    typedef CGAL::Fuzzy_sphere<Traits> Fuzzy_circle;

    // This is used by nearest_station to find a single nearest station
    typedef CGAL::Orthogonal_k_neighbor_search <
        Traits,
        typename CGAL::internal::Spatial_searching_default_distance<Traits>::type,
        Splitter > Neighbor_search;

    Tree _dD_tree; //spatial query tree


};

