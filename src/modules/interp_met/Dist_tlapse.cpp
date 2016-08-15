#include "Dist_tlapse.hpp"

Dist_tlapse::Dist_tlapse(config_file cfg)
        :module_base(parallel::data)

{
    provides("t");
    provides("t_lapse_rate");

    depends_from_met("t");
    depends_from_met("t_lapse_rate")

    LOG_DEBUG << "Successfully instantiated module " << this->ID;
}

Dist_tlapse::~Dist_tlapse()
{

}
void Dist_tlapse::init(mesh domain, boost::shared_ptr<global> global_param)
{
#pragma omp parallel for
    for (size_t i = 0; i < domain->size_faces(); i++)
    {
        auto face = domain->face(i);
        auto d = face->make_module_data<data>(ID);
        d->interp.init(global_param->interp_algorithm,global_param->get_stations_in_radius( face->get_x(), face->get_y(), global_param->station_search_radius).size());
    }
}
void Dist_tlapse::run(mesh_elem& face, boost::shared_ptr<global> global_param)
{
    // Distributed forcing lapse_rate to each face (changes per time step)
    //lapse_rate=0.0047;
    
    // Find nearest station

    // Get the lapse rate from that station
    

    //lower all the station values to sea level prior to the interpolation
    std::vector< boost::tuple<double, double, double> > lowered_values;
    for (auto& s : global_param->get_stations_in_radius( face->get_x(), face->get_y(), global_param->station_search_radius))
    {
        if( is_nan(s->get("t")))
            continue;
        double v = s->get("t") - lapse_rate * (0.0 - s->z());
        lowered_values.push_back( boost::make_tuple(s->x(), s->y(), v ) );
    }


    auto query = boost::make_tuple(face->get_x(), face->get_y(), face->get_z());
    double value = face->get_module_data<data>(ID)->interp(lowered_values, query);

    //raise value back up to the face's elevation from sea level
    value =  value + lapse_rate * (0.0 - face->get_z());

    face->set_face_data("t",value);

    face->set_face_data("t_lapse_rate",lapse_rate);

}
