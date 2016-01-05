#include "core.h"
#include <algorithm>
#include <sstream>


  
static var_info vtab_utility_rate3_mat[] = {

/*   VARTYPE           DATATYPE         NAME                         LABEL                                           UNITS     META                      GROUP          REQUIRED_IF                 CONSTRAINTS                      UI_HINTS*/
	{ SSC_INPUT,        SSC_NUMBER,     "analysis_period",           "Number of years in analysis",                   "years",  "",                      "",             "*",                         "INTEGER,POSITIVE",              "" },

	{ SSC_INPUT, SSC_NUMBER, "system_use_lifetime_output", "Lifetime hourly system outputs", "0/1", "0=hourly first year,1=hourly lifetime", "", "*", "INTEGER,MIN=0,MAX=1", "" },

	// First year or lifetime hourly or subhourly
	// load and gen expected to be > 0
	// grid positive if system generation > load, negative otherwise
	{ SSC_INPUT, SSC_ARRAY, "gen", "System power generated", "kW", "", "Time Series", "*", "", "" },
	{ SSC_INPUT, SSC_ARRAY, "load", "Electricity load (year 1)", "kW", "", "Time Series", "*", "", "" },

	{ SSC_INPUT, SSC_NUMBER, "inflation_rate", "Inflation rate", "%", "", "Financials", "*", "MIN=0,MAX=100", "" },

	{ SSC_INPUT, SSC_ARRAY, "degradation", "Annual energy degradation", "%", "", "AnnualOutput", "*", "", "" },
	{ SSC_INPUT, SSC_ARRAY, "load_escalation", "Annual load escalation", "%/year", "", "", "?=0", "", "" },
	{ SSC_INPUT,        SSC_ARRAY,      "rate_escalation",          "Annual utility rate escalation",  "%/year", "",                      "",             "?=0",                       "",                              "" },
	{ SSC_INPUT, SSC_NUMBER, "ur_metering_option", "Metering options", "0=Net metering rollover monthly excess energy (kWh),1=Net metering rollover monthly excess dollars ($),2=Non-net metering monthly reconciliation,3=Non-net metering hourly reconciliation", "Net metering monthly excess", "", "?=0", "INTEGER", "" },

	// 0 to match with 2015.1.30 release, 1 to use most common URDB kWh and 1 to user daily kWh e.g. PG&E baseline rates.
	{ SSC_INPUT, SSC_NUMBER, "ur_ec_ub_units", "Energy charge tier upper bound units", "0=hourly,1=monthly,2=daily", "Non-net metering hourly tier energy", "", "?=0", "INTEGER", "" },
	// 0 to use previous version sell rates and 1 to use single sell rate, namely flat sell rate
	{ SSC_INPUT, SSC_NUMBER, "ur_ec_sell_rate_option", "Energy charge sell rate option", "0=Sell excess at energy charge sell rates,1=sell excess at specified sell rate", "Non-net metering sell rate", "", "?=0", "INTEGER", "" },

	{ SSC_INPUT, SSC_NUMBER, "ur_ec_single_sell_rate", "Single TOU sell rate", "$/kWh", "", "", "?=0.0", "", "" },


	{ SSC_INPUT, SSC_NUMBER, "ur_nm_yearend_sell_rate", "Year end sell rate", "$/kWh", "", "", "?=0.0", "", "" },
	{ SSC_INPUT,        SSC_NUMBER,     "ur_monthly_fixed_charge",  "Monthly fixed charge",            "$",      "",                      "",             "?=0.0",                     "",                              "" },
	{ SSC_INPUT,        SSC_NUMBER,     "ur_flat_buy_rate",         "Flat rate (buy)",                 "$/kWh",  "",                      "",             "*",                         "",                              "" },
	{ SSC_INPUT,        SSC_NUMBER,     "ur_flat_sell_rate",        "Flat rate (sell)",                "$/kWh",  "",                      "",             "?=0.0",                     "",      "" },

	// urdb minimums
	{ SSC_INPUT, SSC_NUMBER, "ur_monthly_min_charge", "Monthly minimum charge", "$", "", "", "?=0.0", "", "" },
	{ SSC_INPUT, SSC_NUMBER, "ur_annual_min_charge", "Annual minimum charge", "$", "", "", "?=0.0", "", "" },



	// Energy Charge Inputs
	{ SSC_INPUT,        SSC_NUMBER,     "ur_ec_enable",            "Enable energy charge",        "0/1",    "",                      "",             "?=0",                       "BOOLEAN",                       "" },

	{ SSC_INPUT, SSC_MATRIX, "ur_ec_sched_weekday", "Energy Charge Weekday Schedule", "", "12x24", "", "ur_ec_enable=1", "", "" },
	{ SSC_INPUT, SSC_MATRIX, "ur_ec_sched_weekend", "Energy Charge Weekend Schedule", "", "12x24", "", "ur_ec_enable=1", "", "" },

	// ur_ec_tou_mat has 6 columns period, tier, max usage, max usage units, buy rate, sell rate
	// replaces 12(P)*6(T)*(max usage+buy+sell) = 216 single inputs
	{ SSC_INPUT, SSC_MATRIX, "ur_ec_tou_mat", "Energy Charge TOU Inputs", "", "", "", "ur_ec_enable=1", "", "" },
	// Demand Charge Inputs
	{ SSC_INPUT,        SSC_NUMBER,     "ur_dc_enable",            "Enable Demand Charge",        "0/1",    "",                      "",             "?=0",                       "BOOLEAN",                       "" },
	// TOU demand charge
	{ SSC_INPUT, SSC_MATRIX, "ur_dc_sched_weekday", "Demend Charge Weekday Schedule", "", "12x24", "", "", "", "" },
	{ SSC_INPUT, SSC_MATRIX, "ur_dc_sched_weekend", "Demend Charge Weekend Schedule", "", "12x24", "", "", "", "" },

	// ur_dc_tou_mat has 4 columns period, tier, peak demand (kW), demand charge
	// replaces 12(P)*6(T)*(peak+charge) = 144 single inputs
	{ SSC_INPUT, SSC_MATRIX, "ur_dc_tou_mat", "Demand Charge TOU Inputs", "", "", "", "ur_dc_enable=1", "", "" },


	// flat demand charge
	// ur_dc_tou_flat has 4 columns month, tier, peak demand (kW), demand charge
	// replaces 12(P)*6(T)*(peak+charge) = 144 single inputs
	{ SSC_INPUT, SSC_MATRIX, "ur_dc_flat_mat", "Demand Charge Flat Inputs", "", "", "", "ur_dc_enable=1", "", "" },
	

	// outputs
//	{ SSC_OUTPUT,       SSC_ARRAY,      "energy_value",             "Energy value in each year",     "$",    "",                      "",             "*",                         "",   "" },
	{ SSC_OUTPUT,       SSC_ARRAY,      "annual_energy_value",             "Energy value in each year",     "$",    "",                      "Annual",             "*",                         "",   "" },
	{ SSC_OUTPUT,       SSC_ARRAY,      "annual_electric_load",            "Total electric load in each year",  "kWh",    "",                      "Annual",             "*",                         "",   "" },

	// use output from annualoutput not scaled output from here
	//	{ SSC_OUTPUT,       SSC_ARRAY,      "energy_net",               "Energy in each year",           "kW",   "",                      "",             "*",                         "",   "" },


		// outputs from Paul, Nate and Sean 9/9/13
//	{ SSC_OUTPUT,       SSC_ARRAY,      "revenue_with_system",      "Total revenue with system",         "$",    "",                      "",             "*",                         "",   "" },
//	{ SSC_OUTPUT,       SSC_ARRAY,      "revenue_without_system",   "Total revenue without system",      "$",    "",                      "",             "*",                         "",   "" },
	{ SSC_OUTPUT, SSC_ARRAY, "elec_cost_with_system",    "Electricity cost with system",    "$/yr", "", "Annual", "*", "", "" },
	{ SSC_OUTPUT, SSC_ARRAY, "elec_cost_without_system", "Electricity cost without system", "$/yr", "", "Annual", "*", "", "" },

	// year 1 values for metrics
	{ SSC_OUTPUT, SSC_NUMBER, "elec_cost_with_system_year1",    "Electricity cost with system",    "$/yr", "",    "Financial Metrics", "*", "", "" },
	{ SSC_OUTPUT, SSC_NUMBER, "elec_cost_without_system_year1", "Electricity cost without system", "$/yr", "",    "Financial Metrics", "*", "", "" },
	{ SSC_OUTPUT, SSC_NUMBER, "savings_year1",                  "Electricity savings",             "$/yr",    "", "Financial Metrics", "*", "", "" },
	{ SSC_OUTPUT, SSC_NUMBER, "year1_electric_load",            "Electricity load",                "kWh/yr",  "", "Financial Metrics", "*", "", "" },



//	{ SSC_OUTPUT,       SSC_ARRAY,      "year1_hourly_e_grid",         "Year 1 electricity to/from grid",       "kWh", "",                      "",             "*",                         "LENGTH=8760",                   "" },
	{ SSC_OUTPUT,       SSC_ARRAY,      "year1_hourly_e_tofromgrid",         "Electricity to/from grid",       "kWh", "",                      "Time Series",             "*",                         "LENGTH=8760",                   "" },
//	{ SSC_OUTPUT,       SSC_ARRAY,      "year1_hourly_system_output",  "Year 1 hourly electricity from system",     "kWh", "",                      "",             "*",                         "LENGTH=8760",                   "" },
//	{ SSC_OUTPUT,       SSC_ARRAY,      "year1_hourly_e_demand",       "Year 1 hourly electricity from grid",     "kWh", "",                      "",             "*",                         "LENGTH=8760",                   "" },
	
//	{ SSC_OUTPUT,       SSC_ARRAY,      "year1_hourly_system_to_grid",    "Year 1 hourly electricity to grid",     "kWh", "",                      "",             "*",                         "LENGTH=8760",                   "" },
//	{ SSC_OUTPUT,       SSC_ARRAY,      "year1_hourly_system_to_load",    "Year 1 hourly system electricity to load",     "kWh", "",                      "",             "*",                         "LENGTH=8760",                   "" },
	{ SSC_OUTPUT, SSC_ARRAY, "year1_hourly_load", "Electricity load (year 1)", "kW", "", "Time Series", "*", "LENGTH=8760", "" },

// lifetime load (optional for lifetime analysis)
	{ SSC_OUTPUT, SSC_ARRAY, "lifetime_load", "Lifetime electricity load", "kW", "", "Time Series", "system_use_lifetime_output=1", "", "" },

//	{ SSC_OUTPUT,       SSC_ARRAY,      "year1_hourly_p_grid",         "Year 1 subhourly peak to/from grid", "kW",  "",                      "",             "*",                         "LENGTH=8760",                   "" },
	{ SSC_OUTPUT,       SSC_ARRAY,      "year1_hourly_p_tofromgrid",         "Electricity to/from grid peak", "kW",  "",                      "Time Series",             "*",                         "LENGTH=8760",                   "" },
//	{ SSC_OUTPUT,       SSC_ARRAY,      "year1_hourly_p_demand",       "Year 1 subhourly peak from grid", "kW",  "",                      "",             "*",                         "LENGTH=8760",                   "" },
	
	{ SSC_OUTPUT,       SSC_ARRAY,      "year1_hourly_p_system_to_load",         "Electricity peak load met by system", "kW",  "",                      "Time Series",             "*",                         "LENGTH=8760",                   "" },
	

//	{ SSC_OUTPUT,       SSC_ARRAY,      "year1_hourly_revenue_with_system",     "Year 1 hourly sales/purchases with sytem",    "$", "",          "",             "*",                         "LENGTH=8760",                   "" },
	{ SSC_OUTPUT,       SSC_ARRAY,      "year1_hourly_salespurchases_with_system",     "Electricity sales/purchases with sytem",    "$", "",          "Time Series",             "*",                         "LENGTH=8760",                   "" },
//	{ SSC_OUTPUT,       SSC_ARRAY,      "year1_hourly_payment_with_system",     "Year 1 hourly electricity purchases with system",    "$", "",          "",             "*",                         "LENGTH=8760",                   "" },
//	{ SSC_OUTPUT,       SSC_ARRAY,      "year1_hourly_income_with_system",      "Year 1 hourly electricity sales with system",     "$", "",          "",             "*",                         "LENGTH=8760",                   "" },
//	{ SSC_OUTPUT,       SSC_ARRAY,      "year1_hourly_price_with_system",       "Year 1 hourly energy charge with system",      "$", "",          "",             "*",                         "LENGTH=8760",                   "" },
	
//	{ SSC_OUTPUT,       SSC_ARRAY,      "year1_hourly_revenue_without_system",  "Year 1 hourly sales/purchases without sytem", "$", "",          "",             "*",                         "LENGTH=8760",                   "" },
	{ SSC_OUTPUT,       SSC_ARRAY,      "year1_hourly_salespurchases_without_system",  "Electricity sales/purchases without sytem", "$", "",          "Time Series",             "*",                         "LENGTH=8760",                   "" },
//	{ SSC_OUTPUT,       SSC_ARRAY,      "year1_hourly_payment_without_system",  "Year 1 hourly electricity purchases without system", "$", "",          "",             "*",                         "LENGTH=8760",                   "" },
//	{ SSC_OUTPUT,       SSC_ARRAY,      "year1_hourly_income_without_system",   "Year 1 hourly electricity sales without system",  "$", "",          "",             "*",                         "LENGTH=8760",                   "" },
//	{ SSC_OUTPUT,       SSC_ARRAY,      "year1_hourly_price_without_system",    "Year 1 hourly energy charge without system",   "$", "",          "",             "*",                         "LENGTH=8760",                   "" },

	{ SSC_OUTPUT, SSC_ARRAY, "year1_hourly_ec_with_system", "Electricity energy charge with system", "$", "", "Time Series", "*", "LENGTH=8760", "" },
	{ SSC_OUTPUT, SSC_ARRAY, "year1_hourly_ec_without_system", "Electricity energy charge without system", "$", "", "Time Series", "*", "LENGTH=8760", "" },

	{ SSC_OUTPUT, SSC_ARRAY, "year1_hourly_dc_with_system", "Electricity demand charge with system", "$", "", "Time Series", "*", "LENGTH=8760", "" },
	{ SSC_OUTPUT, SSC_ARRAY, "year1_hourly_dc_without_system", "Electricity demand charge without system", "$", "", "Time Series", "*", "LENGTH=8760", "" },

	{ SSC_OUTPUT, SSC_ARRAY, "year1_hourly_ec_tou_schedule", "Electricity TOU period for energy charges", "", "", "Time Series", "*", "LENGTH=8760", "" },
	{ SSC_OUTPUT,       SSC_ARRAY,      "year1_hourly_dc_tou_schedule",       "Electricity TOU period for demand charges", "", "", "Time Series", "*", "LENGTH=8760", "" },
	{ SSC_OUTPUT,       SSC_ARRAY,      "year1_hourly_dc_peak_per_period",    "Electricity from grid peak per TOU period",        "kW", "", "Time Series", "*", "LENGTH=8760", "" },


	{ SSC_OUTPUT, SSC_ARRAY, "year1_monthly_fixed_with_system", "Electricity charge (fixed) with system", "$/mo", "", "Monthly", "*", "LENGTH=12", "" },
	{ SSC_OUTPUT, SSC_ARRAY, "year1_monthly_fixed_without_system", "Electricity charge (fixed) without system", "$/mo", "", "Monthly", "*", "LENGTH=12", "" },
	{ SSC_OUTPUT, SSC_ARRAY, "year1_monthly_minimum_with_system", "Electricity charge (minimum) with system", "$/mo", "", "Monthly", "*", "LENGTH=12", "" },
	{ SSC_OUTPUT, SSC_ARRAY, "year1_monthly_minimum_without_system", "Electricity charge (minimum) without system", "$/mo", "", "Monthly", "*", "LENGTH=12", "" },
	{ SSC_OUTPUT, SSC_ARRAY, "year1_monthly_dc_fixed_with_system", "Electricity demand charge (fixed) with system", "$/mo", "", "Monthly", "*", "LENGTH=12", "" },
	{ SSC_OUTPUT, SSC_ARRAY, "year1_monthly_dc_tou_with_system", "Electricity demand charge (TOU) with system", "$/mo", "", "Monthly", "*", "LENGTH=12", "" },
	{ SSC_OUTPUT, SSC_ARRAY, "year1_monthly_ec_charge_with_system", "Electricity energy charge (TOU) with system", "$/mo", "", "Monthly", "*", "LENGTH=12", "" },
	{ SSC_OUTPUT, SSC_ARRAY, "year1_monthly_ec_charge_flat_with_system", "Electricity energy charge (flat) with system", "$/mo", "", "Monthly", "*", "LENGTH=12", "" },
	//	{ SSC_OUTPUT,       SSC_ARRAY,      "year1_monthly_ec_rate_with_system",       "Year 1 monthly energy rate with system",              "$/kWh", "", "",          "*",                         "LENGTH=12",                     "" },
	
	{ SSC_OUTPUT,       SSC_ARRAY,      "year1_monthly_dc_fixed_without_system",   "Electricity demand charge (fixed) without system", "$/mo", "", "Monthly",          "*",                         "LENGTH=12",                     "" },
	{ SSC_OUTPUT,       SSC_ARRAY,      "year1_monthly_dc_tou_without_system",     "Electricity demand charge (TOU) without system",   "$/mo", "", "Monthly",          "*",                         "LENGTH=12",                     "" },
	{ SSC_OUTPUT, SSC_ARRAY, "year1_monthly_ec_charge_without_system", "Electricity energy charge (TOU) without system", "$/mo", "", "Monthly", "*", "LENGTH=12", "" },
	{ SSC_OUTPUT, SSC_ARRAY, "year1_monthly_ec_charge_flat_without_system", "Electricity energy charge (flat) without system", "$/mo", "", "Monthly", "*", "LENGTH=12", "" },
	//	{ SSC_OUTPUT,       SSC_ARRAY,      "year1_monthly_ec_rate_without_system",    "Year 1 monthly energy rate without system",           "$/kWh", "", "",          "*",                         "LENGTH=12",                     "" },


	// monthly outputs from Sean 7/29/13 "Net Metering Accounting.xlsx" updates from Paul and Sean 8/9/13 and 8/12/13
	{ SSC_OUTPUT,       SSC_ARRAY,      "year1_monthly_load",    "Electricity load",           "kWh/mo", "", "Monthly",          "*",                         "LENGTH=12",                     "" },
//	{ SSC_OUTPUT,       SSC_ARRAY,      "year1_monthly_system_generation",    "monthly system generation",           "kWh", "", "",          "*",                         "LENGTH=12",                     "" },
	{ SSC_OUTPUT,       SSC_ARRAY,      "year1_monthly_electricity_to_grid",    "Electricity to/from grid",           "kWh/mo", "", "Monthly",          "*",                         "LENGTH=12",                     "" },
//	{ SSC_OUTPUT,       SSC_ARRAY,      "year1_monthly_electricity_needed_from_grid",    "Electricity needed from grid",           "kWh", "", "",          "*",                         "LENGTH=12",                     "" },
	{ SSC_OUTPUT, SSC_ARRAY, "year1_monthly_cumulative_excess_generation", "Electricity net metering credit", "kWh/mo", "", "Monthly", "*", "LENGTH=12", "" },
	{ SSC_OUTPUT, SSC_ARRAY, "year1_monthly_cumulative_excess_dollars", "Dollar net metering credit", "$/mo", "", "Monthly", "*", "LENGTH=12", "" },
//	{ SSC_OUTPUT, SSC_ARRAY, "year1_monthly_salespurchases", "Electricity sales/purchases with system", "$/mo", "", "Monthly", "*", "LENGTH=12", "" },
//	{ SSC_OUTPUT, SSC_ARRAY, "year1_monthly_salespurchases_wo_sys", "Electricity sales/purchases without system", "$/mo", "", "Monthly", "*", "LENGTH=12", "" },
	{ SSC_OUTPUT, SSC_ARRAY, "year1_monthly_utility_bill_w_sys", "Utility bill with system", "$/mo", "", "Monthly", "*", "LENGTH=12", "" },
	{ SSC_OUTPUT, SSC_ARRAY, "year1_monthly_utility_bill_wo_sys", "Utility bill without system", "$/mo", "", "Monthly", "*", "LENGTH=12", "" },


	// convert annual outputs from Arrays to Matrices years x months
	{ SSC_OUTPUT, SSC_MATRIX, "utility_bill_w_sys_ym", "Utility bill with system", "$", "", "Charges by Month", "*", "", "COL_LABEL=MONTHS,FORMAT_SPEC=CURRENCY,GROUP=UR_AM" },
	{ SSC_OUTPUT, SSC_MATRIX, "utility_bill_wo_sys_ym", "Utility bill without system", "$", "", "Charges by Month", "*", "", "COL_LABEL=MONTHS,FORMAT_SPEC=CURRENCY,GROUP=UR_AM" },

	{ SSC_OUTPUT, SSC_MATRIX, "charge_w_sys_fixed_ym", "Fixed charge with system", "$", "", "Charges by Month", "*", "", "COL_LABEL=MONTHS,FORMAT_SPEC=CURRENCY,GROUP=UR_AM" },
	{ SSC_OUTPUT, SSC_MATRIX, "charge_wo_sys_fixed_ym", "Fixed charge without system", "$", "", "Charges by Month", "*", "", "COL_LABEL=MONTHS,FORMAT_SPEC=CURRENCY,GROUP=UR_AM" },

	{ SSC_OUTPUT, SSC_MATRIX, "charge_w_sys_minimum_ym", "Minimum charge with system", "$", "", "Charges by Month", "*", "", "COL_LABEL=MONTHS,FORMAT_SPEC=CURRENCY,GROUP=UR_AM" },
	{ SSC_OUTPUT, SSC_MATRIX, "charge_wo_sys_minimum_ym", "Minimum charge without system", "$", "", "Charges by Month", "*", "", "COL_LABEL=MONTHS,FORMAT_SPEC=CURRENCY,GROUP=UR_AM" },

	{ SSC_OUTPUT, SSC_MATRIX, "charge_w_sys_dc_fixed_ym", "Demand charge with system (fixed)", "$", "", "Charges by Month", "*", "", "COL_LABEL=MONTHS,FORMAT_SPEC=CURRENCY,GROUP=UR_AM" },
	{ SSC_OUTPUT, SSC_MATRIX, "charge_w_sys_dc_tou_ym", "Demand charge with system (TOU)", "$", "", "Charges by Month", "*", "", "COL_LABEL=MONTHS,FORMAT_SPEC=CURRENCY,GROUP=UR_AM" },
	{ SSC_OUTPUT, SSC_MATRIX, "charge_wo_sys_dc_fixed_ym", "Demand charge without system (fixed)", "$", "", "Charges by Month", "*", "", "COL_LABEL=MONTHS,FORMAT_SPEC=CURRENCY,GROUP=UR_AM" },
	{ SSC_OUTPUT, SSC_MATRIX, "charge_wo_sys_dc_tou_ym", "Demand charge without system (TOU)", "$", "", "Charges by Month", "*", "", "COL_LABEL=MONTHS,FORMAT_SPEC=CURRENCY,GROUP=UR_AM" },

	{ SSC_OUTPUT, SSC_MATRIX, "charge_w_sys_ec_ym", "Energy charge with system (TOU)", "$", "", "Charges by Month", "*", "", "COL_LABEL=MONTHS,FORMAT_SPEC=CURRENCY,GROUP=UR_AM" },
	{ SSC_OUTPUT, SSC_MATRIX, "charge_w_sys_ec_flat_ym", "Energy charge with system (flat)", "$", "", "Charges by Month", "*", "", "COL_LABEL=MONTHS,FORMAT_SPEC=CURRENCY,GROUP=UR_AM" },
	{ SSC_OUTPUT, SSC_MATRIX, "charge_wo_sys_ec_ym", "Energy charge without system (TOU)", "$", "", "Charges by Month", "*", "", "COL_LABEL=MONTHS,FORMAT_SPEC=CURRENCY,GROUP=UR_AM" },
	{ SSC_OUTPUT, SSC_MATRIX, "charge_wo_sys_ec_flat_ym", "Energy charge without system (flat)", "$", "", "Charges by Month", "*", "", "COL_LABEL=MONTHS,FORMAT_SPEC=CURRENCY,GROUP=UR_AM" },


	// annual sums
	{ SSC_OUTPUT, SSC_ARRAY, "utility_bill_w_sys", "Utility bill with system", "$", "", "Charges by Month", "*", "", "" },
	{ SSC_OUTPUT, SSC_ARRAY, "utility_bill_wo_sys", "Utility bill without system", "$", "", "Charges by Month", "*", "", "" },

	{ SSC_OUTPUT, SSC_ARRAY, "charge_w_sys_fixed", "Fixed charge with system", "$", "", "Charges by Month", "*", "", "" },
	{ SSC_OUTPUT, SSC_ARRAY, "charge_wo_sys_fixed", "Fixed charge without system", "$", "", "Charges by Month", "*", "", "" },

	{ SSC_OUTPUT, SSC_ARRAY, "charge_w_sys_minimum", "Minimum charge with system", "$", "", "Charges by Month", "*", "", "" },
	{ SSC_OUTPUT, SSC_ARRAY, "charge_wo_sys_minimum", "Minimum charge without system", "$", "", "Charges by Month", "*", "", "" },

	{ SSC_OUTPUT, SSC_ARRAY, "charge_w_sys_dc_fixed", "Demand charge with system (fixed)", "$", "", "Charges by Month", "*", "", "" },
	{ SSC_OUTPUT, SSC_ARRAY, "charge_w_sys_dc_tou", "Demand charge with system (TOU)", "$", "", "Charges by Month", "*", "", "" },
	{ SSC_OUTPUT, SSC_ARRAY, "charge_wo_sys_dc_fixed", "Demand charge without system (fixed)", "$", "", "Charges by Month", "*", "", "" },
	{ SSC_OUTPUT, SSC_ARRAY, "charge_wo_sys_dc_tou", "Demand charge without system (TOU)", "$", "", "Charges by Month", "*", "", "" },

	{ SSC_OUTPUT, SSC_ARRAY, "charge_w_sys_ec", "Energy charge with system (TOU)", "$", "", "Charges by Month", "*", "", "" },
	{ SSC_OUTPUT, SSC_ARRAY, "charge_w_sys_ec_flat", "Energy charge with system (flat)", "$", "", "Charges by Month", "*", "", "" },
	{ SSC_OUTPUT, SSC_ARRAY, "charge_wo_sys_ec", "Energy charge without system (TOU)", "$", "", "Charges by Month", "*", "", "" },
	{ SSC_OUTPUT, SSC_ARRAY, "charge_wo_sys_ec_flat", "Energy charge without system (flat)", "$", "", "Charges by Month", "*", "", "" },






// for Pablo at IRENA 8/8/15
// first year outputs only per email from Paul 8/9/15

// energy charge wo system
	{ SSC_OUTPUT, SSC_MATRIX, "charge_wo_sys_ec_jan_tp", "Energy charge without system (TOU) Jan", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },
	{ SSC_OUTPUT, SSC_MATRIX, "charge_wo_sys_ec_feb_tp", "Energy charge without system (TOU) Feb", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },
	{ SSC_OUTPUT, SSC_MATRIX, "charge_wo_sys_ec_mar_tp", "Energy charge without system (TOU) Mar", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },
	{ SSC_OUTPUT, SSC_MATRIX, "charge_wo_sys_ec_apr_tp", "Energy charge without system (TOU) Apr", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },
	{ SSC_OUTPUT, SSC_MATRIX, "charge_wo_sys_ec_may_tp", "Energy charge without system (TOU) May", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },
	{ SSC_OUTPUT, SSC_MATRIX, "charge_wo_sys_ec_jun_tp", "Energy charge without system (TOU) Jun", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },
	{ SSC_OUTPUT, SSC_MATRIX, "charge_wo_sys_ec_jul_tp", "Energy charge without system (TOU) Jul", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },
	{ SSC_OUTPUT, SSC_MATRIX, "charge_wo_sys_ec_aug_tp", "Energy charge without system (TOU) Aug", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },
	{ SSC_OUTPUT, SSC_MATRIX, "charge_wo_sys_ec_sep_tp", "Energy charge without system (TOU) Sep", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },
	{ SSC_OUTPUT, SSC_MATRIX, "charge_wo_sys_ec_oct_tp", "Energy charge without system (TOU) Oct", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },
	{ SSC_OUTPUT, SSC_MATRIX, "charge_wo_sys_ec_nov_tp", "Energy charge without system (TOU) Nov", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },
	{ SSC_OUTPUT, SSC_MATRIX, "charge_wo_sys_ec_dec_tp", "Energy charge without system (TOU) Dec", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },

	// energy use wo system
	{ SSC_OUTPUT, SSC_MATRIX, "energy_wo_sys_ec_jan_tp", "Energy use without system (TOU) Jan", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },
	{ SSC_OUTPUT, SSC_MATRIX, "energy_wo_sys_ec_feb_tp", "Energy use without system (TOU) Feb", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },
	{ SSC_OUTPUT, SSC_MATRIX, "energy_wo_sys_ec_mar_tp", "Energy use without system (TOU) Mar", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },
	{ SSC_OUTPUT, SSC_MATRIX, "energy_wo_sys_ec_apr_tp", "Energy use without system (TOU) Apr", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },
	{ SSC_OUTPUT, SSC_MATRIX, "energy_wo_sys_ec_may_tp", "Energy use without system (TOU) May", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },
	{ SSC_OUTPUT, SSC_MATRIX, "energy_wo_sys_ec_jun_tp", "Energy use without system (TOU) Jun", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },
	{ SSC_OUTPUT, SSC_MATRIX, "energy_wo_sys_ec_jul_tp", "Energy use without system (TOU) Jul", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },
	{ SSC_OUTPUT, SSC_MATRIX, "energy_wo_sys_ec_aug_tp", "Energy use without system (TOU) Aug", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },
	{ SSC_OUTPUT, SSC_MATRIX, "energy_wo_sys_ec_sep_tp", "Energy use without system (TOU) Sep", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },
	{ SSC_OUTPUT, SSC_MATRIX, "energy_wo_sys_ec_oct_tp", "Energy use without system (TOU) Oct", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },
	{ SSC_OUTPUT, SSC_MATRIX, "energy_wo_sys_ec_nov_tp", "Energy use without system (TOU) Nov", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },
	{ SSC_OUTPUT, SSC_MATRIX, "energy_wo_sys_ec_dec_tp", "Energy use without system (TOU) Dec", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },


	// energy charge w system
	{ SSC_OUTPUT, SSC_MATRIX, "charge_w_sys_ec_jan_tp", "Energy charge with system (TOU) Jan", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },
	{ SSC_OUTPUT, SSC_MATRIX, "charge_w_sys_ec_feb_tp", "Energy charge with system (TOU) Feb", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },
	{ SSC_OUTPUT, SSC_MATRIX, "charge_w_sys_ec_mar_tp", "Energy charge with system (TOU) Mar", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },
	{ SSC_OUTPUT, SSC_MATRIX, "charge_w_sys_ec_apr_tp", "Energy charge with system (TOU) Apr", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },
	{ SSC_OUTPUT, SSC_MATRIX, "charge_w_sys_ec_may_tp", "Energy charge with system (TOU) May", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },
	{ SSC_OUTPUT, SSC_MATRIX, "charge_w_sys_ec_jun_tp", "Energy charge with system (TOU) Jun", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },
	{ SSC_OUTPUT, SSC_MATRIX, "charge_w_sys_ec_jul_tp", "Energy charge with system (TOU) Jul", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },
	{ SSC_OUTPUT, SSC_MATRIX, "charge_w_sys_ec_aug_tp", "Energy charge with system (TOU) Aug", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },
	{ SSC_OUTPUT, SSC_MATRIX, "charge_w_sys_ec_sep_tp", "Energy charge with system (TOU) Sep", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },
	{ SSC_OUTPUT, SSC_MATRIX, "charge_w_sys_ec_oct_tp", "Energy charge with system (TOU) Oct", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },
	{ SSC_OUTPUT, SSC_MATRIX, "charge_w_sys_ec_nov_tp", "Energy charge with system (TOU) Nov", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },
	{ SSC_OUTPUT, SSC_MATRIX, "charge_w_sys_ec_dec_tp", "Energy charge with system (TOU) Dec", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },

	// energy use w system
	{ SSC_OUTPUT, SSC_MATRIX, "energy_w_sys_ec_jan_tp", "Energy use with system (TOU) Jan", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },
	{ SSC_OUTPUT, SSC_MATRIX, "energy_w_sys_ec_feb_tp", "Energy use with system (TOU) Feb", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },
	{ SSC_OUTPUT, SSC_MATRIX, "energy_w_sys_ec_mar_tp", "Energy use with system (TOU) Mar", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },
	{ SSC_OUTPUT, SSC_MATRIX, "energy_w_sys_ec_apr_tp", "Energy use with system (TOU) Apr", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },
	{ SSC_OUTPUT, SSC_MATRIX, "energy_w_sys_ec_may_tp", "Energy use with system (TOU) May", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },
	{ SSC_OUTPUT, SSC_MATRIX, "energy_w_sys_ec_jun_tp", "Energy use with system (TOU) Jun", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },
	{ SSC_OUTPUT, SSC_MATRIX, "energy_w_sys_ec_jul_tp", "Energy use with system (TOU) Jul", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },
	{ SSC_OUTPUT, SSC_MATRIX, "energy_w_sys_ec_aug_tp", "Energy use with system (TOU) Aug", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },
	{ SSC_OUTPUT, SSC_MATRIX, "energy_w_sys_ec_sep_tp", "Energy use with system (TOU) Sep", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },
	{ SSC_OUTPUT, SSC_MATRIX, "energy_w_sys_ec_oct_tp", "Energy use with system (TOU) Oct", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },
	{ SSC_OUTPUT, SSC_MATRIX, "energy_w_sys_ec_nov_tp", "Energy use with system (TOU) Nov", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },
	{ SSC_OUTPUT, SSC_MATRIX, "energy_w_sys_ec_dec_tp", "Energy use with system (TOU) Dec", "$", "", "Charges by Month", "*", "", "ROW_LABEL=UR_PERIODNUMS,COL_LABEL=UR_TIERNUMS,FORMAT_SPEC=CURRENCY,GROUP=UR_MTP" },



	var_info_invalid };


class ur_month
{
public:
	// period numbers
	std::vector<int> ec_periods;
	std::vector<int> dc_periods;
	// monthly values
	// net energy use per month
	ssc_number_t energy_net;
	// hours per period per month
	int hours_per_month;
	// energy use period and tier
	util::matrix_t<ssc_number_t> ec_energy_use;
	// peak demand per period
	std::vector<ssc_number_t> dc_tou_peak;
	std::vector<int> dc_tou_peak_hour;
	ssc_number_t dc_flat_peak;
	int dc_flat_peak_hour;
	// energy tou charges
	util::matrix_t<ssc_number_t>  ec_tou_ub;
	util::matrix_t<ssc_number_t>  ec_tou_br;
	util::matrix_t<ssc_number_t>  ec_tou_sr;
	util::matrix_t<int>  ec_tou_units;
	// calculated charges per period and tier
	util::matrix_t<ssc_number_t>  ec_charge;
	// demand tou charges
	util::matrix_t<ssc_number_t>  dc_tou_ub;
	util::matrix_t<ssc_number_t>  dc_tou_ch;
	// demand flat charges
	std::vector<ssc_number_t>  dc_flat_ub;
	std::vector<ssc_number_t>  dc_flat_ch;
	// calculated charges per period
	std::vector<double>  dc_tou_charge;
	ssc_number_t dc_flat_charge;
};

class cm_utilityrate3_mat : public compute_module
{
private:
	// schedule outputs
	std::vector<int> m_ec_tou_sched;
	std::vector<int> m_dc_tou_sched;
	std::vector<ur_month> m_month;
	std::vector<int> m_ec_periods; // period number
	std::vector<std::vector<int> >  m_ec_periods_tiers; // tier numbers
	std::vector<int> m_dc_tou_periods; // period number
	std::vector<std::vector<int> >  m_dc_tou_periods_tiers; // tier numbers
	std::vector<std::vector<int> >  m_dc_flat_tiers; // tier numbers for each month of flat demand charge


public:
	cm_utilityrate3_mat()
	{
		add_var_info( vtab_utility_rate3_mat );
	}

	void exec( ) throw( general_error )
	{
		ssc_number_t *parr = 0;
		size_t count, i, j; 

		size_t nyears = (size_t)as_integer("analysis_period");
		double inflation_rate = as_double("inflation_rate")*0.01;

		// compute annual system output degradation multipliers
		std::vector<ssc_number_t> sys_scale(nyears);

		// degradation
		// degradation starts in year 2 for single value degradation - no degradation in year 1 - degradation =1.0
		// lifetime degradation applied in technology compute modules
		if (as_integer("system_use_lifetime_output") == 1)
		{
			for (i = 0; i<nyears; i++)
				sys_scale[i] = 1.0;
		}
		else
		{
			parr = as_array("degradation", &count);
			if (count == 1)
			{
				for (i = 0; i<nyears; i++)
					sys_scale[i] = (ssc_number_t)pow((double)(1 - parr[0] * 0.01), (double)i);
			}
			else
			{
				for (i = 0; i<nyears && i<count; i++)
					sys_scale[i] = (ssc_number_t)(1.0 - parr[i] * 0.01);
			}
		}



		// compute load (electric demand) annual escalation multipliers
		std::vector<ssc_number_t> load_scale(nyears);
		parr = as_array("load_escalation", &count);
		if (count == 1)
		{
			for (i=0;i<nyears;i++)
				load_scale[i] = (ssc_number_t)pow( (double)(1+parr[0]*0.01), (double)i );
		}
		else
		{
			for (i=0;i<nyears;i++)
				load_scale[i] = (ssc_number_t)(1 + parr[i]*0.01);
		}

		// compute utility rate out-years escalation multipliers
		std::vector<ssc_number_t> rate_scale(nyears);
		parr = as_array("rate_escalation", &count);
		if (count == 1)
		{
			for (i=0;i<nyears;i++)
				rate_scale[i] = (ssc_number_t)pow( (double)(inflation_rate+1+parr[0]*0.01), (double)i );
		}
		else
		{
			for (i=0;i<nyears;i++)
				rate_scale[i] = (ssc_number_t)(1 + parr[i]*0.01);
		}


		// prepare 8760 arrays for load and grid values
		std::vector<ssc_number_t> e_sys(8760), p_sys(8760), e_sys_cy(8760), p_sys_cy(8760),
			e_load(8760), p_load(8760),
			e_grid(8760), p_grid(8760),
			e_load_cy(8760), p_load_cy(8760); // current year load (accounts for escal)
		



		/* Update all e_sys and e_load values based on new inputs
		grid = gen -load where gen = sys + batt
		1. scale load and system value to hourly values as necessary
		2. use (kWh) e_sys[i] = sum((grid+load) * timestep ) over the hour for each hour i
		3. use (kW)  p_sys[i] = max( grid+load) over the hour for each hour i
		3. use (kWh) e_load[i] = sum(load * timestep ) over the hour for each hour i
		4. use (kW)  p_load[i] = max(load) over the hour for each hour i
		5. After above assignment, proceed as before with same outputs
		*/
		ssc_number_t *pload, *pgen;
		size_t nrec_load = 0, nrec_gen = 0, step_per_hour_gen=1, step_per_hour_load=1;
		bool bload=false;
		pgen = as_array("gen", &nrec_gen);
		// for lifetime analysis
		size_t nrec_gen_per_year = nrec_gen;
		if (as_integer("system_use_lifetime_output") == 1)
			nrec_gen_per_year = nrec_gen / nyears;
		step_per_hour_gen = nrec_gen_per_year / 8760;
		if (step_per_hour_gen < 1 || step_per_hour_gen > 60 || step_per_hour_gen * 8760 != nrec_gen_per_year)
			throw exec_error("utilityrate3", util::format("invalid number of gen records (%d): must be an integer multiple of 8760", (int)nrec_gen_per_year));
		ssc_number_t ts_hour_gen = 1.0f / step_per_hour_gen;


		if (is_assigned("load"))
		{ // hourly or sub hourly loads for single year
			bload = true;
			pload = as_array("load", &nrec_load);
			step_per_hour_load = nrec_load / 8760;
			if (step_per_hour_load < 1 || step_per_hour_load > 60 || step_per_hour_load * 8760 != nrec_load)
				throw exec_error("utilityrate3", util::format("invalid number of load records (%d): must be an integer multiple of 8760", (int)nrec_load));
//			if (nrec_load != nrec_gen)
//				throw exec_error("utilityrate3", util::format("number of load records (%d) must be equal to number of gen records (%d)", (int)nrec_load, (int)nrec_gen));
		}
		ssc_number_t ts_hour_load = 1.0f / step_per_hour_load;


		// assign hourly values for utility rate calculations
		size_t idx = 0;
		ssc_number_t ts_power = 0;
		ssc_number_t ts_load = 0;
		ssc_number_t year1_elec_load = 0;
		for (i = 0; i < 8760; i++)
		{
			e_sys[i] = p_sys[i] = e_grid[i] = p_grid[i] = e_load[i] = p_load[i] = e_load_cy[i] = p_load_cy[i] = 0.0;
			for (size_t ii = 0; ii < step_per_hour_gen; ii++)
			{
				ts_power = pgen[idx];
				e_sys[i] += ts_power * ts_hour_gen;
				p_sys[i] = ((ts_power > p_sys[i]) ? ts_power : p_sys[i]);
				idx++;
			}
		}
		//load
		idx = 0;
		for (i = 0; i < 8760; i++)
		{
			for (size_t ii = 0; ii < step_per_hour_load; ii++)
			{
				ts_load = (bload ? pload[idx] : 0);
				e_load[i] += ts_load * ts_hour_load;
				p_load[i] = ((ts_load > p_load[i]) ? ts_load : p_load[i]);
				idx++;
			}
			year1_elec_load += e_load[i];
			// sign correction for utility rate calculations
			e_load[i] = -e_load[i];
			p_load[i] = -p_load[i];
		}

		assign("year1_electric_load", year1_elec_load);


		/* allocate intermediate data arrays */
		std::vector<ssc_number_t> revenue_w_sys(8760), revenue_wo_sys(8760),
			payment(8760), income(8760), price(8760), demand_charge(8760), energy_charge(8760),
			ec_tou_sched(8760), dc_tou_sched(8760), load(8760), dc_hourly_peak(8760),
			e_tofromgrid(8760), p_tofromgrid(8760),	salespurchases(8760);
		std::vector<ssc_number_t> monthly_revenue_w_sys(12), monthly_revenue_wo_sys(12),
			monthly_fixed_charges(12), monthly_minimum_charges(12),
			monthly_dc_fixed(12), monthly_dc_tou(12),
			monthly_ec_charges(12), monthly_ec_flat_charges(12), monthly_ec_rates(12),
			monthly_salespurchases(12),
			monthly_load(12), monthly_system_generation(12), monthly_elec_to_grid(12),
			monthly_elec_needed_from_grid(12),
			monthly_cumulative_excess_energy(12), monthly_cumulative_excess_dollars(12), monthly_bill(12);

		/* allocate outputs */		
		ssc_number_t *annual_net_revenue = allocate("annual_energy_value", nyears+1);
		ssc_number_t *annual_electric_load = allocate("annual_electric_load", nyears+1);
		ssc_number_t *energy_net = allocate("scaled_annual_energy", nyears+1);
		ssc_number_t *annual_revenue_w_sys = allocate("revenue_with_system", nyears+1);
		ssc_number_t *annual_revenue_wo_sys = allocate("revenue_without_system", nyears+1);
		ssc_number_t *annual_elec_cost_w_sys = allocate("elec_cost_with_system", nyears+1);
		ssc_number_t *annual_elec_cost_wo_sys = allocate("elec_cost_without_system", nyears+1);

		// matrices
		ssc_number_t *utility_bill_w_sys_ym = allocate("utility_bill_w_sys_ym", nyears + 1, 12);
		ssc_number_t *utility_bill_wo_sys_ym = allocate("utility_bill_wo_sys_ym", nyears + 1, 12);
		ssc_number_t *ch_w_sys_dc_fixed_ym = allocate("charge_w_sys_dc_fixed_ym", nyears + 1, 12);
		ssc_number_t *ch_w_sys_dc_tou_ym = allocate("charge_w_sys_dc_tou_ym", nyears + 1, 12);
		ssc_number_t *ch_w_sys_ec_ym = allocate("charge_w_sys_ec_ym", nyears + 1, 12);
		ssc_number_t *ch_w_sys_ec_flat_ym = allocate("charge_w_sys_ec_flat_ym", nyears + 1, 12);
		ssc_number_t *ch_wo_sys_dc_fixed_ym = allocate("charge_wo_sys_dc_fixed_ym", nyears + 1, 12);
		ssc_number_t *ch_wo_sys_dc_tou_ym = allocate("charge_wo_sys_dc_tou_ym", nyears + 1, 12);
		ssc_number_t *ch_wo_sys_ec_ym = allocate("charge_wo_sys_ec_ym", nyears + 1, 12);
		ssc_number_t *ch_wo_sys_ec_flat_ym = allocate("charge_wo_sys_ec_flat_ym", nyears + 1, 12);
		ssc_number_t *ch_w_sys_fixed_ym = allocate("charge_w_sys_fixed_ym", nyears + 1, 12);
		ssc_number_t *ch_wo_sys_fixed_ym = allocate("charge_wo_sys_fixed_ym", nyears + 1, 12);
		ssc_number_t *ch_w_sys_minimum_ym = allocate("charge_w_sys_minimum_ym", nyears + 1, 12);
		ssc_number_t *ch_wo_sys_minimum_ym = allocate("charge_wo_sys_minimum_ym", nyears + 1, 12);


		// annual sums
		ssc_number_t *utility_bill_w_sys = allocate("utility_bill_w_sys", nyears + 1);
		ssc_number_t *utility_bill_wo_sys = allocate("utility_bill_wo_sys", nyears + 1);
		ssc_number_t *ch_w_sys_dc_fixed = allocate("charge_w_sys_dc_fixed", nyears + 1);
		ssc_number_t *ch_w_sys_dc_tou = allocate("charge_w_sys_dc_tou", nyears + 1);
		ssc_number_t *ch_w_sys_ec = allocate("charge_w_sys_ec", nyears + 1);
		ssc_number_t *ch_w_sys_ec_flat = allocate("charge_w_sys_ec_flat", nyears + 1);
		ssc_number_t *ch_wo_sys_dc_fixed = allocate("charge_wo_sys_dc_fixed", nyears + 1);
		ssc_number_t *ch_wo_sys_dc_tou = allocate("charge_wo_sys_dc_tou", nyears + 1);
		ssc_number_t *ch_wo_sys_ec = allocate("charge_wo_sys_ec", nyears + 1);
		ssc_number_t *ch_wo_sys_ec_flat = allocate("charge_wo_sys_ec_flat", nyears + 1);
		ssc_number_t *ch_w_sys_fixed = allocate("charge_w_sys_fixed", nyears + 1);
		ssc_number_t *ch_wo_sys_fixed = allocate("charge_wo_sys_fixed", nyears + 1);
		ssc_number_t *ch_w_sys_minimum = allocate("charge_w_sys_minimum", nyears + 1);
		ssc_number_t *ch_wo_sys_minimum = allocate("charge_wo_sys_minimum", nyears + 1);




		// IRENA outputs array of tier values
		// reverse to tiers columns and periods are rows based on IRENA desired output.
		// tiers and periods determined by input matrices 

		setup();


		// note that ec_charge and not ec_energy_use have the correct dimensions after setup
		util::matrix_t<ssc_number_t> &charge_wo_sys_ec_jan_tp = allocate_matrix("charge_wo_sys_ec_jan_tp", m_month[0].ec_charge.nrows() + 2, m_month[0].ec_charge.ncols() + 2);
		util::matrix_t<ssc_number_t> &charge_wo_sys_ec_feb_tp = allocate_matrix("charge_wo_sys_ec_feb_tp", m_month[1].ec_charge.nrows() + 2, m_month[1].ec_charge.ncols() + 2);
		util::matrix_t<ssc_number_t> &charge_wo_sys_ec_mar_tp = allocate_matrix("charge_wo_sys_ec_mar_tp", m_month[2].ec_charge.nrows() + 2, m_month[2].ec_charge.ncols() + 2);
		util::matrix_t<ssc_number_t> &charge_wo_sys_ec_apr_tp = allocate_matrix("charge_wo_sys_ec_apr_tp", m_month[3].ec_charge.nrows() + 2, m_month[3].ec_charge.ncols() + 2);
		util::matrix_t<ssc_number_t> &charge_wo_sys_ec_may_tp = allocate_matrix("charge_wo_sys_ec_may_tp", m_month[4].ec_charge.nrows() + 2, m_month[4].ec_charge.ncols() + 2);
		util::matrix_t<ssc_number_t> &charge_wo_sys_ec_jun_tp = allocate_matrix("charge_wo_sys_ec_jun_tp", m_month[5].ec_charge.nrows() + 2, m_month[5].ec_charge.ncols() + 2);
		util::matrix_t<ssc_number_t> &charge_wo_sys_ec_jul_tp = allocate_matrix("charge_wo_sys_ec_jul_tp", m_month[6].ec_charge.nrows() + 2, m_month[6].ec_charge.ncols() + 2);
		util::matrix_t<ssc_number_t> &charge_wo_sys_ec_aug_tp = allocate_matrix("charge_wo_sys_ec_aug_tp", m_month[7].ec_charge.nrows() + 2, m_month[7].ec_charge.ncols() + 2);
		util::matrix_t<ssc_number_t> &charge_wo_sys_ec_sep_tp = allocate_matrix("charge_wo_sys_ec_sep_tp", m_month[8].ec_charge.nrows() + 2, m_month[8].ec_charge.ncols() + 2);
		util::matrix_t<ssc_number_t> &charge_wo_sys_ec_oct_tp = allocate_matrix("charge_wo_sys_ec_oct_tp", m_month[9].ec_charge.nrows() + 2, m_month[9].ec_charge.ncols() + 2);
		util::matrix_t<ssc_number_t> &charge_wo_sys_ec_nov_tp = allocate_matrix("charge_wo_sys_ec_nov_tp", m_month[10].ec_charge.nrows() + 2, m_month[10].ec_charge.ncols() + 2);
		util::matrix_t<ssc_number_t> &charge_wo_sys_ec_dec_tp = allocate_matrix("charge_wo_sys_ec_dec_tp", m_month[11].ec_charge.nrows() + 2, m_month[11].ec_charge.ncols() + 2);


		util::matrix_t<ssc_number_t> &energy_wo_sys_ec_jan_tp = allocate_matrix("energy_wo_sys_ec_jan_tp", m_month[0].ec_charge.nrows() + 2, m_month[0].ec_charge.ncols() + 2);
		util::matrix_t<ssc_number_t> &energy_wo_sys_ec_feb_tp = allocate_matrix("energy_wo_sys_ec_feb_tp", m_month[1].ec_charge.nrows() + 2, m_month[1].ec_charge.ncols() + 2);
		util::matrix_t<ssc_number_t> &energy_wo_sys_ec_mar_tp = allocate_matrix("energy_wo_sys_ec_mar_tp", m_month[2].ec_charge.nrows() + 2, m_month[2].ec_charge.ncols() + 2);
		util::matrix_t<ssc_number_t> &energy_wo_sys_ec_apr_tp = allocate_matrix("energy_wo_sys_ec_apr_tp", m_month[3].ec_charge.nrows() + 2, m_month[3].ec_charge.ncols() + 2);
		util::matrix_t<ssc_number_t> &energy_wo_sys_ec_may_tp = allocate_matrix("energy_wo_sys_ec_may_tp", m_month[4].ec_charge.nrows() + 2, m_month[4].ec_charge.ncols() + 2);
		util::matrix_t<ssc_number_t> &energy_wo_sys_ec_jun_tp = allocate_matrix("energy_wo_sys_ec_jun_tp", m_month[5].ec_charge.nrows() + 2, m_month[5].ec_charge.ncols() + 2);
		util::matrix_t<ssc_number_t> &energy_wo_sys_ec_jul_tp = allocate_matrix("energy_wo_sys_ec_jul_tp", m_month[6].ec_charge.nrows() + 2, m_month[6].ec_charge.ncols() + 2);
		util::matrix_t<ssc_number_t> &energy_wo_sys_ec_aug_tp = allocate_matrix("energy_wo_sys_ec_aug_tp", m_month[7].ec_charge.nrows() + 2, m_month[7].ec_charge.ncols() + 2);
		util::matrix_t<ssc_number_t> &energy_wo_sys_ec_sep_tp = allocate_matrix("energy_wo_sys_ec_sep_tp", m_month[8].ec_charge.nrows() + 2, m_month[8].ec_charge.ncols() + 2);
		util::matrix_t<ssc_number_t> &energy_wo_sys_ec_oct_tp = allocate_matrix("energy_wo_sys_ec_oct_tp", m_month[9].ec_charge.nrows() + 2, m_month[9].ec_charge.ncols() + 2);
		util::matrix_t<ssc_number_t> &energy_wo_sys_ec_nov_tp = allocate_matrix("energy_wo_sys_ec_nov_tp", m_month[10].ec_charge.nrows() + 2, m_month[10].ec_charge.ncols() + 2);
		util::matrix_t<ssc_number_t> &energy_wo_sys_ec_dec_tp = allocate_matrix("energy_wo_sys_ec_dec_tp", m_month[11].ec_charge.nrows() + 2, m_month[11].ec_charge.ncols() + 2);



		util::matrix_t<ssc_number_t> &charge_w_sys_ec_jan_tp = allocate_matrix("charge_w_sys_ec_jan_tp", m_month[0].ec_charge.nrows() + 2, m_month[0].ec_charge.ncols() + 2);
		util::matrix_t<ssc_number_t> &charge_w_sys_ec_feb_tp = allocate_matrix("charge_w_sys_ec_feb_tp", m_month[1].ec_charge.nrows() + 2, m_month[1].ec_charge.ncols() + 2);
		util::matrix_t<ssc_number_t> &charge_w_sys_ec_mar_tp = allocate_matrix("charge_w_sys_ec_mar_tp", m_month[2].ec_charge.nrows() + 2, m_month[2].ec_charge.ncols() + 2);
		util::matrix_t<ssc_number_t> &charge_w_sys_ec_apr_tp = allocate_matrix("charge_w_sys_ec_apr_tp", m_month[3].ec_charge.nrows() + 2, m_month[3].ec_charge.ncols() + 2);
		util::matrix_t<ssc_number_t> &charge_w_sys_ec_may_tp = allocate_matrix("charge_w_sys_ec_may_tp", m_month[4].ec_charge.nrows() + 2, m_month[4].ec_charge.ncols() + 2);
		util::matrix_t<ssc_number_t> &charge_w_sys_ec_jun_tp = allocate_matrix("charge_w_sys_ec_jun_tp", m_month[5].ec_charge.nrows() + 2, m_month[5].ec_charge.ncols() + 2);
		util::matrix_t<ssc_number_t> &charge_w_sys_ec_jul_tp = allocate_matrix("charge_w_sys_ec_jul_tp", m_month[6].ec_charge.nrows() + 2, m_month[6].ec_charge.ncols() + 2);
		util::matrix_t<ssc_number_t> &charge_w_sys_ec_aug_tp = allocate_matrix("charge_w_sys_ec_aug_tp", m_month[7].ec_charge.nrows() + 2, m_month[7].ec_charge.ncols() + 2);
		util::matrix_t<ssc_number_t> &charge_w_sys_ec_sep_tp = allocate_matrix("charge_w_sys_ec_sep_tp", m_month[8].ec_charge.nrows() + 2, m_month[8].ec_charge.ncols() + 2);
		util::matrix_t<ssc_number_t> &charge_w_sys_ec_oct_tp = allocate_matrix("charge_w_sys_ec_oct_tp", m_month[9].ec_charge.nrows() + 2, m_month[9].ec_charge.ncols() + 2);
		util::matrix_t<ssc_number_t> &charge_w_sys_ec_nov_tp = allocate_matrix("charge_w_sys_ec_nov_tp", m_month[10].ec_charge.nrows() + 2, m_month[10].ec_charge.ncols() + 2);
		util::matrix_t<ssc_number_t> &charge_w_sys_ec_dec_tp = allocate_matrix("charge_w_sys_ec_dec_tp", m_month[11].ec_charge.nrows() + 2, m_month[11].ec_charge.ncols() + 2);


		util::matrix_t<ssc_number_t> &energy_w_sys_ec_jan_tp = allocate_matrix("energy_w_sys_ec_jan_tp", m_month[0].ec_charge.nrows() + 2, m_month[0].ec_charge.ncols() + 2);
		util::matrix_t<ssc_number_t> &energy_w_sys_ec_feb_tp = allocate_matrix("energy_w_sys_ec_feb_tp", m_month[1].ec_charge.nrows() + 2, m_month[1].ec_charge.ncols() + 2);
		util::matrix_t<ssc_number_t> &energy_w_sys_ec_mar_tp = allocate_matrix("energy_w_sys_ec_mar_tp", m_month[2].ec_charge.nrows() + 2, m_month[2].ec_charge.ncols() + 2);
		util::matrix_t<ssc_number_t> &energy_w_sys_ec_apr_tp = allocate_matrix("energy_w_sys_ec_apr_tp", m_month[3].ec_charge.nrows() + 2, m_month[3].ec_charge.ncols() + 2);
		util::matrix_t<ssc_number_t> &energy_w_sys_ec_may_tp = allocate_matrix("energy_w_sys_ec_may_tp", m_month[4].ec_charge.nrows() + 2, m_month[4].ec_charge.ncols() + 2);
		util::matrix_t<ssc_number_t> &energy_w_sys_ec_jun_tp = allocate_matrix("energy_w_sys_ec_jun_tp", m_month[5].ec_charge.nrows() + 2, m_month[5].ec_charge.ncols() + 2);
		util::matrix_t<ssc_number_t> &energy_w_sys_ec_jul_tp = allocate_matrix("energy_w_sys_ec_jul_tp", m_month[6].ec_charge.nrows() + 2, m_month[6].ec_charge.ncols() + 2);
		util::matrix_t<ssc_number_t> &energy_w_sys_ec_aug_tp = allocate_matrix("energy_w_sys_ec_aug_tp", m_month[7].ec_charge.nrows() + 2, m_month[7].ec_charge.ncols() + 2);
		util::matrix_t<ssc_number_t> &energy_w_sys_ec_sep_tp = allocate_matrix("energy_w_sys_ec_sep_tp", m_month[8].ec_charge.nrows() + 2, m_month[8].ec_charge.ncols() + 2);
		util::matrix_t<ssc_number_t> &energy_w_sys_ec_oct_tp = allocate_matrix("energy_w_sys_ec_oct_tp", m_month[9].ec_charge.nrows() + 2, m_month[9].ec_charge.ncols() + 2);
		util::matrix_t<ssc_number_t> &energy_w_sys_ec_nov_tp = allocate_matrix("energy_w_sys_ec_nov_tp", m_month[10].ec_charge.nrows() + 2, m_month[10].ec_charge.ncols() + 2);
		util::matrix_t<ssc_number_t> &energy_w_sys_ec_dec_tp = allocate_matrix("energy_w_sys_ec_dec_tp", m_month[11].ec_charge.nrows() + 2, m_month[11].ec_charge.ncols() + 2);




		// lifetime hourly load
		ssc_number_t *lifetime_hourly_load = allocate("lifetime_load", nrec_gen);




		// false = 2 meters, load and system treated separately
		// true = 1 meter, net grid energy used for bill calculation with either energy or dollar rollover.
		//			bool enable_nm = as_boolean("ur_enable_net_metering");
		int metering_option = as_integer("ur_metering_option");
		bool enable_nm = (metering_option == 0 || metering_option == 1);
		bool hourly_reconciliation = (metering_option == 3);


		idx = 0;
		for (i=0;i<nyears;i++)
		{
			for (j=0;j<8760;j++)
			{
				/* for future implementation for lifetime loads
				// update e_load and p_load per year if lifetime output
				if ((as_integer("system_use_lifetime_output") == 1) && (idx < nrec_load))
				{
					e_load[j] = p_load[j] = 0.0;
					for (size_t ii = 0; (ii < step_per_hour_load) && (idx < nrec_load); ii++)
					{
						ts_load = (bload ? pload[idx] : 0);
						e_load[i] += ts_load * ts_hour_load;
						p_load[i] = ((ts_load > p_load[i]) ? ts_load : p_load[i]);
						idx++;
					}
					lifetime_hourly_load[i*8760 + j] = e_load[i];
					// sign correction for utility rate calculations
					e_load[i] = -e_load[i];
					p_load[i] = -p_load[i];
				}
				*/
				// apply load escalation appropriate for current year
				e_load_cy[j] = e_load[j] * load_scale[i];
				p_load_cy[j] = p_load[j] * load_scale[i];

				
				// update e_sys per year if lifetime output
				if ((as_integer("system_use_lifetime_output") == 1) )
				{
					e_sys[j] = p_sys[j] = 0.0;
					for (size_t ii = 0; (ii < step_per_hour_gen); ii++)
					{
						ts_power = pgen[idx];
						e_sys[j] += ts_power * ts_hour_gen;
						p_sys[j] = ((ts_power > p_sys[j]) ? ts_power : p_sys[j]);
						// until lifetime load fully implemented
						lifetime_hourly_load[idx] = -e_load_cy[j];
						idx++;
					}
				}

				// calculate e_grid value (e_sys + e_load)
				e_sys_cy[j] = e_sys[j] * sys_scale[i];
				p_sys_cy[j] = p_sys[j] * sys_scale[i];
				// note: load is assumed to have negative sign
				e_grid[j] = e_sys_cy[j] + e_load_cy[j];
				p_grid[j] = p_sys_cy[j] + p_load_cy[j];
			}

			// now calculate revenue without solar system (using load only)
			if (hourly_reconciliation)
			{
				ur_calc_hourly(&e_load_cy[0], &p_load_cy[0],
					&revenue_wo_sys[0], &payment[0], &income[0], &price[0], &demand_charge[0], &energy_charge[0],
					&monthly_fixed_charges[0], &monthly_minimum_charges[0],
					&monthly_dc_fixed[0], &monthly_dc_tou[0],
					&monthly_ec_charges[0], &monthly_ec_flat_charges[0], &dc_hourly_peak[0], &monthly_cumulative_excess_energy[0], &monthly_cumulative_excess_dollars[0], &monthly_bill[0], rate_scale[i]);
			}
			else
			{
				ur_calc(&e_load_cy[0], &p_load_cy[0],
					&revenue_wo_sys[0], &payment[0], &income[0], &price[0], &demand_charge[0], &energy_charge[0],
					&monthly_fixed_charges[0], &monthly_minimum_charges[0],
					&monthly_dc_fixed[0], &monthly_dc_tou[0],
					&monthly_ec_charges[0], &monthly_ec_flat_charges[0], &dc_hourly_peak[0], &monthly_cumulative_excess_energy[0], &monthly_cumulative_excess_dollars[0], &monthly_bill[0], rate_scale[i]);
			}
	


			for (j = 0; j < 12; j++)
			{
				utility_bill_wo_sys_ym[(i + 1) * 12 + j] = monthly_bill[j];
				ch_wo_sys_dc_fixed_ym[(i + 1) * 12 + j] = monthly_dc_fixed[j];
				ch_wo_sys_dc_tou_ym[(i + 1) * 12 + j] = monthly_dc_tou[j];
				ch_wo_sys_ec_ym[(i + 1) * 12 + j] = monthly_ec_charges[j];
				ch_wo_sys_ec_flat_ym[(i + 1) * 12 + j] = monthly_ec_flat_charges[j];
				ch_wo_sys_fixed_ym[(i + 1) * 12 + j] = monthly_fixed_charges[j];
				ch_wo_sys_minimum_ym[(i + 1) * 12 + j] = monthly_minimum_charges[j];

				utility_bill_wo_sys[i + 1] += monthly_bill[j];
				ch_wo_sys_dc_fixed[i + 1] += monthly_dc_fixed[j];
				ch_wo_sys_dc_tou[i + 1] += monthly_dc_tou[j];
				ch_wo_sys_ec[i + 1] += monthly_ec_charges[j];
				ch_wo_sys_ec_flat[i + 1] += monthly_ec_flat_charges[j];
				ch_wo_sys_fixed[i + 1] += monthly_fixed_charges[j];
				ch_wo_sys_minimum[i + 1] += monthly_minimum_charges[j];
			}








			if (i == 0)
			{
				ur_update_ec_monthly(0, charge_wo_sys_ec_jan_tp, energy_wo_sys_ec_jan_tp);
				ur_update_ec_monthly(1, charge_wo_sys_ec_feb_tp, energy_wo_sys_ec_feb_tp);
				ur_update_ec_monthly(2, charge_wo_sys_ec_mar_tp, energy_wo_sys_ec_mar_tp);
				ur_update_ec_monthly(3, charge_wo_sys_ec_apr_tp, energy_wo_sys_ec_apr_tp);
				ur_update_ec_monthly(4, charge_wo_sys_ec_may_tp, energy_wo_sys_ec_may_tp);
				ur_update_ec_monthly(5, charge_wo_sys_ec_jun_tp, energy_wo_sys_ec_jun_tp);
				ur_update_ec_monthly(6, charge_wo_sys_ec_jul_tp, energy_wo_sys_ec_jul_tp);
				ur_update_ec_monthly(7, charge_wo_sys_ec_aug_tp, energy_wo_sys_ec_aug_tp);
				ur_update_ec_monthly(8, charge_wo_sys_ec_sep_tp, energy_wo_sys_ec_sep_tp);
				ur_update_ec_monthly(9, charge_wo_sys_ec_oct_tp, energy_wo_sys_ec_oct_tp);
				ur_update_ec_monthly(10, charge_wo_sys_ec_nov_tp, energy_wo_sys_ec_nov_tp);
				ur_update_ec_monthly(11, charge_wo_sys_ec_dec_tp, energy_wo_sys_ec_dec_tp);

				//assign( "year1_hourly_revenue_without_system", var_data( &revenue_wo_sys[0], 8760 ) );
				//assign( "year1_hourly_payment_without_system", var_data( &payment[0], 8760 ) );
				//assign( "year1_hourly_income_without_system", var_data( &income[0], 8760 ) );
				//assign( "year1_hourly_price_without_system", var_data( &price[0], 8760 ) );
				assign("year1_hourly_dc_without_system", var_data(&demand_charge[0], 8760));
				assign("year1_hourly_ec_without_system", var_data(&energy_charge[0], 8760));

				assign( "year1_monthly_dc_fixed_without_system", var_data(&monthly_dc_fixed[0], 12) );
				assign( "year1_monthly_dc_tou_without_system", var_data(&monthly_dc_tou[0], 12) );
				assign("year1_monthly_ec_charge_without_system", var_data(&monthly_ec_charges[0], 12));
				assign("year1_monthly_ec_charge_flat_without_system", var_data(&monthly_ec_flat_charges[0], 12));

				// sign reversal based on 9/5/13 meeting, reverse again 9/6/13
				for (int ii=0;ii<8760;ii++) 
				{
					salespurchases[ii] = revenue_wo_sys[ii];
				}
				int c = 0;
				for (int m=0;m<12;m++)
				{
					monthly_salespurchases[m] = 0;
					for (int d=0;d<util::nday[m];d++)
					{
						for(int h=0;h<24;h++)
						{
							monthly_salespurchases[m] += salespurchases[c];
							c++;
						}
					}
				}
				assign( "year1_hourly_salespurchases_without_system", var_data( &salespurchases[0], 8760 ) );
//				assign("year1_monthly_salespurchases_wo_sys", var_data(&monthly_salespurchases[0], 12));
				assign("year1_monthly_utility_bill_wo_sys", var_data(&monthly_bill[0], 12));
				assign("year1_monthly_fixed_without_system", var_data(&monthly_fixed_charges[0], 12));
				assign("year1_monthly_minimum_without_system", var_data(&monthly_minimum_charges[0], 12));
			}


// with system

			if (hourly_reconciliation)
			{
				ur_calc_hourly(&e_grid[0], &p_grid[0],
					&revenue_w_sys[0], &payment[0], &income[0], &price[0], &demand_charge[0],
					&energy_charge[0],
					&monthly_fixed_charges[0], &monthly_minimum_charges[0],
					&monthly_dc_fixed[0], &monthly_dc_tou[0],
					&monthly_ec_charges[0], &monthly_ec_flat_charges[0], &dc_hourly_peak[0], &monthly_cumulative_excess_energy[0], &monthly_cumulative_excess_dollars[0], &monthly_bill[0], rate_scale[i]);
			}
			else // monthly reconciliation per 2015.6.30 release
			{

				if (enable_nm)
				{
					// calculate revenue with solar system (using net grid energy & maxpower)
					ur_calc(&e_grid[0], &p_grid[0],
						&revenue_w_sys[0], &payment[0], &income[0], &price[0], &demand_charge[0],
						&energy_charge[0],
						&monthly_fixed_charges[0], &monthly_minimum_charges[0],
						&monthly_dc_fixed[0], &monthly_dc_tou[0],
						&monthly_ec_charges[0], &monthly_ec_flat_charges[0], &dc_hourly_peak[0], &monthly_cumulative_excess_energy[0], &monthly_cumulative_excess_dollars[0], &monthly_bill[0],  rate_scale[i]);
				}
				else
				{
					// calculate revenue with solar system (using system energy & maxpower)
					ur_calc(&e_sys_cy[0], &p_sys_cy[0],
						&revenue_w_sys[0], &payment[0], &income[0], &price[0], &demand_charge[0],
						&energy_charge[0],
						&monthly_fixed_charges[0], &monthly_minimum_charges[0],
						&monthly_dc_fixed[0], &monthly_dc_tou[0],
						&monthly_ec_charges[0], &monthly_ec_flat_charges[0], &dc_hourly_peak[0], &monthly_cumulative_excess_energy[0], &monthly_cumulative_excess_dollars[0], &monthly_bill[0],  rate_scale[i], false, false);
					// TODO - remove annual_revenue and just use annual bill
					// Two meters - adjust output accordingly
					for (j = 0; j < 8760; j++)
					{
						revenue_w_sys[j] += revenue_wo_sys[j]; // watch sign
						annual_revenue_w_sys[i + 1] += revenue_w_sys[j] - revenue_wo_sys[j];
					}
					// adjust monthly outputs as sum of both meters = system meter + load meter 

					for (j = 0; j < 12; j++)
					{
						monthly_dc_fixed[j] += ch_wo_sys_dc_fixed_ym[(i + 1) * 12 + j];
						monthly_dc_tou[j] += ch_wo_sys_dc_tou_ym[(i + 1) * 12 + j];
						monthly_ec_charges[j] += ch_wo_sys_ec_ym[(i + 1) * 12 + j];
						monthly_ec_flat_charges[j] += ch_wo_sys_ec_flat_ym[(i + 1) * 12 + j];
						monthly_fixed_charges[j] += ch_wo_sys_fixed_ym[(i + 1) * 12 + j];
						monthly_minimum_charges[j] += ch_wo_sys_minimum_ym[(i + 1) * 12 + j];
						monthly_bill[j] += utility_bill_wo_sys_ym[(i + 1) * 12 + j];
					}

					if (i == 0)
					{

						// for each month add the wo system charge and energy
						// not that first row contains tier num and first column contains period numbers in the charge_wo_sys_ec and energy_wo_sys_ec matrices
						for (int m = 0; m < (int)m_month.size(); m++)
						{
							for (int ir = 0; ir < (int)m_month[m].ec_charge.nrows(); ir++)
							{
								for (int ic = 0; ic < (int)m_month[m].ec_charge.ncols(); ic++)
								{
									ssc_number_t charge_adj = 0;
									ssc_number_t energy_adj = 0;
									switch (m)
									{
									case 0:
										charge_adj = charge_wo_sys_ec_jan_tp.at(ir + 1, ic + 1);
										energy_adj = energy_wo_sys_ec_jan_tp.at(ir + 1, ic + 1);
										break;
									case 1:
										charge_adj = charge_wo_sys_ec_feb_tp.at(ir + 1, ic + 1);
										energy_adj = energy_wo_sys_ec_feb_tp.at(ir + 1, ic + 1);
										break;
									case 2:
										charge_adj = charge_wo_sys_ec_mar_tp.at(ir + 1, ic + 1);
										energy_adj = energy_wo_sys_ec_mar_tp.at(ir + 1, ic + 1);
										break;
									case 3:
										charge_adj = charge_wo_sys_ec_apr_tp.at(ir + 1, ic + 1);
										energy_adj = energy_wo_sys_ec_apr_tp.at(ir + 1, ic + 1);
										break;
									case 4:
										charge_adj = charge_wo_sys_ec_may_tp.at(ir + 1, ic + 1);
										energy_adj = energy_wo_sys_ec_may_tp.at(ir + 1, ic + 1);
										break;
									case 5:
										charge_adj = charge_wo_sys_ec_jun_tp.at(ir + 1, ic + 1);
										energy_adj = energy_wo_sys_ec_jun_tp.at(ir + 1, ic + 1);
										break;
									case 6:
										charge_adj = charge_wo_sys_ec_jul_tp.at(ir + 1, ic + 1);
										energy_adj = energy_wo_sys_ec_jul_tp.at(ir + 1, ic + 1);
										break;
									case 7:
										charge_adj = charge_wo_sys_ec_aug_tp.at(ir + 1, ic + 1);
										energy_adj = energy_wo_sys_ec_aug_tp.at(ir + 1, ic + 1);
										break;
									case 8:
										charge_adj = charge_wo_sys_ec_sep_tp.at(ir + 1, ic + 1);
										energy_adj = energy_wo_sys_ec_sep_tp.at(ir + 1, ic + 1);
										break;
									case 9:
										charge_adj = charge_wo_sys_ec_oct_tp.at(ir + 1, ic + 1);
										energy_adj = energy_wo_sys_ec_oct_tp.at(ir + 1, ic + 1);
										break;
									case 10:
										charge_adj = charge_wo_sys_ec_nov_tp.at(ir + 1, ic + 1);
										energy_adj = energy_wo_sys_ec_nov_tp.at(ir + 1, ic + 1);
										break;
									case 11:
										charge_adj = charge_wo_sys_ec_dec_tp.at(ir + 1, ic + 1);
										energy_adj = energy_wo_sys_ec_dec_tp.at(ir + 1, ic + 1);
										break;
									}
									m_month[m].ec_charge.at(ir, ic) += charge_adj;
									m_month[m].ec_energy_use.at(ir, ic) += energy_adj;
								}
							}
						}

						/* 
						for (int p = 0; p < 12; p++)
						{
							for (int t = 0; t < 6; t++)
							{
//								monthly_charge_period_tier[0][p][t] += charge_wo_sys_ec_jan_tp[t * 12 + p];
								monthly_e_use_period_tier[0][p][t] += energy_wo_sys_ec_jan_tp[t * 12 + p];
							}
						}
						*/


					}
				} // non net metering with monthly reconciliation
			} // monthly reconciliation

			if (i == 0)
			{
				ur_update_ec_monthly(0, charge_w_sys_ec_jan_tp, energy_w_sys_ec_jan_tp);
				ur_update_ec_monthly(1, charge_w_sys_ec_feb_tp, energy_w_sys_ec_feb_tp);
				ur_update_ec_monthly(2, charge_w_sys_ec_mar_tp, energy_w_sys_ec_mar_tp);
				ur_update_ec_monthly(3, charge_w_sys_ec_apr_tp, energy_w_sys_ec_apr_tp);
				ur_update_ec_monthly(4, charge_w_sys_ec_may_tp, energy_w_sys_ec_may_tp);
				ur_update_ec_monthly(5, charge_w_sys_ec_jun_tp, energy_w_sys_ec_jun_tp);
				ur_update_ec_monthly(6, charge_w_sys_ec_jul_tp, energy_w_sys_ec_jul_tp);
				ur_update_ec_monthly(7, charge_w_sys_ec_aug_tp, energy_w_sys_ec_aug_tp);
				ur_update_ec_monthly(8, charge_w_sys_ec_sep_tp, energy_w_sys_ec_sep_tp);
				ur_update_ec_monthly(9, charge_w_sys_ec_oct_tp, energy_w_sys_ec_oct_tp);
				ur_update_ec_monthly(10, charge_w_sys_ec_nov_tp, energy_w_sys_ec_nov_tp);
				ur_update_ec_monthly(11, charge_w_sys_ec_dec_tp, energy_w_sys_ec_dec_tp);


				//assign( "year1_hourly_revenue_with_system", var_data( &revenue_w_sys[0], 8760 ) );
				//assign( "year1_hourly_payment_with_system", var_data( &payment[0], 8760 ) );
				//assign( "year1_hourly_income_with_system", var_data( &income[0], 8760 ) );
				//assign( "year1_hourly_price_with_system", var_data( &price[0], 8760 ) );
				assign("year1_hourly_dc_with_system", var_data(&demand_charge[0], 8760));
				assign("year1_hourly_ec_with_system", var_data(&energy_charge[0], 8760));
				//				assign( "year1_hourly_e_grid", var_data( &e_grid[0], 8760 ) );
				//				assign( "year1_hourly_p_grid", var_data( &p_grid[0], 8760 ) );
				assign("year1_hourly_ec_tou_schedule", var_data(&ec_tou_sched[0], 8760));
				assign("year1_hourly_dc_tou_schedule", var_data(&dc_tou_sched[0], 8760));
				assign("year1_hourly_dc_peak_per_period", var_data(&dc_hourly_peak[0], 8760));

				// sign reversal based on 9/5/13 meeting reverse again 9/6/13
				for (int ii = 0; ii<8760; ii++)
				{
					load[ii] = -e_load[ii];
					e_tofromgrid[ii] = e_grid[ii];
					p_tofromgrid[ii] = p_grid[ii];
					salespurchases[ii] = revenue_w_sys[ii];
				}
				// monthly outputs - Paul and Sean 7/29/13 - updated 8/9/13 and 8/12/13 and 9/10/13
				monthly_outputs(&e_load[0], &e_sys_cy[0], &e_grid[0], &salespurchases[0],
					&monthly_load[0], &monthly_system_generation[0], &monthly_elec_to_grid[0],
					&monthly_elec_needed_from_grid[0],
					&monthly_salespurchases[0]);

				assign("year1_hourly_e_tofromgrid", var_data(&e_tofromgrid[0], 8760));
				assign("year1_hourly_p_tofromgrid", var_data(&p_tofromgrid[0], 8760));
				assign("year1_hourly_load", var_data(&load[0], 8760));
				assign("year1_hourly_salespurchases_with_system", var_data(&salespurchases[0], 8760));
				assign("year1_monthly_load", var_data(&monthly_load[0], 12));
				assign("year1_monthly_system_generation", var_data(&monthly_system_generation[0], 12));
				assign("year1_monthly_electricity_to_grid", var_data(&monthly_elec_to_grid[0], 12));
				assign("year1_monthly_electricity_needed_from_grid", var_data(&monthly_elec_needed_from_grid[0], 12));

				assign("year1_monthly_cumulative_excess_generation", var_data(&monthly_cumulative_excess_energy[0], 12));
				assign("year1_monthly_cumulative_excess_dollars", var_data(&monthly_cumulative_excess_dollars[0], 12));
//				assign("year1_monthly_salespurchases", var_data(&monthly_salespurchases[0], 12));
				assign("year1_monthly_utility_bill_w_sys", var_data(&monthly_bill[0], 12));

				// output and demand per Paul's email 9/10/10
				// positive demand indicates system does not produce enough electricity to meet load
				// zero if the system produces more than the demand
				std::vector<ssc_number_t> output(8760), edemand(8760), pdemand(8760), e_sys_to_grid(8760), e_sys_to_load(8760), p_sys_to_load(8760);
				for (j = 0; j<8760; j++)
				{
					output[j] = e_sys_cy[j];
					edemand[j] = e_grid[j] < 0.0 ? -e_grid[j] : (ssc_number_t)0.0;
					pdemand[j] = p_grid[j] < 0.0 ? -p_grid[j] : (ssc_number_t)0.0;

					ssc_number_t sys_e_net = output[j] + e_load[j];// loads are assumed negative
					e_sys_to_grid[j] = sys_e_net > 0 ? sys_e_net : (ssc_number_t)0.0;
					e_sys_to_load[j] = sys_e_net > 0 ? -e_load[j] : output[j];

					ssc_number_t sys_p_net = output[j] + p_load[j];// loads are assumed negative
					p_sys_to_load[j] = sys_p_net > 0 ? -p_load[j] : output[j];
				}

				assign("year1_hourly_system_output", var_data(&output[0], 8760));
				assign("year1_hourly_e_demand", var_data(&edemand[0], 8760));
				assign("year1_hourly_p_demand", var_data(&pdemand[0], 8760));

				assign("year1_hourly_system_to_grid", var_data(&e_sys_to_grid[0], 8760));
				assign("year1_hourly_system_to_load", var_data(&e_sys_to_load[0], 8760));
				assign("year1_hourly_p_system_to_load", var_data(&p_sys_to_load[0], 8760));

				assign("year1_monthly_fixed_with_system", var_data(&monthly_fixed_charges[0], 12));
				assign("year1_monthly_minimum_with_system", var_data(&monthly_minimum_charges[0], 12));
				assign("year1_monthly_dc_fixed_with_system", var_data(&monthly_dc_fixed[0], 12));
				assign("year1_monthly_dc_tou_with_system", var_data(&monthly_dc_tou[0], 12));
				assign("year1_monthly_ec_charge_with_system", var_data(&monthly_ec_charges[0], 12));
				assign("year1_monthly_ec_charge_flat_with_system", var_data(&monthly_ec_flat_charges[0], 12));
			}

			// determine net-revenue benefit due to solar for year 'i'
			
			annual_net_revenue[i+1] = 0.0;
			annual_electric_load[i + 1] = 0.0;
			energy_net[i + 1] = 0.0;
			annual_revenue_w_sys[i + 1] = 0.0;
			annual_revenue_wo_sys[i + 1] = 0.0;

			for(j=0;j<8760;j++)
			{
				energy_net[i + 1] +=  e_sys_cy[j];
				annual_net_revenue[i + 1] += revenue_w_sys[j] - revenue_wo_sys[j];
				annual_electric_load[i + 1] += -e_load_cy[j];
				annual_revenue_w_sys[i + 1] += revenue_w_sys[j];
				annual_revenue_wo_sys[i + 1] += revenue_wo_sys[j];
			}

			//Outputs from Paul, Nate and Sean 9/9/13
			annual_elec_cost_w_sys[i + 1] = -annual_revenue_w_sys[i+1];
			annual_elec_cost_wo_sys[i + 1] = -annual_revenue_wo_sys[i+1];


			for (j = 0; j < 12; j++)
			{
				utility_bill_w_sys_ym[(i+1)*12 + j] = monthly_bill[j];
				ch_w_sys_dc_fixed_ym[(i + 1) * 12 + j] = monthly_dc_fixed[j];
				ch_w_sys_dc_tou_ym[(i + 1) * 12 + j] = monthly_dc_tou[j];
				ch_w_sys_ec_ym[(i + 1) * 12 + j] = monthly_ec_charges[j];
				ch_w_sys_ec_flat_ym[(i + 1) * 12 + j] = monthly_ec_flat_charges[j];
				ch_w_sys_fixed_ym[(i + 1) * 12 + j] = monthly_fixed_charges[j];
				ch_w_sys_minimum_ym[(i + 1) * 12 + j] = monthly_minimum_charges[j];

				utility_bill_w_sys[i + 1] += monthly_bill[j];
				ch_w_sys_dc_fixed[i + 1] += monthly_dc_fixed[j];
				ch_w_sys_dc_tou[i + 1] += monthly_dc_tou[j];
				ch_w_sys_ec[i + 1] += monthly_ec_charges[j];
				ch_w_sys_ec_flat[i + 1] += monthly_ec_flat_charges[j];
				ch_w_sys_fixed[i + 1] += monthly_fixed_charges[j];
				ch_w_sys_minimum[i + 1] += monthly_minimum_charges[j];
			}


		}

		assign("elec_cost_with_system_year1", annual_elec_cost_w_sys[1]);
		assign("elec_cost_without_system_year1", annual_elec_cost_wo_sys[1]);
		assign("savings_year1", annual_elec_cost_wo_sys[1] - annual_elec_cost_w_sys[1]);
	}

	void monthly_outputs( ssc_number_t e_load[8760], ssc_number_t e_sys[8760], ssc_number_t e_grid[8760], ssc_number_t salespurchases[8760], ssc_number_t monthly_load[12], ssc_number_t monthly_generation[12], ssc_number_t monthly_elec_to_grid[12], ssc_number_t monthly_elec_needed_from_grid[12], ssc_number_t monthly_salespurchases[12])
	{
		// calculate the monthly net energy and monthly hours
		int m,d,h;
		ssc_number_t energy_use[12]; // 12 months
		int c=0;
//		bool sell_eq_buy = as_boolean("ur_sell_eq_buy");
//			bool enable_nm = as_boolean("ur_enable_net_metering");
//		int metering_option = as_integer("ur_metering_option");
//		bool enable_nm = (metering_option == 0 || metering_option == 1);


		for (m=0;m<12;m++)
		{
			energy_use[m] = 0;
			monthly_load[m] = 0;
			monthly_generation[m] = 0;
			monthly_elec_to_grid[m] = 0;
			monthly_salespurchases[m] = 0;
			for (d=0;d<util::nday[m];d++)
			{
				for(h=0;h<24;h++)
				{
					// net energy use per month
					energy_use[m] += e_grid[c];
					// Sean's sign convention
					monthly_load[m] -= e_load[c];
					monthly_generation[m] += e_sys[c]; // does not include first year sys_scale
					monthly_elec_to_grid[m] += e_grid[c];
// 9/10/13 update from Paul
					monthly_salespurchases[m] += salespurchases[c];
					c++;
				}
			}
		}
		//
		
		for (m=0;m<12;m++)
		{
			if (monthly_elec_to_grid[m] > 0)
				monthly_elec_needed_from_grid[m] = monthly_elec_to_grid[m];
			else
				monthly_elec_needed_from_grid[m]=0;
		}
	}

	void setup()
	{
		size_t nrows, ncols, r, c, m, i, j;
		int period, tier, month;
		util::matrix_t<float> dc_schedwkday(12, 24, 1);
		util::matrix_t<float> dc_schedwkend(12, 24, 1);

		for (i = 0; i < m_ec_periods_tiers.size(); i++)
			m_ec_periods_tiers[i].clear();
		m_ec_periods.clear();

		for (i = 0; i < m_dc_tou_periods_tiers.size(); i++)
			m_dc_tou_periods_tiers[i].clear();
		m_dc_tou_periods.clear();

		for (i = 0; i < m_dc_flat_tiers.size(); i++)
			m_dc_flat_tiers[i].clear();

		m_month.clear();
		for (m = 0; m < 12; m++)
		{
			ur_month urm;
			m_month.push_back(urm);
		}


		bool ec_enabled = as_boolean("ur_ec_enable");
		bool dc_enabled = as_boolean("ur_dc_enable");

		if (ec_enabled)
		{

			ssc_number_t *ec_weekday = as_matrix("ur_ec_sched_weekday", &nrows, &ncols);
			if (nrows != 12 || ncols != 24)
			{
				std::ostringstream ss;
				ss << "energy charge weekday schedule must be 12x24, input is " << nrows << "x" << ncols;
				throw exec_error("utilityrate3", ss.str());
			}
			ssc_number_t *ec_weekend = as_matrix("ur_ec_sched_weekend", &nrows, &ncols);
			if (nrows != 12 || ncols != 24)
			{
				std::ostringstream ss;
				ss << "energy charge weekend schedule must be 12x24, input is " << nrows << "x" << ncols;
				throw exec_error("utilityrate3", ss.str());
			}
			util::matrix_t<float> ec_schedwkday(nrows, ncols);
			ec_schedwkday.assign(ec_weekday, nrows, ncols);
			util::matrix_t<float> ec_schedwkend(nrows, ncols);
			ec_schedwkend.assign(ec_weekend, nrows, ncols);

			// for each row (month) determine periods in the month
			// m_monthly_ec_tou_ub max of period tier matrix of period xtier +1
			// columns are period, tier1 max, tier 2 max, ..., tier n max


			int ec_tod[8760];

			if (!util::translate_schedule(ec_tod, ec_schedwkday, ec_schedwkend, 1, 12))
				throw general_error("could not translate weekday and weekend schedules for energy charges");

			for (i = 0; i < 8760; i++) 
				m_ec_tou_sched.push_back(ec_tod[i]);


			// 6 columns period, tier, max usage, max usage units, buy, sell
			ssc_number_t *ec_tou_in = as_matrix("ur_ec_tou_mat", &nrows, &ncols);
			if (ncols != 6)
			{
				std::ostringstream ss;
				ss << "energy tou inputs must have 6 columns, input has " << ncols << "columns";
				throw exec_error("utilityrate3", ss.str());
			}
			util::matrix_t<float> ec_tou_mat(nrows, ncols);
			ec_tou_mat.assign(ec_tou_in, nrows, ncols);


			// adjust sell rate based on input selections
			int metering_option = as_integer("ur_metering_option");
			bool enable_nm = (metering_option == 0 || metering_option == 1);
			// 0 = net metering energy rollover, 1=net metering dollar rollover
			// 2= non-net metering monthly, 3= non-net metering hourly

			// non net metering only
			int ur_ec_sell_rate_option = as_integer("ur_ec_sell_rate_option");
			// 0=sell at ec sell rates, 1= sell at flat sell rate
			bool ur_ec_sell_at_ec_rates = (ur_ec_sell_rate_option == 0);
			ssc_number_t ur_ec_single_sell_rate = as_number("ur_ec_single_sell_rate");

			bool sell_eq_buy = enable_nm;

			// find all periods for each month m through schedules
			for (m = 0; m < m_month.size(); m++)
			{
				// energy charges
				for (c = 0; c < ec_schedwkday.ncols(); c++)
				{
					if (std::find(std::begin(m_month[m].ec_periods), std::end(m_month[m].ec_periods), ec_schedwkday.at(m, c)) == std::end(m_month[m].ec_periods))
						m_month[m].ec_periods.push_back((int)ec_schedwkday.at(m, c));
				}
				for (c = 0; c < ec_schedwkend.ncols(); c++)
				{
					if (std::find(std::begin(m_month[m].ec_periods), std::end(m_month[m].ec_periods), ec_schedwkend.at(m, c)) == std::end(m_month[m].ec_periods))
						m_month[m].ec_periods.push_back((int)ec_schedwkend.at(m, c));
				}
				std::sort(m_month[m].ec_periods.begin(), m_month[m].ec_periods.end());
			}

			for (r = 0; r < nrows; r++)
			{
				period = (int)ec_tou_mat.at(r, 0);
				if (std::find(std::begin(m_ec_periods), std::end(m_ec_periods), period) == std::end(m_ec_periods))
					m_ec_periods.push_back(period);
			}
			// sorted periods smallest to largest
			std::sort(m_ec_periods.begin(), m_ec_periods.end());
			// for each period, get list of tier numbers and then sort and construct 
			//m_ec_tou_ub, m_ec_tou_units, m_ec_tou_br, ec_tou_sr vectors of vectors
			
			for (r = 0; r < m_ec_periods.size(); r++)
			{
				m_ec_periods_tiers.push_back(std::vector<int>());
			}
			
			for (r = 0; r < nrows; r++)
			{
				period = (int)ec_tou_mat.at(r, 0);
				tier = (int)ec_tou_mat.at(r, 1);
				std::vector<int>::iterator result = std::find(std::begin(m_ec_periods), std::end(m_ec_periods), period);
				if (result == std::end(m_ec_periods))
				{
					std::ostringstream ss;
					ss << "energy charge period not found " << period;
					throw exec_error("utilityrate3", ss.str());
				}
				int ndx = (int)(result - m_ec_periods.begin());
				m_ec_periods_tiers[ndx].push_back(tier);
			}
			// sort tier values for each period
			for (r = 0; r < m_ec_periods_tiers.size(); r++)
				std::sort(m_ec_periods_tiers[r].begin(), m_ec_periods_tiers[r].end());


			// periods are rows and tiers are columns - note that columns can change based on rows
			// Initialize each month variables that are constant over the simulation

			for (m = 0; m < m_month.size(); m++)
			{
				int num_periods=0;
				int num_tiers = 0;
				for (i = 0; i < m_month[m].ec_periods.size(); i++)
				{
					// find all periods and check that number of tiers the same for all for the month, if not through error
					std::vector<int>::iterator per_num = std::find(std::begin(m_ec_periods), std::end(m_ec_periods), m_month[m].ec_periods[i]);
					if (per_num == std::end(m_ec_periods))
					{
						std::ostringstream ss;
						ss << "energy charge period not found for month " << " m and period " << m_month[m].ec_periods[i];
						throw exec_error("utilityrate3", ss.str());
					}
					period = (*per_num);
					int ndx = (int)(per_num - m_ec_periods.begin());
					if (i == 0)
					{
						// redimension ec_ field of ur_month class
						num_periods = (int)m_month[m].ec_periods.size();
						num_tiers = (int)m_ec_periods_tiers[ndx].size();
						m_month[m].ec_tou_ub.resize_fill(num_periods, num_tiers, (ssc_number_t)1e+38);
						m_month[m].ec_tou_units.resize_fill(num_periods, num_tiers, 0); // kWh
						m_month[m].ec_tou_br.resize_fill(num_periods, num_tiers, 0);
						m_month[m].ec_tou_sr.resize_fill(num_periods, num_tiers, 0);
					}
					else
					{
						if ((int)m_ec_periods_tiers[ndx].size() != num_tiers)
						{
							std::ostringstream ss;
							ss << "energy charge number of tiers " << m_ec_periods_tiers[ndx].size() << ", incorrect for month " << m << " and period " << m_month[m].ec_periods[i] << " should be " << num_tiers;
							throw exec_error("utilityrate3", ss.str());
						}
					}
					for (j = 0; j < m_ec_periods_tiers[ndx].size(); j++)
					{
						tier = m_ec_periods_tiers[ndx][j];
						// initialize for each period and tier
						bool found = false;
						for (r = 0; (r < nrows) && !found; r++)
						{
							if ((period == (int)ec_tou_mat.at(r, 0)) 
								&& (tier == (int)ec_tou_mat.at(r, 1)))
							{
								m_month[m].ec_tou_ub.at(i,j) = ec_tou_mat.at(r, 2);
								m_month[m].ec_tou_units.at(i, j) = (int)ec_tou_mat.at(r, 3);
								m_month[m].ec_tou_br.at(i, j) = ec_tou_mat.at(r, 4);
								// adjust sell rate based on input selections
								ssc_number_t sell = ec_tou_mat.at(r, 5);
								if (sell_eq_buy)
									sell = ec_tou_mat.at(r, 4);
								else if (!ur_ec_sell_at_ec_rates)
									sell = ur_ec_single_sell_rate;
								m_month[m].ec_tou_sr.at(i, j) = sell;
								found = true;
							}
						}

					}
				}
			}

		}


// demand charge initialization
		if (dc_enabled)
		{

			ssc_number_t *dc_weekday = as_matrix("ur_dc_sched_weekday", &nrows, &ncols);
			if (nrows != 12 || ncols != 24)
			{
				std::ostringstream ss;
				ss << "demand charge weekday schedule must be 12x24, input is " << nrows << "x" << ncols;
				throw exec_error("utilityrate3", ss.str());
			}
			ssc_number_t *dc_weekend = as_matrix("ur_dc_sched_weekend", &nrows, &ncols);
			if (nrows != 12 || ncols != 24)
			{
				std::ostringstream ss;
				ss << "demand charge weekend schedule must be 12x24, input is " << nrows << "x" << ncols;
				throw exec_error("utilityrate3", ss.str());
			}
			util::matrix_t<float> dc_schedwkday(nrows, ncols);
			dc_schedwkday.assign(dc_weekday, nrows, ncols);
			util::matrix_t<float> dc_schedwkend(nrows, ncols);
			dc_schedwkend.assign(dc_weekend, nrows, ncols);

			// for each row (month) determine periods in the month
			// m_monthly_dc_tou_ub max of period tier matrix of period xtier +1
			// columns are period, tier1 max, tier 2 max, ..., tier n max


			int dc_tod[8760];

			if (!util::translate_schedule(dc_tod, dc_schedwkday, dc_schedwkend, 1, 12))
				throw general_error("could not translate weekday and weekend schedules for demand charges");

			for (i = 0; i < 8760; i++)
				m_dc_tou_sched.push_back(dc_tod[i]);


			// 4 columns period, tier, max usage, charge
			ssc_number_t *dc_tou_in = as_matrix("ur_dc_tou_mat", &nrows, &ncols);
			if (ncols != 4)
			{
				std::ostringstream ss;
				ss << "demand tou inputs must have 4 columns, input has " << ncols << "columns";
				throw exec_error("utilityrate3", ss.str());
			}
			util::matrix_t<float> dc_tou_mat(nrows, ncols);
			dc_tou_mat.assign(dc_tou_in, nrows, ncols);

			// find all periods for each month m through schedules
			for (m = 0; m < m_month.size(); m++)
			{
				// demand charges
				for (c = 0; c < dc_schedwkday.ncols(); c++)
				{
					if (std::find(std::begin(m_month[m].dc_periods), std::end(m_month[m].dc_periods), dc_schedwkday.at(m, c)) == std::end(m_month[m].dc_periods))
						m_month[m].dc_periods.push_back((int)dc_schedwkday.at(m, c));
				}
				for (c = 0; c < dc_schedwkend.ncols(); c++)
				{
					if (std::find(std::begin(m_month[m].dc_periods), std::end(m_month[m].dc_periods), dc_schedwkend.at(m, c)) == std::end(m_month[m].dc_periods))
						m_month[m].dc_periods.push_back((int)dc_schedwkend.at(m, c));
				}
				std::sort(m_month[m].dc_periods.begin(), m_month[m].dc_periods.end());
			}

			for (r = 0; r < nrows; r++)
			{
				period = (int)dc_tou_mat.at(r, 0);
				if (std::find(std::begin(m_dc_tou_periods), std::end(m_dc_tou_periods), period) == std::end(m_dc_tou_periods))
					m_dc_tou_periods.push_back(period);
			}
			// sorted periods smallest to largest
			std::sort(m_dc_tou_periods.begin(), m_dc_tou_periods.end());
			// for each period, get list of tier numbers and then sort and construct 
			//m_dc_tou_ub, m_dc_tou_units, m_dc_tou_br, dc_tou_sr vectors of vectors
			for (r = 0; r < m_dc_tou_periods.size(); r++)
			{
				m_dc_tou_periods_tiers.push_back(std::vector<int>());
			}

			for (r = 0; r < nrows; r++)
			{
				period = (int)dc_tou_mat.at(r, 0);
				tier = (int)dc_tou_mat.at(r, 1);
				std::vector<int>::iterator result = std::find(std::begin(m_dc_tou_periods), std::end(m_dc_tou_periods), period);
				if (result == std::end(m_dc_tou_periods))
				{
					std::ostringstream ss;
					ss << "demand charge period not found " << period;
					throw exec_error("utilityrate3", ss.str());
				}
				int ndx = (int)(result - m_dc_tou_periods.begin());
				m_dc_tou_periods_tiers[ndx].push_back(tier);
			}
			// sort tier values for each period
			for (r = 0; r < m_dc_tou_periods_tiers.size(); r++)
				std::sort(m_dc_tou_periods_tiers[r].begin(), m_dc_tou_periods_tiers[r].end());


			// periods are rows and tiers are columns - note that columns can change based on rows
			// Initialize each month variables that are constant over the simulation

			for (m = 0; m < m_month.size(); m++)
			{
				int num_periods = 0;
				int num_tiers = 0;
				for (i = 0; i < m_month[m].dc_periods.size(); i++)
				{
					// find all periods and check that number of tiers the same for all for the month, if not through error
					std::vector<int>::iterator per_num = std::find(std::begin(m_dc_tou_periods), std::end(m_dc_tou_periods), m_month[m].dc_periods[i]);
					if (per_num == std::end(m_dc_tou_periods))
					{
						std::ostringstream ss;
						ss << "demand charge period not found for month " << " m and period " << m_month[m].dc_periods[i];
						throw exec_error("utilityrate3", ss.str());
					}
					period = (*per_num);
					int ndx = (int)(per_num - m_dc_tou_periods.begin());
					if (i == 0)
					{
						// redimension dc_ field of ur_month class
						num_periods = (int)m_month[m].dc_periods.size();
						num_tiers = (int)m_dc_tou_periods_tiers[ndx].size();
						m_month[m].dc_tou_ub.resize_fill(num_periods, num_tiers, (ssc_number_t)1e38);
						m_month[m].dc_tou_ch.resize_fill(num_periods, num_tiers, 0); // kWh
					}
					else
					{
						if ((int)m_dc_tou_periods_tiers[ndx].size() != num_tiers)
						{
							std::ostringstream ss;
							ss << "demand charge number of tiers " << m_dc_tou_periods_tiers[ndx].size() << ", incorrect for month " << m << " and period " << m_month[m].dc_periods[i] << " should be " << num_tiers;
							throw exec_error("utilityrate3", ss.str());
						}
					}
					for (j = 0; j < m_dc_tou_periods_tiers[ndx].size(); j++)
					{
						tier = m_dc_tou_periods_tiers[ndx][j];
						// initialize for each period and tier
						bool found = false;
						for (r = 0; (r < nrows) && !found; r++)
						{
							if ((period == (int)dc_tou_mat.at(r, 0))
								&& (tier == (int)dc_tou_mat.at(r, 1)))
							{
								m_month[m].dc_tou_ub.at(i, j) = dc_tou_mat.at(r, 2);
								m_month[m].dc_tou_ch.at(i, j) = dc_tou_mat.at(r, 3);//rate_esc;
								found = true;
							}
						}

					}
				}
			}
				// flat demand charge
				// 4 columns month, tier, max usage, charge
				ssc_number_t *dc_flat_in = as_matrix("ur_dc_flat_mat", &nrows, &ncols);
				if (ncols != 4)
				{
					std::ostringstream ss;
					ss << "demand flat inputs must have 4 columns, input has " << ncols << "columns";
					throw exec_error("utilityrate3", ss.str());
				}
				util::matrix_t<float> dc_flat_mat(nrows, ncols);
				dc_flat_mat.assign(dc_flat_in, nrows, ncols);

				for (r = 0; r < m_month.size(); r++)
				{
					m_dc_flat_tiers.push_back(std::vector<int>());
				}

				for (r = 0; r < nrows; r++)
				{
					month = (int)dc_flat_mat.at(r, 0);
					tier = (int)dc_flat_mat.at(r, 1);
					if ((month < 0) || (month >= (int)m_month.size()))
					{
						std::ostringstream ss;
						ss << "demand flt month not found " << month;
						throw exec_error("utilityrate3", ss.str());
					}
					m_dc_flat_tiers[month].push_back(tier);
				}
				// sort tier values for each period
				for (r = 0; r < m_dc_flat_tiers.size(); r++)
					std::sort(m_dc_flat_tiers[r].begin(), m_dc_flat_tiers[r].end());


				// months are rows and tiers are columns - note that columns can change based on rows
				// Initialize each month variables that are constant over the simulation



				for (m = 0; m < m_month.size(); m++)
				{
					for (j = 0; j < m_dc_flat_tiers[m].size(); j++)
					{
						tier = m_dc_flat_tiers[m][j];
						m_month[m].dc_flat_ub.clear();
						m_month[m].dc_flat_ch.clear();
						// initialize for each period and tier
						bool found = false;
						for (r = 0; (r < nrows) && !found; r++)
						{
							if ((m == dc_flat_mat.at(r, 0))
								&& (tier == (int)dc_flat_mat.at(r, 1)))
							{
								m_month[m].dc_flat_ub.push_back(dc_flat_mat.at(r, 2));
								m_month[m].dc_flat_ch.push_back(dc_flat_mat.at(r, 3));//rate_esc;
								found = true;
							}
						}

					}
				}

		}




	}




	void ur_calc( ssc_number_t e_in[8760], ssc_number_t p_in[8760],
		ssc_number_t revenue[8760], ssc_number_t payment[8760], ssc_number_t income[8760], 
		ssc_number_t price[8760], ssc_number_t demand_charge[8760], 
		ssc_number_t energy_charge[8760],
		ssc_number_t monthly_fixed_charges[12], ssc_number_t monthly_minimum_charges[12],
		ssc_number_t monthly_dc_fixed[12], ssc_number_t monthly_dc_tou[12],
		ssc_number_t monthly_ec_charges[12], ssc_number_t monthly_ec_flat_charges[12],
		ssc_number_t dc_hourly_peak[8760], ssc_number_t monthly_cumulative_excess_energy[12], 
		ssc_number_t monthly_cumulative_excess_dollars[12], ssc_number_t monthly_bill[12], 
		ssc_number_t rate_esc, bool include_fixed=true, bool include_min=true) 
		throw(general_error)
	{
		int i;

		for (i=0;i<8760;i++)
			revenue[i] = payment[i] = income[i] = price[i] = demand_charge[i] = dc_hourly_peak[i] = energy_charge[i] = 0.0;

		for (i=0;i<12;i++)
		{
			monthly_fixed_charges[i] = monthly_minimum_charges[i]
				= monthly_ec_flat_charges[i]
				= monthly_dc_fixed[i] = monthly_dc_tou[i] 
				= monthly_ec_charges[i]
				= monthly_cumulative_excess_energy[i] 
				= monthly_cumulative_excess_dollars[i] 
				= monthly_bill[i] = 0.0;
		}
		// initialize all montly values
		ssc_number_t buy = as_number("ur_flat_buy_rate")*rate_esc;
		ssc_number_t sell = as_number("ur_flat_sell_rate")*rate_esc;

		//bool sell_eq_buy = as_boolean("ur_sell_eq_buy");

	

		// false = 2 meters, load and system treated separately
		// true = 1 meter, net grid energy used for bill calculation with either energy or dollar rollover.
//		bool enable_nm = as_boolean("ur_enable_net_metering");
		//			bool enable_nm = as_boolean("ur_enable_net_metering");
		int metering_option = as_integer("ur_metering_option");
		bool enable_nm = (metering_option == 0 || metering_option == 1);
		// 0 = net metering energy rollover, 1=net metering dollar rollover
		// 2= non-net metering monthly, 3= non-net metering hourly

		// non net metering only
		int ur_ec_sell_rate_option = as_integer("ur_ec_sell_rate_option");
		// 0=sell at ec sell rates, 1= sell at flat sell rate
		bool ur_ec_sell_at_ec_rates = (ur_ec_sell_rate_option==0);
		ssc_number_t ur_ec_single_sell_rate = as_number("ur_ec_single_sell_rate")*rate_esc;

		bool sell_eq_buy = enable_nm; // update from 6/25/15 meeting

		bool ec_enabled = as_boolean("ur_ec_enable");
		bool dc_enabled = as_boolean("ur_dc_enable");

		//bool excess_monthly_dollars = (as_integer("ur_excess_monthly_energy_or_dollars") == 1);
		bool excess_monthly_dollars = (as_integer("ur_metering_option") == 1);
		//		bool apply_excess_to_flat_rate = !ec_enabled;

		if (sell_eq_buy)
			sell = buy;
		else if (!ur_ec_sell_at_ec_rates)
			sell = ur_ec_single_sell_rate*rate_esc;

		// calculate the monthly net energy and monthly hours
		int m, d, h, period, tier;
		int c = 0;
		for (m = 0; m < (int)m_month.size(); m++)
		{
			m_month[m].energy_net = 0;
			m_month[m].hours_per_month = 0;
			m_month[m].dc_flat_peak = 0;
			m_month[m].dc_flat_peak_hour = 0;
			for (d = 0; d < util::nday[m]; d++)
			{
				for (h = 0; h < 24; h++)
				{
					// net energy use per month
					m_month[m].energy_net += e_in[c]; // -load and +gen
					// hours per period per month
					m_month[m].hours_per_month++;
					// peak
					if (p_in[c] < 0 && p_in[c] < -m_month[m].dc_flat_peak)
					{
						m_month[m].dc_flat_peak = -p_in[c];
						m_month[m].dc_flat_peak_hour = c;
					}
					c++;
				}
			}
		}

		// monthly cumulative excess energy (positive = excess energy, negative = excess load)
		if (enable_nm && !excess_monthly_dollars)
		{
			ssc_number_t prev_value = 0;
			for (m = 0; m < 12; m++)
			{
				prev_value = (m > 0) ? monthly_cumulative_excess_energy[m - 1] : 0;
				monthly_cumulative_excess_energy[m] = ((prev_value + m_month[m].energy_net) > 0) ? (prev_value + m_month[m].energy_net) : 0;
			}
		}

	
		// adjust net energy if net metering with monthly rollover
		if (enable_nm && !excess_monthly_dollars)
		{
			for (m = 1; m < (int)m_month.size(); m++)
			{
				if (m_month[m].energy_net < 0)
					m_month[m].energy_net += monthly_cumulative_excess_energy[m - 1];
			}
		}


		if (ec_enabled)
		{
			// calculate the monthly net energy per tier and period based on units
			c = 0;
			ssc_number_t mon_e_net = 0;
			if (m>0 && enable_nm && !excess_monthly_dollars)
			{
				mon_e_net = monthly_cumulative_excess_energy[m - 1]; // rollover
			}
			for (m = 0; m < (int)m_month.size(); m++)
			{
				// assume kWh here initially and will update for other units
				int num_periods = (int)m_month[m].ec_tou_ub.nrows();
				int num_tiers = (int)m_month[m].ec_tou_ub.ncols();
				m_month[m].ec_energy_use.resize_fill(num_periods, num_tiers, 0);
				m_month[m].ec_charge.resize_fill(num_periods, num_tiers, 0);

				for (d = 0; d < util::nday[m]; d++)
				{
					for (h = 0; h < 24; h++)
					{
						mon_e_net += e_in[c];
						int toup = m_ec_tou_sched[c];
						std::vector<int>::iterator per_num = std::find(m_month[m].ec_periods.begin(), m_month[m].ec_periods.end(), toup);
						if (per_num == m_month[m].ec_periods.end())
						{
							std::ostringstream ss;
							ss << "energy charge period " << toup << " not found for month " << m;
							throw exec_error("utilityrate3", ss.str());
						}
						int row = (int)(per_num - m_month[m].ec_periods.begin());
						// look at energy accumulation and tier ub to determine where energy goes in matrix
						tier = 0;
						// translate to energy use for tier determination
						// update energy per tier for charges
						ssc_number_t ub = -mon_e_net;
						if (mon_e_net > 0) 
							ub = mon_e_net;

						while ((tier <  (int)(m_month[m].ec_tou_ub.ncols() - 1)) && (ub >  m_month[m].ec_tou_ub.at(row, tier)))
							tier++;
						m_month[m].ec_energy_use.at(row, tier) -= e_in[c]; // energy use - load negative and generation positive
						c++;
					}
				}
			}
		}


		// set peak per period - no tier accumulation
		if (dc_enabled)
		{
			c = 0;
			for (m = 0; m < (int)m_month.size(); m++)
			{
				m_month[m].dc_tou_peak.clear();
				m_month[m].dc_tou_peak_hour.clear();
				for (i = 0; i < (int)m_month[m].dc_periods.size(); i++)
				{
					m_month[m].dc_tou_peak.push_back(0);
					m_month[m].dc_tou_peak_hour.push_back(0);
				}
				for (d = 0; d < util::nday[m]; d++)
				{
					for (h = 0; h < 24; h++)
					{
						int todp = m_dc_tou_sched[c];
						std::vector<int>::iterator per_num = std::find(m_month[m].dc_periods.begin(), m_month[m].dc_periods.end(), todp);
						if (per_num == m_month[m].dc_periods.end())
						{
							std::ostringstream ss;
							ss << "demand charge period " << todp << " not found for month " << m;
							throw exec_error("utilityrate3", ss.str());
						}
						int row = (int)(per_num - m_month[m].dc_periods.begin());
						if (p_in[c] < 0 && p_in[c] < -m_month[m].dc_tou_peak[row])
						{
							m_month[m].dc_tou_peak[row] = -p_in[c];
							m_month[m].dc_tou_peak_hour[row] = c;
						}
						c++;
					}
				}
			}
		}


		
		
// main loop
		c = 0;
		// process one month at a time
		for (m = 0; m < (int)m_month.size(); m++)
		{
// flat rate
			if (m_month[m].hours_per_month <= 0) break;
			for (d = 0; d<util::nday[m]; d++)
			{
				for (h = 0; h<24; h++)
				{
					if (d == util::nday[m] - 1 && h == 23)
					{
						if (enable_nm)
						{
							if (m_month[m].energy_net < 0)
							{
								payment[c] += -m_month[m].energy_net * buy;
								monthly_ec_flat_charges[m] += payment[c];
							}
						}
						else // no net metering - so no rollover.
						{
							if (m_month[m].energy_net < 0) // must buy from grid
							{
								payment[c] += -m_month[m].energy_net * buy;
								monthly_ec_flat_charges[m] += payment[c];
							}
							else
							{
								income[c] += m_month[m].energy_net * sell;
								monthly_ec_flat_charges[m] -= income[c];
							}
						}
						// added for Mike Gleason 
						energy_charge[c] += monthly_ec_flat_charges[m];
// end of flat rate

// energy charge
						if (ec_enabled)
						{

							if (m_month[m].energy_net >= 0.0)
							{ // calculate income or credit
								ssc_number_t credit_amt = 0;

								for (period = 0; period < (int)m_month[m].ec_tou_sr.nrows(); period++)
								{
									for (tier = 0; tier < (int)m_month[m].ec_tou_sr.ncols(); tier++)
									{
										ssc_number_t cr = -m_month[m].ec_energy_use.at(period, tier) * m_month[m].ec_tou_sr.at(period, tier) * rate_esc;
										m_month[m].ec_charge.at(period, tier) = -cr;
										credit_amt += cr;
									}
								}
								monthly_ec_charges[m] -= credit_amt;
							}
							else
	
							{ // calculate payment or charge

								ssc_number_t charge_amt = 0;
								for (period = 0; period < (int)m_month[m].ec_tou_sr.nrows(); period++)
								{
									for (tier = 0; tier < (int)m_month[m].ec_tou_sr.ncols(); tier++)
									{
										ssc_number_t ch = m_month[m].ec_energy_use.at(period, tier) * m_month[m].ec_tou_br.at(period, tier) * rate_esc;
										m_month[m].ec_charge.at(period, tier) = ch;
										charge_amt += ch;
									}
								}
								monthly_ec_charges[m] += charge_amt;
							}


							// monthly rollover with year end sell at reduced rate
							if (enable_nm)
							{
								payment[c] += monthly_ec_charges[m];
							}
							else // non-net metering - no rollover 
							{
								if (m_month[m].energy_net < 0) // must buy from grid
									payment[c] += monthly_ec_charges[m];
								else // surplus - sell to grid
									income[c] -= monthly_ec_charges[m]; // charge is negative for income!
							}

							energy_charge[c] += monthly_ec_charges[m];

							// end of energy charge

						}


						if (dc_enabled)
						{
							// fixed demand charge
							// compute charge based on tier structure for the month
							ssc_number_t charge = 0;
							ssc_number_t d_lower = 0;
							ssc_number_t demand = m_month[m].dc_flat_peak;
							bool found = false;
							for (tier = 0; tier < (int)m_month[m].dc_flat_ub.size() && !found; tier++)
							{
								if (demand < m_month[m].dc_flat_ub[tier])
								{
									found = true;
									charge += (demand - d_lower) * 
										m_month[m].dc_flat_ch[tier] * rate_esc;
									m_month[m].dc_flat_charge = charge;
								}
								else
								{
									charge += (m_month[m].dc_flat_ub[tier] - d_lower) *
										m_month[m].dc_flat_ch[tier] * rate_esc;
									d_lower = m_month[m].dc_flat_ub[tier];
								}
							}
							
							monthly_dc_fixed[m] = charge; // redundant...
							payment[c] += monthly_dc_fixed[m];
							demand_charge[c] = charge;
							dc_hourly_peak[m_month[m].dc_flat_peak_hour] = demand;
							

							// end of fixed demand charge


							// TOU demand charge for each period find correct tier
							demand = 0;
							d_lower = 0;
							int peak_hour = 0;
							m_month[m].dc_tou_charge.clear();
							for (period = 0; period < (int)m_month[m].dc_tou_ub.nrows(); period++)
							{
								charge = 0;
								d_lower = 0;
								demand = m_month[m].dc_tou_peak[period];
								// find tier corresponding to peak demand
								bool found = false;
								for (tier = 0; tier < (int)m_month[m].dc_tou_ub.ncols() && !found; tier++)
								{
									if (demand < m_month[m].dc_tou_ub.at(period, tier))
									{
										found = true;
										charge += (demand - d_lower) * 
											m_month[m].dc_tou_ch.at(period, tier)* rate_esc;
										m_month[m].dc_tou_charge.push_back(charge);
									}
									else
									{
										charge += (m_month[m].dc_tou_ub.at(period, tier) - d_lower) * m_month[m].dc_tou_ch.at(period, tier)* rate_esc;
										d_lower = m_month[m].dc_tou_ub.at(period, tier);
									}
								}

								dc_hourly_peak[peak_hour] = demand;
								// add to payments
								monthly_dc_tou[m] += charge;
								payment[c] += charge; // apply to last hour of the month
								demand_charge[c] += charge; // add TOU charge to hourly demand charge
							}


							
							// end of TOU demand charge
						}


					} // end of if end of month
					c++;
				}  // h loop
			} // d loop

			// Calculate monthly bill (before minimums and fixed charges) and excess dollars and rollover
			monthly_bill[m] = payment[c - 1] - income[c - 1];
			if (enable_nm)
			{
				if (m > 0) monthly_bill[m] -= monthly_cumulative_excess_dollars[m - 1];
				if (monthly_bill[m] < 0)
				{
					if (excess_monthly_dollars)
						monthly_cumulative_excess_dollars[m] = -monthly_bill[m];
					monthly_bill[m] = 0;
					payment[c - 1] = 0; // fixed charges applied below
				}
			}
		} // end of month m (m loop)


		// Assumption that fixed and minimum charges independent of rollovers kWh or $
		// process monthly fixed charges
		/*
		if (include_fixed)
			process_monthly_charge(payment, monthly_fixed_charges, rate_esc);
		// process min charges
		if (include_min)
		{
			process_monthly_min(payment, monthly_minimum_charges, rate_esc);
			process_annual_min(payment, monthly_minimum_charges, rate_esc);
		}
		*/
		// compute revenue ( = income - payment ) and monthly bill ( = payment - income) and apply fixed and minimum charges
		c = 0;
		ssc_number_t mon_bill = 0, ann_bill = 0;
		ssc_number_t ann_min_charge = as_number("ur_annual_min_charge")*rate_esc;
		ssc_number_t mon_min_charge = as_number("ur_monthly_min_charge")*rate_esc;
		ssc_number_t mon_fixed = as_number("ur_monthly_fixed_charge")*rate_esc;

		// process one month at a time
		for (m = 0; m < 12; m++)
		{
			for (d = 0; d < util::nday[m]; d++)
			{
				for (h = 0; h < 24; h++)
				{
					if (d == util::nday[m] - 1 && h == 23)
					{
						// apply fixed first
						if (include_fixed)
						{
							payment[c] += mon_fixed;
							monthly_fixed_charges[m] += mon_fixed;
						}
						mon_bill = payment[c] - income[c];
						if (mon_bill < 0) mon_bill = 0; // for calculating min charge when monthly surplus.
						// apply monthly minimum
						if (include_min)
						{
							if (mon_bill < mon_min_charge)
							{
								monthly_minimum_charges[m] += mon_min_charge - mon_bill;
								payment[c] += mon_min_charge - mon_bill;
							}
						}
						ann_bill += mon_bill;
						if (m == 11)
						{
							// apply annual minimum
							if (include_min)
							{
								if (ann_bill < ann_min_charge)
								{
									monthly_minimum_charges[m] += ann_min_charge - ann_bill;
									payment[c] += ann_min_charge - ann_bill;
								}
							}
							// apply annual rollovers AFTER minimum calculations
							if (enable_nm)
							{
								// monthly rollover with year end sell at reduced rate
								if (!excess_monthly_dollars && (monthly_cumulative_excess_energy[11] > 0))
									income[8759] += monthly_cumulative_excess_energy[11] * as_number("ur_nm_yearend_sell_rate")*rate_esc;
								else if (excess_monthly_dollars && (monthly_cumulative_excess_dollars[11] > 0))
									income[8759] += monthly_cumulative_excess_dollars[11];
							}
						}
						revenue[c] = income[c] - payment[c];
						monthly_bill[m] = -revenue[c]; 
					}
					c++;
				}
			}
		}





	}



	// will be used for non net metering case to match 2015.1.30 release
	void ur_calc_hourly(ssc_number_t e_in[8760], ssc_number_t p_in[8760],
		ssc_number_t revenue[8760], ssc_number_t payment[8760], ssc_number_t income[8760],
		ssc_number_t price[8760], ssc_number_t demand_charge[8760],
		ssc_number_t energy_charge[8760],
		ssc_number_t monthly_fixed_charges[12], ssc_number_t monthly_minimum_charges[12],
		ssc_number_t monthly_dc_fixed[12], ssc_number_t monthly_dc_tou[12],
		ssc_number_t monthly_ec_charges[12], ssc_number_t monthly_ec_flat_charges[12],
		ssc_number_t dc_hourly_peak[8760], ssc_number_t monthly_cumulative_excess_energy[12],
		ssc_number_t monthly_cumulative_excess_dollars[12], ssc_number_t monthly_bill[12],
		ssc_number_t rate_esc, bool include_fixed = true, bool include_min = true)
		throw(general_error)
	{
		int i;

		for (i = 0; i<8760; i++)
			revenue[i] = payment[i] = income[i] = price[i] = demand_charge[i] = dc_hourly_peak[i] = energy_charge[i] = 0.0;

		for (i = 0; i<12; i++)
		{
			monthly_fixed_charges[i] = monthly_minimum_charges[i]
				= monthly_ec_flat_charges[i]
				= monthly_dc_fixed[i] = monthly_dc_tou[i]
				= monthly_ec_charges[i] 
				= monthly_cumulative_excess_energy[i]
				= monthly_cumulative_excess_dollars[i]
				= monthly_bill[i] = 0.0;
		}
		// initialize all montly values
		ssc_number_t buy = as_number("ur_flat_buy_rate")*rate_esc;
		ssc_number_t sell = as_number("ur_flat_sell_rate")*rate_esc;


		// non net metering only
		int ur_ec_sell_rate_option = as_integer("ur_ec_sell_rate_option");
		// 0=sell at ec sell rates, 1= sell at flat sell rate
		bool ur_ec_sell_at_ec_rates = (ur_ec_sell_rate_option == 0);
		ssc_number_t ur_ec_single_sell_rate = as_number("ur_ec_single_sell_rate")*rate_esc;

		 
		// 0=hourly (match with 2015.1.30 release, 1=monthly (most common unit in URDB), 2=daily (used for PG&E baseline rates).
		int ur_ec_ub_units = as_integer("ur_ec_ub_units");
		double daily_surplus_energy; 
		double monthly_surplus_energy; 
		double daily_deficit_energy; 
		double monthly_deficit_energy;

		bool ec_enabled = as_boolean("ur_ec_enable");
		bool dc_enabled = as_boolean("ur_dc_enable");


		if (!ur_ec_sell_at_ec_rates)
			sell = ur_ec_single_sell_rate*rate_esc;

		// calculate the monthly net energy and monthly hours
		int m, d, h, period, tier;
		int c = 0;
		for (m = 0; m < (int)m_month.size(); m++)
		{
			m_month[m].energy_net = 0;
			m_month[m].hours_per_month = 0;
			m_month[m].dc_flat_peak = 0;
			m_month[m].dc_flat_peak_hour = 0;
			for (d = 0; d < util::nday[m]; d++)
			{
				for (h = 0; h < 24; h++)
				{
					// net energy use per month
					m_month[m].energy_net += e_in[c]; // -load and +gen
					// hours per period per month
					m_month[m].hours_per_month++;
					// peak
					if (p_in[c] < 0 && p_in[c] < -m_month[m].dc_flat_peak)
					{
						m_month[m].dc_flat_peak = -p_in[c];
						m_month[m].dc_flat_peak_hour = c;
					}
					c++;
				}
			}
		}



		if (ec_enabled)
		{
			// calculate the monthly net energy per tier and period based on units
			c = 0;
			for (m = 0; m < (int)m_month.size(); m++)
			{
				// assume kWh here initially and will update for other units
				int num_periods = (int)m_month[m].ec_tou_ub.nrows();
				int num_tiers = (int)m_month[m].ec_tou_ub.ncols();
				m_month[m].ec_energy_use.resize_fill(num_periods, num_tiers, 0);
				m_month[m].ec_charge.resize_fill(num_periods, num_tiers, 0);

			}
		}


		// set peak per period - no tier accumulation
		if (dc_enabled)
		{
			c = 0;
			for (m = 0; m < (int)m_month.size(); m++)
			{
				m_month[m].dc_tou_peak.clear();
				m_month[m].dc_tou_peak_hour.clear();
				for (i = 0; i < (int)m_month[m].dc_periods.size(); i++)
				{
					m_month[m].dc_tou_peak.push_back(0);
					m_month[m].dc_tou_peak_hour.push_back(0);
				}
				for (d = 0; d < util::nday[m]; d++)
				{
					for (h = 0; h < 24; h++)
					{
						int todp = m_dc_tou_sched[c];
						std::vector<int>::iterator per_num = std::find(m_month[m].dc_periods.begin(), m_month[m].dc_periods.end(), todp);
						if (per_num == m_month[m].dc_periods.end())
						{
							std::ostringstream ss;
							ss << "demand charge period " << todp << " not found for month " << m;
							throw exec_error("utilityrate3", ss.str());
						}
						int row = (int)(per_num - m_month[m].dc_periods.begin());
						if (p_in[c] < 0 && p_in[c] < -m_month[m].dc_tou_peak[row])
						{
							m_month[m].dc_tou_peak[row] = -p_in[c];
							m_month[m].dc_tou_peak_hour[row] = c;
						}
						c++;
					}
				}
			}
		}






// main loop
		c = 0; // hourly count
		// process one hour at a time
		for (m = 0; m < 12; m++)
		{
			monthly_surplus_energy = 0;
			monthly_deficit_energy = 0;
			for (d = 0; d<util::nday[m]; d++)
			{
				daily_surplus_energy = 0;
				daily_deficit_energy = 0;
				for (h = 0; h<24; h++)
				{
					// flat rate
					if (e_in[c] < 0) // must buy from grid
					{
						payment[c] += -e_in[c] * buy;
						energy_charge[c] += payment[c];
						price[c] += buy;
					}
					else
					{
						income[c] += e_in[c] * sell;
						energy_charge[c] -= income[c];
						price[c] += sell;
					}
					monthly_ec_flat_charges[m] += energy_charge[c];
					// end of flat rate

					// energy charge
					if (ec_enabled)
					{
						period = m_ec_tou_sched[c];
						// find corresponding monthly period
						// check for valid period
						std::vector<int>::iterator per_num = std::find(m_month[m].ec_periods.begin(), m_month[m].ec_periods.end(), period);
						if (per_num == m_month[m].ec_periods.end())
						{
							std::ostringstream ss;
							ss << "energy charge period " << period << " not found for month " << m;
							throw exec_error("utilityrate3", ss.str());
						}
						int row = (int)(per_num - m_month[m].ec_periods.begin());

						if (e_in[c] >= 0.0)
						{ // calculate income or credit
							monthly_surplus_energy += e_in[c];
							daily_surplus_energy += e_in[c];

							// base period charge on units specified
							double energy_surplus = e_in[c];
							double cumulative_energy = e_in[c];
							if (ur_ec_ub_units == 1)
								cumulative_energy = monthly_surplus_energy;
							else if (ur_ec_ub_units == 2)
								cumulative_energy = daily_surplus_energy;


							// cumulative energy used to determine tier for credit of entire surplus amount
							double credit_amt = 0;
							tier = -1;
							bool found = false;
							while ((tier < 5) && !found)
							{
								tier++;
								double e_upper = m_month[m].ec_tou_ub.at(row,tier);
								if (cumulative_energy < e_upper)
									found = true;
							}
							double tier_energy = energy_surplus;
							double tier_credit = tier_energy*m_month[m].ec_tou_sr.at(row, tier);
							credit_amt += tier_credit;
							m_month[m].ec_charge.at(row, tier) -= (ssc_number_t)tier_credit;
							m_month[m].ec_energy_use.at(row, tier) -= (ssc_number_t)tier_energy;

							income[c] += (ssc_number_t)credit_amt;
							monthly_ec_charges[m] -= (ssc_number_t)credit_amt;
							price[c] += (ssc_number_t)credit_amt;
							energy_charge[c] -= (ssc_number_t)credit_amt;
						}
						else
						{ // calculate payment or charge
							monthly_deficit_energy -= e_in[c];
							daily_deficit_energy -= e_in[c];

							double charge_amt = 0;
							double energy_deficit = -e_in[c];
							// base period charge on units specified
							double cumulative_deficit = -e_in[c];
							if (ur_ec_ub_units == 1)
								cumulative_deficit = monthly_deficit_energy;
							else if (ur_ec_ub_units == 2)
								cumulative_deficit = daily_deficit_energy;


							// cumulative energy used to determine tier for credit of entire surplus amount
							tier = -1;
							bool found = false;
							while ((tier < 5) && !found)
							{
								tier++;
								double e_upper = m_month[m].ec_tou_ub.at(row, tier);
								if (cumulative_deficit < e_upper)
									found = true;
							}
							double tier_energy = energy_deficit;
							double tier_charge = tier_energy*m_month[m].ec_tou_br.at(row, tier);
							charge_amt += tier_charge;
							m_month[m].ec_energy_use.at(row, tier) += (ssc_number_t)tier_energy;
							m_month[m].ec_charge.at(row, tier) += (ssc_number_t)tier_charge;

							payment[c] += (ssc_number_t)charge_amt;
							monthly_ec_charges[m] += (ssc_number_t)charge_amt;
							price[c] += (ssc_number_t)charge_amt;
							energy_charge[c] += (ssc_number_t)charge_amt;
						}
					}
					// end of energy charge


					// demand charge - end of month only
					if (d == util::nday[m] - 1 && h == 23)
					{

						if (dc_enabled)
						{
							// fixed demand charge
							// compute charge based on tier structure for the month
							ssc_number_t charge = 0;
							ssc_number_t d_lower = 0;
							ssc_number_t demand = m_month[m].dc_flat_peak;
							bool found = false;
							for (tier = 0; tier < (int)m_month[m].dc_flat_ub.size() && !found; tier++)
							{
								if (demand < m_month[m].dc_flat_ub[tier])
								{
									found = true;
									charge += (demand - d_lower) *
										m_month[m].dc_flat_ch[tier] * rate_esc;
									m_month[m].dc_flat_charge = charge;
								}
								else
								{
									charge += (m_month[m].dc_flat_ub[tier] - d_lower) *
										m_month[m].dc_flat_ch[tier] * rate_esc;
									d_lower = m_month[m].dc_flat_ub[tier];
								}
							}

							monthly_dc_fixed[m] = charge; // redundant...
							payment[c] += monthly_dc_fixed[m];
							demand_charge[c] = charge;
							dc_hourly_peak[m_month[m].dc_flat_peak_hour] = demand;


							// end of fixed demand charge


							// TOU demand charge for each period find correct tier
							demand = 0;
							d_lower = 0;
							int peak_hour = 0;
							m_month[m].dc_tou_charge.clear();
							for (period = 0; period < (int)m_month[m].dc_tou_ub.nrows(); period++)
							{
								charge = 0;
								d_lower = 0;
								demand = m_month[m].dc_tou_peak[period];
								// find tier corresponding to peak demand
								bool found = false;
								for (tier = 0; tier < (int)m_month[m].dc_tou_ub.ncols() && !found; tier++)
								{
									if (demand < m_month[m].dc_tou_ub.at(period, tier))
									{
										found = true;
										charge += (demand - d_lower) *
											m_month[m].dc_tou_ch.at(period, tier)* rate_esc;
										m_month[m].dc_tou_charge.push_back(charge);
									}
									else
									{
										charge += (m_month[m].dc_tou_ub.at(period, tier) - d_lower) * m_month[m].dc_tou_ch.at(period, tier)* rate_esc;
										d_lower = m_month[m].dc_tou_ub.at(period, tier);
									}
								}

								dc_hourly_peak[peak_hour] = demand;
								// add to payments
								monthly_dc_tou[m] += charge;
								payment[c] += charge; // apply to last hour of the month
								demand_charge[c] += charge; // add TOU charge to hourly demand charge
							}



							// end of TOU demand charge
							// end of TOU demand charge
						} // if demand charges enabled (dc_enabled)
					}	// end of demand charges at end of month

					c++;
				}  // h loop
			} // d loop

			// Calculate monthly bill (before minimums and fixed charges) and excess dollars and rollover
			monthly_bill[m] = monthly_ec_flat_charges[m] + monthly_ec_charges[m] + monthly_dc_fixed[m] + monthly_dc_tou[m];
		} // end of month m (m loop)


		// Assumption that fixed and minimum charges independent of rollovers kWh or $
		// process monthly fixed charges
		/*
		if (include_fixed)
		process_monthly_charge(payment, monthly_fixed_charges, rate_esc);
		// process min charges
		if (include_min)
		{
		process_monthly_min(payment, monthly_minimum_charges, rate_esc);
		process_annual_min(payment, monthly_minimum_charges, rate_esc);
		}
		*/
		// compute revenue ( = income - payment ) and monthly bill ( = payment - income) and apply fixed and minimum charges
		c = 0;
		ssc_number_t mon_bill = 0, ann_bill = 0;
		ssc_number_t ann_min_charge = as_number("ur_annual_min_charge")*rate_esc;
		ssc_number_t mon_min_charge = as_number("ur_monthly_min_charge")*rate_esc;
		ssc_number_t mon_fixed = as_number("ur_monthly_fixed_charge")*rate_esc;

		// process one month at a time
		for (m = 0; m < 12; m++)
		{
			for (d = 0; d < util::nday[m]; d++)
			{
				for (h = 0; h < 24; h++)
				{
					if (d == util::nday[m] - 1 && h == 23)
					{
						// apply fixed first
						if (include_fixed)
						{
							payment[c] += mon_fixed;
							monthly_fixed_charges[m] += mon_fixed;
						}
						mon_bill = monthly_bill[m] + monthly_fixed_charges[m];
						if (mon_bill < 0) mon_bill = 0; // for calculating min charge with monthly surplus
						// apply monthly minimum
						if (include_min)
						{
							if (mon_bill < mon_min_charge)
							{
								monthly_minimum_charges[m] += mon_min_charge - mon_bill;
								payment[c] += mon_min_charge - mon_bill;
							}
						}
						ann_bill += mon_bill;
						if (m == 11)
						{
							// apply annual minimum
							if (include_min)
							{
								if (ann_bill < ann_min_charge)
								{
									monthly_minimum_charges[m] += ann_min_charge - ann_bill;
									payment[c] += ann_min_charge - ann_bill;
								}
							}
						}
						monthly_bill[m] += monthly_fixed_charges[m] + monthly_minimum_charges[m];
					}
					revenue[c] = income[c] - payment[c];
					c++;
				}
			}
		}





	}


	void ur_update_ec_monthly(int month, util::matrix_t<float>& charge, util::matrix_t<float>& energy)
		throw(general_error)
	{
		if (month < 0 || month > (int)m_month.size())
		{
			std::ostringstream ss;
			ss << "ur_update_ec_monthly month not found " << month;
			throw exec_error("utilityrate3", ss.str());
		}
		charge.resize_fill(m_month[month].ec_charge.nrows() + 2, m_month[month].ec_charge.ncols() + 2, 0);
		energy.resize_fill(m_month[month].ec_charge.nrows() + 2, m_month[month].ec_charge.ncols() + 2, 0);
		// output with tier column headings and period row labels 
		// and totals for rows and columns.
		int ndx = -1;
		int period = 0;
		if ((m_month[month].ec_periods.size() > 0) && (m_ec_periods.size() > 0))
		{
			// note that all monthly periods have same tiers (checked in setup)
			period = m_month[month].ec_periods[0];
			std::vector<int>::iterator result = std::find(std::begin(m_ec_periods), std::end(m_ec_periods), period);
			if (result == std::end(m_ec_periods))
			{
				std::ostringstream ss;
				ss << "energy charge period not found " << period;
				throw exec_error("utilityrate3", ss.str());
			}
			ndx = (int)(result - m_ec_periods.begin());
		}
		if (ndx > -1)
		{
			for (int ic = 0; ic < (int)m_month[month].ec_charge.ncols(); ic++)
			{
				charge.at(0, ic + 1) = (float)m_ec_periods_tiers[ndx][ic];
				energy.at(0, ic + 1) = (float)m_ec_periods_tiers[ndx][ic];
			}
			for (int ir = 0; ir < (int)m_month[month].ec_charge.nrows(); ir++)
			{
				charge.at(ir + 1, 0) = (float)m_month[month].ec_periods[ir];
				energy.at(ir + 1, 0) = (float)m_month[month].ec_periods[ir];
			}
			float c_total = 0;
			float e_total = 0;
			for (int ir = 0; ir < (int)m_month[month].ec_charge.nrows(); ir++)
			{
				float c_row_total = 0;
				float e_row_total = 0;
				for (int ic = 0; ic <(int)m_month[month].ec_charge.ncols(); ic++)
				{
					charge.at(ir + 1, ic + 1) = m_month[month].ec_charge.at(ir, ic);
					c_row_total += m_month[month].ec_charge.at(ir, ic);
					energy.at(ir + 1, ic + 1) = m_month[month].ec_energy_use.at(ir, ic);
					e_row_total += m_month[month].ec_energy_use.at(ir, ic);
				}
				charge.at(ir + 1, m_month[month].ec_charge.ncols() + 1) = c_row_total;
				energy.at(ir + 1, m_month[month].ec_charge.ncols() + 1) = e_row_total;
				c_total += c_row_total;
				e_total += e_row_total;
			}
			for (int ic = 0; ic < (int)m_month[month].ec_charge.ncols(); ic++)
			{
				float c_col_total = 0;
				float e_col_total = 0;
				for (int ir = 0; ir < (int)m_month[month].ec_charge.nrows(); ir++)
				{
					c_col_total += m_month[month].ec_charge.at(ir, ic);
					e_col_total += m_month[month].ec_energy_use.at(ir, ic);
				}
				charge.at(m_month[month].ec_charge.nrows() + 1, ic + 1) = c_col_total;
				energy.at(m_month[month].ec_energy_use.nrows() + 1, ic + 1) = e_col_total;
			}
			charge.at(m_month[month].ec_charge.nrows() + 1, m_month[month].ec_charge.ncols() + 1) = c_total;
			energy.at(m_month[month].ec_energy_use.nrows() + 1, m_month[month].ec_energy_use.ncols() + 1) = e_total;
		}

	}


};

DEFINE_MODULE_ENTRY( utilityrate3_mat, "Complex utility rate structure net revenue calculator OpenEI Version 3", 1 );


