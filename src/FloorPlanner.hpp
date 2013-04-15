/*
 * =====================================================================================
 *
 *    Description:  Corblivar floorplanner (SA operations and related handler)
 *
 *         Author:  Johann Knechtel, johann.knechtel@ifte.de
 *        Company:  Institute of Electromechanical and Electronic Design, www.ifte.de
 *
 * =====================================================================================
 */
#ifndef _CORBLIVAR_FLOORPLANNER
#define _CORBLIVAR_FLOORPLANNER

// library includes
#include "Corblivar.incl.hpp"
// Corblivar includes, if any
#include "Block.hpp"
#include "Net.hpp"
#include "ThermalAnalyzer.hpp"
// forward declarations, if any
class CorblivarCore;

class FloorPlanner {
	// debugging code switch (private)
	private:
		static constexpr bool DBG_SA = false;
		static constexpr bool DBG_CALLS_SA = false;
		static constexpr bool DBG_LAYOUT = false;

	// private data, functions
	private:
		// chip data
		mutable vector<Block> blocks;
		mutable vector<Block> terminals;
		mutable vector<Net> nets;

		// config parameters
		int conf_layer;
		int conf_log;
		double conf_outline_x, conf_outline_y;
		double outline_AR;
		double conf_blocks_scale;

		// PODs
		struct Cost {
			double cost;
			bool fits_fixed_outline;

			// http://www.learncpp.com/cpp-tutorial/93-overloading-the-io-operators/
			friend ostream& operator<< (ostream& out, Cost const& cost) {
				out << "cost=" << cost.cost << ", fits_fixed_outline=" << cost.fits_fixed_outline;
				return out;
			}
		};
		struct CostInterconn {
			double HPWL;
			double TSVs;

			friend ostream& operator<< (ostream& out, CostInterconn const& cost) {
				out << "HPWL=" << cost.HPWL << ", TSVs=" << cost.TSVs;
				return out;
			}
		};
		struct TempStep {
			int step;
			double temp;
			bool new_best_sol_found;
		};

		// IO
		string benchmark, blocks_file, power_density_file, nets_file;
		ofstream results, solution_out;
		ifstream solution_in;
		struct timeb start;
		// flag whether power density file is available / was handled / thermal
		// analysis should be performed / thermal files should be generated
		bool power_density_file_avail;

		// logging
		static constexpr int LOG_MINIMAL = 1;
		static constexpr int LOG_MEDIUM = 2;
		static constexpr int LOG_MAXIMUM = 3;

		// SA config parameters
		double conf_SA_loopFactor, conf_SA_loopLimit;
		double conf_SA_cost_temp, conf_SA_cost_WL, conf_SA_cost_TSVs, conf_SA_cost_area_outline;

		// SA parameters: temperature-scaling factors
		double conf_SA_temp_factor_phase1, conf_SA_temp_factor_phase1_limit, conf_SA_temp_factor_phase2;

		// SA parameter: scaling factor for loops during solution-space sampling
		static constexpr int SA_SAMPLING_LOOP_FACTOR = 1;

		// SA parameter: scaling factor for initial temp
		static constexpr double SA_INIT_TEMP_FACTOR = 20;

		// SA: temperature-schedule log data
		mutable vector<TempStep> tempSchedule;

		// SA: layout-operation handler variables
		mutable int last_op, last_op_die1, last_op_die2, last_op_tuple1, last_op_tuple2, last_op_juncts;

		// SA: layout-operation handler
		bool performRandomLayoutOp(CorblivarCore const& corb, bool const& revertLastOp = false) const;

		// SA: cost functions, i.e., layout-evalutions
		Cost determCost(double const& ratio_feasible_solutions_fixed_outline = 0.0, bool const& phase_two = false, bool const& set_max_cost = false) const;
		inline double determCostThermalDistr(bool const& set_max_cost = false, bool const& normalize = true) const {
			this->thermalAnalyzer.generatePowerMaps(this->conf_layer, this->blocks, this->conf_outline_x, this->conf_outline_y);
			return this->thermalAnalyzer.performPowerBlurring(this->conf_layer, this->max_cost_temp, set_max_cost, normalize);
		};
		Cost determCostAreaOutline(double const& ratio_feasible_solutions_fixed_outline = 0.0) const;
		CostInterconn determCostInterconnects(bool const& set_max_cost = false, bool const& normalize = true) const;

		// SA parameters: max cost values
		mutable double max_cost_temp, max_cost_WL, max_cost_TSVs, max_cost_alignments;

		// SA: helper for main handler
		// note that various parameters are return-by-reference
		void initSA(CorblivarCore const& corb, vector<double>& cost_samples, int& innerLoopMax, double& init_temp);
		inline void updateTemp(double& cur_temp, int const& iteration, int const& iteration_first_valid_layout) const;

		// thermal analyzer
		ThermalAnalyzer thermalAnalyzer;

		// ThermalAnalyzer parameters: mask fitting
		double conf_power_blurring_impulse_factor, conf_power_blurring_impulse_factor_scaling_exponent, conf_power_blurring_mask_boundary_value;

	// constructors, destructors, if any non-implicit
	public:

	// public data, functions
	public:
		friend class IO;

		// logging
		inline bool logMin() const {
			return (this->conf_log >= LOG_MINIMAL);
		};
		inline bool logMed() const {
			return (this->conf_log >= LOG_MEDIUM);
		};
		inline bool logMax() const {
			return (this->conf_log >= LOG_MAXIMUM);
		};

		// ThermalAnalyzer: handler
		inline void initThermalAnalyzer() {
			// init masks parameters
			ThermalAnalyzer::MaskParameters parameters;
			parameters.impulse_factor = this->conf_power_blurring_impulse_factor;
			parameters.impulse_factor_scaling_exponent = this->conf_power_blurring_impulse_factor_scaling_exponent;
			parameters.mask_boundary_value = this->conf_power_blurring_mask_boundary_value;

			// init thermal masks
			this->thermalAnalyzer.initThermalMasks(this->conf_layer, this->logMed(), parameters);

			// init power maps, i.e. predetermine maps parameters
			this->thermalAnalyzer.initPowerMaps(this->conf_layer, this->conf_outline_x, this->conf_outline_y);
		};

		// getter / setter
		inline int const& getLayers() const {
			return this->conf_layer;
		};

		inline vector<Block> const& getBlocks() const {
			return this->blocks;
		};

		inline void setTimeStart() {
			ftime(&(this->start));
		};

		inline bool inputSolutionFileOpen() const {
			return this->solution_in.is_open();
		};

		// SA: handler
		bool performSA(CorblivarCore const& corb);
		void finalize(CorblivarCore const& corb, bool const& determ_overall_cost = true);
};

#endif
