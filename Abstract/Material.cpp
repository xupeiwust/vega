/*
 * Copyright (C) Alneos, s. a r. l. (contact@alneos.fr)
 * Released under the GNU General Public License
 *
 * Material.cpp
 *
 *  Created on: Sep 7, 2013
 *      Author: devel
 */
#include "Material.h"
#include "Model.h"
#include <float.h>
#include <stdexcept>      // invalid_argument
#include <iostream>

using namespace std;

namespace vega {


const string Material::name = "Material";
const map<Material::Type, string> Material::stringByType = {
        { Material::Type::MATERIAL, "MATERIAL" }
};


ostream &operator<<(ostream &out, const Material& material) {
	out << to_str(material);
	if (not material.nature_by_type.empty()) {
		cout << " with:";
		for (const auto& nature : material.nature_by_type) {
		    out<< " "<< *nature.second;
		}
	}
	return out;
}

Material::Material(Model& model, int original_id) :
		Identifiable(original_id), model(model) {
}

bool Material::validate() const {
	bool validMaterial = not nature_by_type.empty();
	if (not validMaterial) {
		cerr << *this << " has no nature assigned.";
	}
	return validMaterial;
}

void Material::addNature(const shared_ptr<Nature>& nature) {
	if (findNature(nature->type)) {
		throw invalid_argument("Nature already in use " + to_str(*nature));
	}
	nature_by_type[nature->type] = nature;
}

shared_ptr<Nature> Material::findNature(const Nature::NatureType natureType) const {
	shared_ptr<Nature> nature;
	auto it = nature_by_type.find(natureType);
	if (it != nature_by_type.end())
		nature = it->second;
	return nature;
}

Nature::Nature(const Model& model, Nature::NatureType type) :
		model(model), type(type) {

}

const double Nature::UNAVAILABLE_DOUBLE = -DBL_MAX;

const string Nature::name = "NATURE";
const map<Nature::NatureType, string> Nature::stringByType = {
        {Nature::NatureType::NATURE_ELASTIC , "NATURE ELASTIC"},
        {Nature::NatureType::NATURE_VISCOELASTIC , "NATURE VISCOELASTIC"},
        {Nature::NatureType::NATURE_BILINEAR_ELASTIC, "NATURE BILINEAR ELASTIC"},
        {Nature::NatureType::NATURE_NONLINEAR_ELASTIC, "NATURE NONLINEAR ELASTIC"},
        {Nature::NatureType::NATURE_RIGID, "NATURE RIGID"},
        {Nature::NatureType::NATURE_ORTHOTROPIC, "NATURE ORTHOTROPIC"},
        {Nature::NatureType::NATURE_HYPERELASTIC, "NATURE HYPERELASTIC"},
};

ostream &operator<<(ostream &out, const Nature& nature) {
    out << to_str(nature);
    return out;
}

std::string to_str(const Nature& nature){
    std::ostringstream oss;
    std::string type;
    auto it = Nature::stringByType.find(nature.type);
    if (it != Nature::stringByType.end())
        type = "type=" + it->second;
    else
        type = "unknown type";

    oss << Nature::name << "{" << type << "}";
    return oss.str();
}

ElasticNature::ElasticNature(const Model& model, const double e, const double nu, const double g,
		const double rho, const double alpha, const double tref, const double ge) :
		Nature(model, Nature::NatureType::NATURE_ELASTIC), e(e), nu(nu), g(g), rho(rho), alpha(alpha), tref(tref), ge(ge) {

	if (is_equal(e, UNAVAILABLE_DOUBLE) && is_equal(g, UNAVAILABLE_DOUBLE))
		throw invalid_argument("E and G may not both be blank.");
}

double ElasticNature::getE() const {
	if (is_equal(nu, UNAVAILABLE_DOUBLE) && is_equal(e, UNAVAILABLE_DOUBLE))
		// If NU and E are both blank, then both are set to 0.0.
		return 0.0;
	else
		return (is_equal(e, UNAVAILABLE_DOUBLE)) ? (2 * (1 + nu) * g) : e;
}

double ElasticNature::getNu() const {
	if ((is_equal(nu, UNAVAILABLE_DOUBLE) && is_equal(g, UNAVAILABLE_DOUBLE))
			|| (is_equal(nu, UNAVAILABLE_DOUBLE) && is_equal(e, UNAVAILABLE_DOUBLE)))
		// If NU and E, or NU and G, are both blank, then both are set to 0.0.
		return 0.0;
	else
		return is_equal(nu, UNAVAILABLE_DOUBLE) ? (e / (2 * g) - 1) : nu;
}

double ElasticNature::getG() const {
	if (is_equal(nu, UNAVAILABLE_DOUBLE) && is_equal(g, UNAVAILABLE_DOUBLE))
		// If NU and G are both blank, then both are set to 0.0.
		return 0.0;
	else
		return (is_equal(g, UNAVAILABLE_DOUBLE)) ? (e / (2 * (1 + nu))) : g;
}

double ElasticNature::getRho() const {
	double mass_multiplier = 1;
	if (model.contains(ModelParameter::MASS_OVER_FORCE_MULTIPLIER)) {
		mass_multiplier = stod(model.getParameter(ModelParameter::MASS_OVER_FORCE_MULTIPLIER));
		assert(!is_zero(mass_multiplier));
	}
	return (is_equal(rho, UNAVAILABLE_DOUBLE)) ? 0 : rho * mass_multiplier;
}

double ElasticNature::getRhoAsForceDensity() const {
	return (is_equal(rho, UNAVAILABLE_DOUBLE)) ? 0 : rho;
}

double ElasticNature::getAlpha() const {
	return (is_equal(alpha, UNAVAILABLE_DOUBLE)) ? 0 : alpha;
}

double ElasticNature::getTref() const {
	return (is_equal(tref, UNAVAILABLE_DOUBLE)) ? 0 : tref;
}

double ElasticNature::getGE() const {
    return ge;
}

OrthotropicNature::OrthotropicNature(const Model& model, const double e_longitudinal, const double e_transverse,
                                     const double nu_longitudinal_transverse, const double g_longitudinal_transverse,
                                     const double g_transverse_normal, const double g_longitudinal_normal, const double rho) :
		Nature(model, Nature::NatureType::NATURE_ORTHOTROPIC), _e_longitudinal(e_longitudinal), _e_transverse(e_transverse),
		_nu_longitudinal_transverse(nu_longitudinal_transverse),
		_g_longitudinal_transverse(g_longitudinal_transverse), _g_transverse_normal(g_transverse_normal),
		_g_longitudinal_normal(g_longitudinal_normal), rho(rho) {
}

double OrthotropicNature::getE_longitudinal() const {
	return _e_longitudinal;
}

double OrthotropicNature::getE_transverse() const {
	return _e_longitudinal;
}

double OrthotropicNature::getNu_longitudinal_transverse() const {
	return _nu_longitudinal_transverse;
}

double OrthotropicNature::getG_longitudinal_transverse() const {
	return _g_longitudinal_transverse;
}

double OrthotropicNature::getG_transverse_normal() const {
	return _g_transverse_normal;
}

double OrthotropicNature::getG_longitudinal_normal() const {
	return _g_longitudinal_normal;
}

double OrthotropicNature::getRho() const {
	double mass_multiplier = 1;
	if (model.contains(ModelParameter::MASS_OVER_FORCE_MULTIPLIER)) {
		mass_multiplier = stod(model.getParameter(ModelParameter::MASS_OVER_FORCE_MULTIPLIER));
		assert(!is_zero(mass_multiplier));
	}
	return (is_equal(rho, UNAVAILABLE_DOUBLE)) ? 0 : rho * mass_multiplier;
}

double OrthotropicNature::getRhoAsForceDensity() const {
	return (is_equal(rho, UNAVAILABLE_DOUBLE)) ? 0 : rho;
}

BilinearElasticNature::BilinearElasticNature(const Model& model, const double elastic_limit,
		const double secondary_slope) :
		Nature(model, Nature::NatureType::NATURE_BILINEAR_ELASTIC), elastic_limit(elastic_limit), secondary_slope(
				secondary_slope) {
}

BilinearElasticNature::BilinearElasticNature(const Model& model) :
		Nature(model, Nature::NatureType::NATURE_BILINEAR_ELASTIC) {
}

HyperElasticNature::HyperElasticNature(const Model& model, double c10, double c01, double c20, double k, double rho) :
		Nature(model, Nature::NatureType::NATURE_HYPERELASTIC), c10(c10), c01(c01), c20(c20), k(k), rho(rho) {
}

NonLinearElasticNature::NonLinearElasticNature(const Model& model,
		const int stress_strain_function_id) :
		Nature(model, Nature::NatureType::NATURE_NONLINEAR_ELASTIC), stress_strain_function_ref(Value::Type::FUNCTION_TABLE,
				stress_strain_function_id) {
}

NonLinearElasticNature::NonLinearElasticNature(const Model& model,
		const FunctionTable& stress_strain_function) :
		Nature(model, Nature::NatureType::NATURE_NONLINEAR_ELASTIC), stress_strain_function_ref(stress_strain_function) {
}

shared_ptr<FunctionTable> NonLinearElasticNature::getStressStrainFunction() const {
	return dynamic_pointer_cast<FunctionTable>(model.find(stress_strain_function_ref));
}

RigidNature::RigidNature(const Model& model, const double rigidity, const double lagrangian) :
        Nature(model, Nature::NatureType::NATURE_RIGID), rigidity(rigidity), lagrangian(lagrangian) {
    if (is_equal(rigidity, Globals::UNAVAILABLE_DOUBLE) && is_equal(lagrangian, Globals::UNAVAILABLE_DOUBLE))
        throw invalid_argument("Rigidity and Lagrangian may not both be blank.");
}

double RigidNature::getRigidity() const {
    return this->rigidity;
}

void RigidNature::setRigidity(double rigidity) {
    this->rigidity= rigidity;
}

double RigidNature::getLagrangian() const {
    return this->lagrangian;
}

void RigidNature::setLagrangian(double lagrangian) {
    this->lagrangian = lagrangian;
}

CellContainer Material::getAssignment() const {
	return this->model.getMaterialAssignment(this->getId());
}

void Material::assignMaterial(const CellContainer& cellsToAssign) {
	this->model.assignMaterial(this->getId(), cellsToAssign);
}

}
/* namespace vega */

