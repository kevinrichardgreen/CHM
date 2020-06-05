/*
 *  SNOWPACK stand-alone
 *
 *  Copyright WSL Institute for Snow and Avalanche Research SLF, DAVOS, SWITZERLAND
*/
/*  This file is part of Snowpack.
    Snowpack is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Snowpack is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Snowpack.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <snowpack/SnowDrift.h>
#include <snowpack/Hazard.h>
#include <snowpack/Utils.h>

#include <vector>
#include <assert.h>

using namespace mio;
using namespace std;

/************************************************************
 * static section                                           *
 ************************************************************/

///Deviation from geometrical factors defined by Schmidt
const double SnowDrift::schmidt_drift_fudge = 1.0;

///Enables erosion notification
const bool SnowDrift::msg_erosion = false;


/************************************************************
 * non-static section                                       *
 ************************************************************/

SnowDrift::SnowDrift(const SnowpackConfig& cfg) : saltation(cfg),
                     enforce_measured_snow_heights(false), snow_redistribution(false), snow_erosion(false), alpine3d(false),
                     sn_dt(0.), nSlopes(0)
{
	cfg.getValue("ALPINE3D", "SnowpackAdvanced", alpine3d);

	// See Snowpack.cc for a description
	cfg.getValue("ENFORCE_MEASURED_SNOW_HEIGHTS", "Snowpack", enforce_measured_snow_heights);

	/*
	 * Number of stations incl. the main station: at least 1, either 3, 5, 7 or 9 for SNOW_REDISTRIBUTION
	 * - 1: real simulation at main station (flat field or slope. In the latter case virtual slopes are somewhat odd (see also PERP_TO_SLOPE)
	 * - 3: real simulation at main station (flat field) plus 2 virtual slopes
	 * - 5: real simulation at main station (flat field) plus 4 virtual slopes
	 * - 7: real simulation at main station (flat field) plus 6 virtual slopes
	 * - 9: real simulation at main station (flat field) plus 8 virtual slopes
	 */
	cfg.getValue("NUMBER_SLOPES", "SnowpackAdvanced", nSlopes);

	// Defines whether real snow erosion at main station or/and redistribution on virtual slopes (default in operational mode)
	// should happen under blowing snow conditions.
	cfg.getValue("SNOW_EROSION", "SnowpackAdvanced", snow_erosion);
	if (nSlopes>1)
		cfg.getValue("SNOW_REDISTRIBUTION", "SnowpackAdvanced", snow_redistribution);

	//Calculation time step in seconds as derived from CALCULATION_STEP_LENGTH
	const double calculation_step_length = cfg.get("CALCULATION_STEP_LENGTH", "Snowpack");
	sn_dt = M_TO_S(calculation_step_length);
}

/**
 * @brief Computes the local mass flux of snow
 * @bug Contribution from suspension not considered yet!
 * @param *Edata
 * @param ustar Shear wind velocity (m s-1)
 * @param angle Slope angle (deg)
 * @return Saltation mass flux (kg m-1 s-1)
 */
double SnowDrift::compMassFlux(const ElementData& Edata, const double& ustar, const double& slope_angle)
{
	// Compute basic quantities that are needed: friction velocity, z0, threshold vw
	// For now assume logarithmic wind profile; TODO change this later
	const double weight = 0.02 * Constants::density_ice * (Edata.sp + 1.) * Constants::g * MM_TO_M(Edata.rg);
	// weight = Edata.Rho*(Edata.sp + 1.)*Constants::g*MM_TO_M(Edata.rg);
	const double sig = 300.;
	const double binding = 0.0015 * sig * Edata.N3 * Optim::pow2(Edata.rb/Edata.rg);
	const double tau_thresh = SnowDrift::schmidt_drift_fudge * (weight + binding);  // Original value for fudge: 1. (Schmidt)
	//const double ustar_thresh = sqrt(tau_thresh / Constants::density_air);
	const double tau = Constants::density_air * Optim::pow2(ustar);

	// First, look whether there is any transport at all: use formulation of Schmidt
	if ( tau_thresh > tau ) {
		return (0.0);
	}

	// Compute the saltation mass flux (after Pomeroy and Gray)
	double Qsalt = 0., Qsusp = 0., c_salt; // The mass fluxes in saltation and suspension (kg m-1 s-1)
	if (!saltation.compSaltation(tau, tau_thresh, slope_angle, MM_TO_M(2.*Edata.rg), Qsalt, c_salt)) {
		prn_msg(__FILE__, __LINE__, "err", Date(), "Saltation computation failed");
		throw IOException("Saltation computation failed", MetAT);
	}

	if (Qsalt > 0.) {
		Qsusp = 0.; // TODO What about sd_IntegrateFlux(ustar, ustar_thresh, Qsalt, z0); ???
	} else {
		Qsalt = 0.;
		Qsusp = 0.;
	}

	return (Qsalt + Qsusp);
}

/**
 * @brief Erodes Elements from the top and computes the associated mass flux
 * Even so the code is quite obscure, it should cover all of the following cases:
 * -# Externally provided eroded mass (for example, by Alpine3D); see parameter forced_massErode
 * -# SNOW_REDISTRIBUTION is true: using vw_drift to erode the snow surface on the windward virtual slope
 * -# SNOW_EROSION is true: using vw to erode the snow surface at the main station (flat field or slope).
 * 	@note However, erosion will also be controlled by mH and thus a measured snow depth (HS1) is required
 * 	@note If either measured snow depth is missing or the conditions for a real erosion are not fulfilled,
 *          the possibility of a virtual erosion will be considered using the ErosionLevel marker (virtual erodible layer).
 * @param Mdata
 * @param Xdata
 * @param Sdata
 * @param forced_massErode if greater than 0, force the eroded mass to the given value (instead of computing it)
*/
void SnowDrift::compSnowDrift(const CurrentMeteo& Mdata, SnowStation& Xdata, SurfaceFluxes& Sdata, double& forced_massErode)
{
	size_t nE = Xdata.getNumberOfElements();
	vector<NodeData>& NDS = Xdata.Ndata;
	vector<ElementData>& EMS = Xdata.Edata;

	const bool no_snow = ((nE < Xdata.SoilNode+1) || (EMS[nE-1].theta[SOIL] > 0.));
	const bool no_wind_data = (Mdata.vw_drift == mio::IOUtils::nodata);
	if (no_snow || no_wind_data) {
		Xdata.ErosionMass = 0.;
		if (no_snow) {
			Xdata.ErosionLevel = Xdata.SoilNode;
			Sdata.drift = 0.;
		} else
			Sdata.drift = Constants::undefined;
		return;
	}

	// Real erosion either on windward virtual slope, from Alpine3D, or at main station.
	// At main station, measured snow depth controls whether erosion is possible or not
	const bool windward = !alpine3d && snow_redistribution && Xdata.windward; // check for windward virtual slope
	const bool erosion = snow_erosion && (Xdata.mH > (Xdata.Ground + Constants::eps)) && ((Xdata.mH + 0.02) < Xdata.cH);
	if (windward || alpine3d || erosion) {
		double massErode=0.; // Mass loss due to erosion

		if (fabs(forced_massErode) > Constants::eps2) {
			massErode = std::max(0., -forced_massErode); //negative mass is erosion
		} else {
            massErode = 0;

            //No, we don't want to do this
//			const double ustar_max = (Mdata.vw>0.1) ? Mdata.ustar * Mdata.vw_drift / Mdata.vw : 0.; // Scale Mdata.ustar
//			try {
//				if (enforce_measured_snow_heights && !windward)
//					Sdata.drift = compMassFlux(EMS[nE-1], Mdata.ustar, Xdata.meta.getSlopeAngle()); // kg m-1 s-1, main station, local vw && nE-1
//				else
//					Sdata.drift = compMassFlux(EMS[nE-1], ustar_max, Xdata.meta.getSlopeAngle()); // kg m-1 s-1, windward slope && vw_drift && nE-1
//			} catch(const exception&) {
//					prn_msg(__FILE__, __LINE__, "err", Mdata.date, "Cannot compute mass flux of drifting snow!");
//					throw;
//			}
//			massErode = Sdata.drift * sn_dt / Hazard::typical_slope_length; // Convert to eroded snow mass in kg m-2
		}

		Xdata.ErosionMass = 0;

        //don't remove more mass than what exists
        double totalmass = 0;
//        totalmass = Xdata.swe;

        for(int i =0; i<nE;i++)
        {
            totalmass += EMS[i].M;
        }

        if (massErode > totalmass)
            massErode = totalmass;

		while (massErode > 0)
		{
			unsigned int nErode = 0; // number of eroded elements
			if (massErode >= 0.95 * EMS[nE - 1].M)
			{
				// Erode at most one element with a maximal error of +- 5 % on mass ...
				if (windward)
					Xdata.rho_hn = EMS[nE - 1].Rho;
				nE--;
				Xdata.cH -= EMS[nE].L;
                Xdata.cH = std::max(0.0,Xdata.cH); // ensure we aren't ever so slightly negative
				NDS[nE].hoar = 0.;
				Xdata.ErosionMass += EMS[nE].M;
				Xdata.ErosionLevel = std::min(nE - 1, Xdata.ErosionLevel);
				nErode++;
				massErode -= EMS[nE].M; //reduce total mass to erode by what we removed
//				forced_massErode -= massErode;

			} else if (massErode > Constants::eps)
			{ // ... or take away massErode from top element - partial real erosion
				if (fabs(EMS[nE - 1].L * EMS[nE - 1].Rho - EMS[nE - 1].M) > 0.001)
				{
					prn_msg(__FILE__, __LINE__, "wrn", Mdata.date, "Inconsistent Mass:%lf   L*Rho:%lf", EMS[nE - 1].M,
							EMS[nE - 1].L * EMS[nE - 1].Rho);
					EMS[nE - 1].M = EMS[nE - 1].L * EMS[nE - 1].Rho;
					assert(EMS[nE - 1].M >= 0.); //mass must be positive
				}
				if (windward)
					Xdata.rho_hn = EMS[nE - 1].Rho; // Density of drifting snow on virtual luv slope
				const double dL = -massErode / (EMS[nE - 1].Rho);
				NDS[nE].z += dL;
				EMS[nE - 1].L0 = EMS[nE - 1].L = EMS[nE - 1].L + dL;
				Xdata.cH += dL;
                Xdata.cH = std::max(0.0,Xdata.cH); // ensure we aren't ever so slightly negative
				NDS[nE].z += NDS[nE].u;
				NDS[nE].u = 0.0;
				NDS[nE].hoar = 0.;
				EMS[nE - 1].M -= massErode;
				assert(EMS[nE - 1].M >= 0.); //mass must be positive
				Xdata.ErosionMass += massErode;

				massErode=0;
//				forced_massErode = 0.;
			} else
			{
				massErode=0; // we done
				Xdata.ErosionMass = 0.;
			}
			if (nErode > 0)
				Xdata.resize(nE);

            if(nE == 0)
                break;
		}

//		if (!alpine3d && SnowDrift::msg_erosion) { //messages on demand but not in Alpine3D
//			if (Xdata.ErosionMass > 0.) {
//				if (windward)
//					prn_msg(__FILE__, __LINE__, "msg+", Mdata.date, "Eroding %d layer(s) w/ total mass %.3lf kg/m2 (windward: azi=%.1lf, slope=%.1lf)", nErode, Xdata.ErosionMass, Xdata.meta.getAzimuth(), Xdata.meta.getSlopeAngle());
//				else
//					prn_msg(__FILE__, __LINE__, "msg+", Mdata.date, "Eroding %d layer(s) w/ total mass %.3lf kg/m2 (azi=%.1lf, slope=%.1lf)", nErode, Xdata.ErosionMass, Xdata.meta.getAzimuth(), Xdata.meta.getSlopeAngle());
//			}
//		}
	// ... or, in case of no real erosion, check whether you can potentially erode at the main station.
	// This will never contribute to the drift index VI24, though!
	} else if (snow_erosion && (Xdata.ErosionLevel > Xdata.SoilNode)) {
		double virtuallyErodedMass = compMassFlux(EMS[Xdata.ErosionLevel], Mdata.ustar, Xdata.meta.getSlopeAngle());  // kg m-1 s-1, main station, local vw && erosion level
		virtuallyErodedMass *= sn_dt / Hazard::typical_slope_length; // Convert to eroded snow mass in kg m-2
		if (virtuallyErodedMass > Constants::eps) {
			// Add (negative) value stored in Xdata.ErosionMass
			if (Xdata.ErosionMass < -Constants::eps)
				virtuallyErodedMass -= Xdata.ErosionMass;
			Sdata.mass[SurfaceFluxes::MS_WIND] = std::min(virtuallyErodedMass, EMS[Xdata.ErosionLevel].M); // use MS_WIND to carry virtually eroded mass
			// Now keep track of mass that either did or did not lead to erosion of full layer
			if (virtuallyErodedMass > EMS[Xdata.ErosionLevel].M) {
				virtuallyErodedMass -= EMS[Xdata.ErosionLevel].M;
				Xdata.ErosionLevel--;
			}
			Xdata.ErosionMass = -virtuallyErodedMass;
			Xdata.ErosionLevel = std::max(Xdata.SoilNode, std::min(Xdata.ErosionLevel, nE-1));
		} else {
			Xdata.ErosionMass = 0.;
		}
		if (!alpine3d && SnowDrift::msg_erosion) { //messages on demand but not in Alpine3D
			if (Xdata.ErosionLevel > nE-1)
				prn_msg(__FILE__, __LINE__, "wrn", Mdata.date, "Virtual erosion: ErosionLevel=%d did get messed up (nE-1=%d)", Xdata.ErosionLevel, nE-1);
		}
	} else {
		Xdata.ErosionMass = 0.;
	}
}
