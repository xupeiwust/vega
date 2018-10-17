/*
 * Copyright (C) Alneos, s. a r. l. (contact@alneos.fr)
 * This file is part of Vega.
 *
 *   Vega is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   Vega is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Vega.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Value.cpp
 *
 *  Created on: 6 mars 2014
 *      Author: siavelis
 */

#include "Value.h"
#include "Model.h"

using namespace std;

namespace vega {

Value::Value(Value::Type type) : type(type) {
}

const string NamedValue::name = "NamedValue";

const map<Value::Type, string> Value::stringByType = { { STEP_RANGE, "STEP_RANGE" }, { SPREAD_RANGE, "SPREAD_RANGE" }, {
        FUNCTION_TABLE, "FUNCTION_TABLE" }, { DYNA_PHASE, "DYNA_PHASE" }, { LIST, "LIST" }};

ostream &operator<<(ostream &out, const NamedValue& value) {
    out << to_str(value);
    return out;
}

NamedValue::NamedValue(const Model& model, Type type, int original_id, ParaName paraX, ParaName paraY) :
        Value(type), Identifiable(original_id), model(model), paraX(paraX), paraY(paraY) {
}

ValuePlaceHolder::ValuePlaceHolder(const Model& model, Type type, int original_id, ParaName paraX,
        ParaName paraY) :
        NamedValue(model, type, original_id, paraX, paraY) {
}

shared_ptr<NamedValue> ValuePlaceHolder::clone() const {
    return make_shared<ValuePlaceHolder>(*this);
}

FloatValue::FloatValue(const Model& model, double number, int original_id) :
        NamedValue(model, FloatValue::FLOAT, original_id), number(number) {
}

shared_ptr<NamedValue> FloatValue::clone() const {
    return make_shared<FloatValue>(*this);
}

bool FloatValue::iszero() const {
    return is_zero(number);
}

void FloatValue::scale(double factor) {
    number *= factor;
}

ValueRange::ValueRange(const Model& model, Type type, int original_id) :
        NamedValue(model, type, original_id) {
}

BandRange::BandRange(const Model& model, double start, int maxsearch, double end, int original_id) :
        ValueRange(model, BAND_RANGE, original_id), start(start), maxsearch(maxsearch), end(end) {
}

shared_ptr<NamedValue> BandRange::clone() const {
    return make_shared<BandRange>(*this);
}

bool BandRange::iszero() const {
    return is_equal(start, end);
}

void BandRange::scale(double factor) {
    UNUSEDV(factor);
    throw logic_error("Should not try to scale bandranges");
}

StepRange::StepRange(const Model& model, double start, double step, double end, int original_id) :
        ValueRange(model, STEP_RANGE, original_id), start(start), step(step), end(end) {
    assert(step > 0);
    count = int((end - start) / step);
}

StepRange::StepRange(const Model& model, double start, int count, double end, int original_id) :
        ValueRange(model, STEP_RANGE, original_id), start(start), count(count), end(end) {
    if (count <= 0) {
        throw logic_error("Count should be positive in steprange");
    }
    step = (end - start) / count;
}
StepRange::StepRange(const Model& model, double start, double step, int count, int original_id) :
        ValueRange(model, STEP_RANGE, original_id), start(start), step(step), count(count) {
    end = start + step * count;
}

shared_ptr<NamedValue> StepRange::clone() const {
    return make_shared<StepRange>(*this);
}

bool StepRange::iszero() const {
    return is_equal(start, end) || count == 0 || is_equal(step, 0);
}

void StepRange::scale(double factor) {
    UNUSEDV(factor);
    throw logic_error("Should not try to scale stepranges");
}

ListValue::ListValue(const Model& model, list<double> alist, int original_id) :
        NamedValue(model, LIST, original_id), alist(alist) {
}

const std::list<double> ListValue::getList() const {
  return alist;
}

shared_ptr<NamedValue> ListValue::clone() const {
    return make_shared<ListValue>(*this);
}

bool ListValue::iszero() const {
    return alist.empty();
}

void ListValue::scale(double factor) {
    std::transform(alist.begin(), alist.end(), alist.begin(), [factor](double d) -> double { return d * factor; });
}

SpreadRange::SpreadRange(const Model& model, double start, int count, double end, double spread, int original_id) :
		ValueRange(model, SPREAD_RANGE, original_id), start(start), count(count), end(end), spread(spread) {
    assert(end > start);
}

shared_ptr<NamedValue> SpreadRange::clone() const {
    return make_shared<SpreadRange>(*this);
}

bool SpreadRange::iszero() const {
    return is_equal(start, end) || count == 0;
}

void SpreadRange::scale(double factor) {
    UNUSEDV(factor);
    throw logic_error("Should not try to scale spreadranges");
}

Function::Function(const Model& model, Type type, int original_id) :
        NamedValue(model, type, original_id) {
}

FunctionTable::FunctionTable(const Model& model, Interpolation parameter, Interpolation value,
        Interpolation left, Interpolation right, int original_id) :
        Function(model, FUNCTION_TABLE, original_id), parameter(parameter), value(value), left(
                left), right(right) {
}

void FunctionTable::setXY(const double X, const double Y) {
    valuesXY.push_back(pair<double, double>(X, Y));
}

const vector<pair<double, double>>::const_iterator FunctionTable::getBeginValuesXY() const {
    return valuesXY.begin();
}

const vector<pair<double, double>>::const_iterator FunctionTable::getEndValuesXY() const {
    return valuesXY.end();
}

shared_ptr<NamedValue> FunctionTable::clone() const {
    return make_shared<FunctionTable>(*this);
}

bool FunctionTable::iszero() const {
    return std::all_of(valuesXY.begin(), valuesXY.end(), [](pair<double, double> xy) { return is_zero(xy.second); });
}

void FunctionTable::scale(double factor) {
    for(auto& xy: valuesXY) {
            xy.second *= factor;
    }
}

ConstantValue::ConstantValue(const Model&, Type type, double value, int original_id) :
        NamedValue(model, type, original_id), value(value) {
}

DynaPhase::DynaPhase(const Model&, double value, int original_id) :
        ConstantValue(model, DYNA_PHASE, value, original_id) {
}

VectorialValue::VectorialValue(double x, double y, double z) :
		Value(Value::VECTOR), value(ublas::vector<double>(3)) {
	value[0] = x;
	value[1] = y;
	value[2] = z;
}

VectorialValue::VectorialValue(ublas::vector<double>& value) :
		Value(Value::VECTOR), value(value) {
}

VectorialValue::VectorialValue(initializer_list<double> init_list):
				Value(Value::VECTOR), value(ublas::vector<double>(3)) {
	copy_n(init_list.begin(),min(static_cast<int>(init_list.size()),3), value.begin());
}

VectorialValue::VectorialValue() :
		Value(Value::VECTOR), value(ublas::vector<double>(3)) {
}

double VectorialValue::norm() const {
	return ublas::norm_2(value);
}

bool VectorialValue::iszero() const {
	return is_zero(value[0]) && is_zero(value[1]) && is_zero(value[2]);
}

VectorialValue VectorialValue::orthonormalized(const VectorialValue& u) const {
	VectorialValue v = *this;
	// https://en.wikipedia.org/wiki/Gram%E2%80%93Schmidt_process
	VectorialValue v2 = v - (u.dot(v) / u.dot(u)) * u;
	return v2.normalized();
}

VectorialValue VectorialValue::normalized() const {
	//double norm = sqrt(pow(x(), 2) + pow(y(), 2) + pow(z(), 2));
	double norm = ublas::norm_2(value);
    return VectorialValue(x() / norm, y() / norm, z() / norm);
}

VectorialValue VectorialValue::scaled(double factor) const {
	return VectorialValue(x() * factor, y() * factor, z() * factor);
}

void vega::VectorialValue::scale(double factor) {
	value[0] *= factor;
	value[1] *= factor;
	value[2] *= factor;
}

// Dot product with another VectorialValue
double VectorialValue::dot(const VectorialValue &v) const {
	return x() * v.x() + y() * v.y() + z() * v.z();
}

VectorialValue VectorialValue::cross(const VectorialValue &v) const {
	return VectorialValue(y() * v.z() - z() * v.y(), z() * v.x() - x() * v.z(),
			x() * v.y() - y() * v.x());
}

ostream& operator<<(ostream& os, const VectorialValue& obj) {
	os << "[x:" << obj.x() << ",y:" << obj.y() << ",z:" << obj.z() << "]";
	return os;
}

VectorialValue& VectorialValue::operator=(const VectorialValue& vv) {
    value=vv.value;
	return *this;
}
const VectorialValue operator+(const VectorialValue& left, const VectorialValue& right) {
	return VectorialValue(left.x() + right.x(), left.y() + right.y(), left.z() + right.z());
}
const VectorialValue operator-(const VectorialValue& left, const VectorialValue& right) {
	return VectorialValue(left.x() - right.x(), left.y() - right.y(), left.z() - right.z());
}
const VectorialValue operator*(const double& left, const VectorialValue& right) {
	return VectorialValue(left * right.x(), left * right.y(), left * right.z());
}

const VectorialValue operator/(const VectorialValue& left, const double& right) {
	return VectorialValue(left.x() / right, left.y() / right, left.z() / right);
}


bool operator==(const VectorialValue& left, const VectorialValue& right) {
	double norm = max(left.norm(), right.norm());
	if (is_zero(norm))
		return true;
	return (left - right).norm() / norm < 1e-8;
}

bool operator!=(const VectorialValue& left, const VectorialValue& right) {
	return !(left==right);
}


const VectorialValue VectorialValue::O(0, 0, 0);
const VectorialValue VectorialValue::X(1, 0, 0);
const VectorialValue VectorialValue::Y(0, 1, 0);
const VectorialValue VectorialValue::Z(0, 0, 1);
const VectorialValue VectorialValue::XYZ[3] = { VectorialValue::X, VectorialValue::Y,
		VectorialValue::Z };

/**
 * ValueOrReference
 */
ostream& operator<<(ostream &out, const ValueOrReference& valueOrReference) {
	out << "ValueOrReference: ";
	if (valueOrReference.isReference()) {
		out << "Ref:" << valueOrReference.getReference().id;
	} else if (valueOrReference.isEmpty()) {
		out << "empty";
	} else {
		out << "value:" << valueOrReference.getValue();
	}
	return out;
}

const ValueOrReference ValueOrReference::EMPTY_VALUE(
		Reference<NamedValue>(NamedValue::STEP_RANGE, Reference<NamedValue>::NO_ID, Reference<NamedValue>::NO_ID));

ValueOrReference::ValueOrReference(const boost::variant<double, Reference<NamedValue>>& _value) :
		storage(_value) {
}

ValueOrReference::ValueOrReference() :
		storage(EMPTY_VALUE.storage) {
}

ValueOrReference::ValueOrReference(double _value) :
		storage(_value) {
}

double ValueOrReference::getValue() const {
	return boost::get<double>(storage);
}

Reference<NamedValue> ValueOrReference::getReference() const {
	return boost::get<Reference<NamedValue>>(storage);
}

bool ValueOrReference::isReference() const {
	return storage.which() == 1;
}

bool ValueOrReference::isEmpty() const {
	return (*this) == EMPTY_VALUE;
}

bool ValueOrReference::operator==(const ValueOrReference& rhs) const {
	bool result;
	if (storage.which() != rhs.storage.which()) {
		result = false;
	} else if (storage.which() == 0) {
		result = is_equal(getValue() , rhs.getValue());
	} else {
		result = (getReference() == rhs.getReference());
	}
	return result;
}

bool ValueOrReference::operator<(const ValueOrReference& rhs) const {
	bool result;
	if (storage.which() != rhs.storage.which()) {
		result = storage.which() < rhs.storage.which();
	} else if (storage.which() == 0) {
		result = getValue() < rhs.getValue();
	} else {
		result = (getReference() < rhs.getReference());
	}
	return result;
}

VectorialFunction::VectorialFunction(const Model& model, Function& fx, Function& fy, Function& fz, int original_id) :
        NamedValue(model, Value::VECTORFUNCTION, original_id), _fx(fx), _fy(fy), _fz(fz) {
}

void VectorialFunction::scale(double factor) {
    _fx.scale(factor);
    _fy.scale(factor);
    _fz.scale(factor);
}

bool VectorialFunction::iszero() const {
    return _fx.iszero() && _fy.iszero() && _fz.iszero();
}

} /* namespace vega */
