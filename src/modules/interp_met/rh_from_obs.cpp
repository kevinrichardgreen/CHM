
#include "rh_from_obs.h"

rh_from_obs::rh_from_obs()
:module_base(parallel::data)
{
    depends_from_met("rh");
    depends_from_met("t");
    depends("t");
    provides("rh");
}
rh_from_obs::~rh_from_obs()
{

}

double esat(double TA)
{

    double Es, E, Rhi, Rhw, Rh;                         //saturation and current water vapro pressure
    const double Aw = 611.21, Bw = 17.502, Cw = 240.97; //parameters for water
    const double Ai = 611.15, Bi = 22.452, Ci = 272.55; //parameters for ice
    const double Tfreeze = 0.;                          //freezing temperature

    if (TA >= Tfreeze )
    {
        //above freezing point, water
        Es = Aw * exp((Bw * TA) / (Cw + TA));
    }
    else
    {
        Es = Ai * exp( (Bi * TA) / (Ci + TA) );
    }
    return Es;
}
void rh_from_obs::run(mesh_elem& elem, boost::shared_ptr<global> global_param)
{
    //generate lapse rates
    std::vector<double> sea;
    std::vector<double> sz;

    static boost::posix_time::ptime last_update;
    static double lapse=-999.0;

    //we have to run this at least once, use the first bool flag to do so
    //otherwise just check that the last update was at a different time step, if so calculate the lapse rate.
    //otherwise, just used the stored lapse rate
    if(last_update != global_param->posix_time() )
    {
        for (auto& s : global_param->stations)
        {
            double rh = s->get("rh")/100.;
            double t = s->get("t");
            double es = esat(t);
            double ea = rh * es;
            sea.push_back( ea  );
            sz.push_back( s->z());
        }


        // least squares linear fit to these points ( p v. z)
        double c0, c1, cov00, cov01, cov11, chisq;

        gsl_fit_linear (&sz[0], 1,&sea[0], 1, sz.size(),
                        &c0, &c1, &cov00, &cov01, &cov11,
                        &chisq);
        lapse = c1;//use the slope (y=mx+b c1 == m)
        last_update = global_param->posix_time();
    }

    std::vector< boost::tuple<double, double, double> > lowered_values;
    for (auto& s : global_param->stations)
    {
        double rh = s->get("rh")/100.;
        double t = s->get("t");
        double es = esat(t);
        double ea = rh * es;
        double z = s->z();
        ea = ea + lapse*(0.0-z);
        lowered_values.push_back( boost::make_tuple(s->x(), s->y(), ea ) );

    }

    interp_base* interp=nullptr;
    std::string interp_method = "spline";
    if(interp_method == "spline")
        interp = new thin_plate_spline();

    auto query = boost::make_tuple(elem->get_x(), elem->get_y(), elem->get_z());
    double ea = (*interp)(lowered_values, query);

    //raise it back up
    ea = ea + lapse*( elem->get_z() - 0.0);

    double es = esat(elem->face_data("t"));
    double rh = ea/es*100.0;

    rh = std::min(rh,100.0);
    rh = std::max(10.0,rh);

    elem->set_face_data("rh",rh);


}