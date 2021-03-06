/*
 * Copyright (C) Alneos, s. a r. l. (contact@alneos.fr)
 * Released under the GNU General Public License
 *
 * Loading.h
 *
 *  Created on: Aug 24, 2013
 *      Author: devel
 */

#ifndef LOADING_H_
#define LOADING_H_

#include "MeshComponents.h"
#include "Object.h"
#include "BoundaryCondition.h"
#include "Dof.h"
#include "Value.h"
#include "CoordinateSystem.h"
#include "Reference.h"
#include <climits>
#include <tuple>

namespace vega {

class Model;
class LoadSet;

/**
 * Base class for all Loadings
 */
class Loading: public Identifiable<Loading>, public BoundaryCondition {
private:
	friend std::ostream &operator<<(std::ostream&, const Loading&);    //output

public:
	enum class Type {
		DYNAMIC_EXCITATION,
		FORCE_SURFACE,
		GRAVITY,
		ROTATION,
		FORCE_LINE,
		NODAL_FORCE,
		NORMAL_PRESSION_FACE,
		NORMAL_PRESSION_SHELL,
		COMBINED_LOADING,
		INITIAL_TEMPERATURE,
		IMPOSED_DISPLACEMENT
	};
protected:
	Model& model;
	Loading(Model&, const std::shared_ptr<LoadSet>, Loading::Type, const int original_id =
			NO_ORIGINAL_ID, const Reference<CoordinateSystem> csref = CoordinateSystem::GLOBAL_COORDINATE_SYSTEM);
public:
	const Type type;
public:
	static const std::string name;
	static const std::map<Type, std::string> stringByType;
	const std::shared_ptr<LoadSet> loadset;
	const Reference<CoordinateSystem> csref;
	inline bool hasCoordinateSystem() const noexcept {
		return csref
				!= CoordinateSystem::GLOBAL_COORDINATE_SYSTEM;
	}
	virtual bool isNodeLoading() const {
		return false;
	}
	virtual bool isCellLoading() const {
		return false;
	}
	virtual std::unique_ptr<Loading> clone() const = 0;
	virtual void scale(const double factor) {
		std::cerr << "loading scale(" << factor << ") used but not implemented" << std::endl;
		throw std::logic_error("loading scale() used but not implemented");
	}
	bool validate() const override;
};

/**
 * Set of loadings that are often referenced by an analysis.
 */
class LoadSet: public Identifiable<LoadSet> {
	Model& model;
	friend std::ostream &operator<<(std::ostream&, const LoadSet&);
public:
	enum class Type {
		LOAD,
		DLOAD,
		LOADSET, // Static Load Set Selection for Use in Dynamics
		EXCITEID,
		ALL
	};
	LoadSet(Model&, Type type = Type::LOAD, int original_id = NO_ORIGINAL_ID);
	LoadSet(Model&, const Reference<LoadSet>& loadSetRef);
	static constexpr int COMMON_SET_ID = 0;
	std::vector<std::pair<Reference<LoadSet>, double>> embedded_loadsets;
	const Type type;
	static const std::string name;
	static const std::map<Type, std::string> stringByType;
	std::size_t size() const;
	inline bool empty() const noexcept { return size() == 0;};
	std::set<std::shared_ptr<Loading>, ptrLess<Loading> > getLoadings() const;
	std::set<std::shared_ptr<Loading>, ptrLess<Loading> > getLoadingsByType(Loading::Type) const;
	std::string getGroupName() const noexcept;
	bool validate() const override;
	std::unique_ptr<LoadSet> clone() const;
	bool hasFunctions() const;
	//bool operator<(const LoadSet &rhs) const;
};

/**
 * Represent loading applied on nodes
 */
class NodeLoading: public Loading, public NodeContainer {
protected:
	NodeLoading(Model&, const std::shared_ptr<LoadSet> loadset, Loading::Type, const int original_id = NO_ORIGINAL_ID,
			const Reference<CoordinateSystem> csref = CoordinateSystem::GLOBAL_COORDINATE_SYSTEM);
public:
	std::set<int> nodePositions() const override final;
	SpaceDimension getLoadingDimension() const {
		return SpaceDimension::DIMENSION_0D;
	}
	bool isNodeLoading() const override final {
		return true;
	}
};

class VolumicLoading: public Loading {
protected:
	VolumicLoading(Model&, const std::shared_ptr<LoadSet> loadset, Loading::Type, const int original_id = NO_ORIGINAL_ID);
};

class Gravity: public VolumicLoading {
private:
	double scalingFactor;
	const VectorialValue gravityVector;
public:
	/**
	 * gravity vector (without its scaling factor).
	 */
	VectorialValue getGravityVector() const;
	/**
	 * acceleration vector is given by scalingFactor * gravityVector.
	 */
	VectorialValue getAccelerationVector() const;
	/**
	 * Get gravity acceleration scale (no units), useful if density material is expressed as a force density.
	 */
	double getAccelerationScale() const;
	/**
	 * acceleration vector is given by scalingFactor * gravityVector.
	 */
	Gravity(Model&, const std::shared_ptr<LoadSet> loadset, double scalingFactor, const VectorialValue& gravityVector,
			const int original_id = NO_ORIGINAL_ID);
	DOFS getDOFSForNode(int nodePosition) const override;
	std::set<int> nodePositions() const override;
	std::unique_ptr<Loading> clone() const override;
	void scale(const double factor) override;
	bool ineffective() const override;
};

class Rotation: public VolumicLoading {
protected:
	Rotation(Model& model, const std::shared_ptr<LoadSet> loadset, const int original_id = NO_ORIGINAL_ID);
public:
	/**
	 * Get rotation speed (in rad/s).
	 */
	virtual double getSpeed() const = 0;
	/**
	 * Get rotation axis.
	 */
	virtual VectorialValue getAxis() const = 0;

	/**
	 * Get rotation center.
	 */
	virtual VectorialValue getCenter() const = 0;
	DOFS getDOFSForNode(int nodePosition) const override;
	std::set<int> nodePositions() const override;
	bool ineffective() const override;
};

class RotationCenter: public Rotation {
	double speed;
	const VectorialValue axis;
	const VectorialValue center;
public:
	RotationCenter(Model& model, const std::shared_ptr<LoadSet> loadset, double speed, double center_x, double center_y,
			double center_z, double axis_x, double axis_y, double axis_z, const int original_id =
					NO_ORIGINAL_ID);
	double getSpeed() const override;
	VectorialValue getAxis() const override ;
	VectorialValue getCenter() const override;
	std::unique_ptr<Loading> clone() const override;
	void scale(const double factor) override;
};

class RotationNode: public Rotation {
	double speed;
	const VectorialValue axis;
	const int node_position;
public:
	RotationNode(Model& model, const std::shared_ptr<LoadSet> loadset, double speed, const int node_id, double axis_x, double axis_y,
			double axis_z, const int original_id = NO_ORIGINAL_ID);
	double getSpeed() const override;
	VectorialValue getAxis() const override;
	VectorialValue getCenter() const override;
	std::unique_ptr<Loading> clone() const override;
	void scale(const double factor) override;
};

class ImposedDisplacement: public NodeLoading {
	DOFCoefs displacements;
public:
	/**
	 * The value is assigned to all the dof present in DOFS.
	 */
	ImposedDisplacement(Model& model, const std::shared_ptr<LoadSet> loadset, DOFS dofs, double value, int original_id = NO_ORIGINAL_ID,
                     const Reference<CoordinateSystem> csref = CoordinateSystem::GLOBAL_COORDINATE_SYSTEM);
	double getDoubleForDOF(const DOF& dof) const;
	DOFS getDOFSForNode(int nodePosition) const override;
	bool ineffective() const override;
	std::unique_ptr<Loading> clone() const override;
	void scale(const double factor) override;
};

/**
 * Base class for all forces applied on nodes
 */
class NodalForce: public NodeLoading {
public:
	NodalForce(Model&, const std::shared_ptr<LoadSet> loadset, const VectorialValue& force, const VectorialValue& moment = {},
			const int original_id = NO_ORIGINAL_ID, const Reference<CoordinateSystem> csref =
					CoordinateSystem::GLOBAL_COORDINATE_SYSTEM);
	NodalForce(Model&, const std::shared_ptr<LoadSet> loadset, double fx, double fy = 0, double fz = 0, double mx = 0,
			double my = 0, double mz = 0, const int original_id = NO_ORIGINAL_ID,
			const Reference<CoordinateSystem> csref = CoordinateSystem::GLOBAL_COORDINATE_SYSTEM);
protected:
	VectorialValue localToGlobal(int nodePosition, const VectorialValue&) const;
	VectorialValue force;
	VectorialValue moment;
public:
	virtual VectorialValue getForceInGlobalCS(const int nodePosition) const;
	virtual VectorialValue getMomentInGlobalCS(const int nodePosition) const;
	DOFS getDOFSForNode(int nodePosition) const override;
	std::unique_ptr<Loading> clone() const override;
	void scale(const double factor) override;
	bool ineffective() const override;
};

class NodalForceTwoNodes: public NodalForce {
	const int node_position1;
	const int node_position2;
	double magnitude;
public:
	NodalForceTwoNodes(Model&, const std::shared_ptr<LoadSet> loadset, const int node1_id, const int node2_id,
			double magnitude, const int original_id = NO_ORIGINAL_ID);
	VectorialValue getForceInGlobalCS(const int) const override;
	std::unique_ptr<Loading> clone() const override;
	void scale(const double factor) override;
	bool ineffective() const override;
};

/**
 *  In this class, the vector force is defined by N1N2^N3N4.
 */
//TODO: We build three classes for Nodal Force... because we have 3 ways to define a vector. That's not good.
class NodalForceFourNodes: public NodalForce {
    const int node_position1;
    const int node_position2;
    const int node_position3;
    const int node_position4;
    double magnitude;
public:
    NodalForceFourNodes(Model&, const std::shared_ptr<LoadSet> loadset, const int node1_id, const int node2_id,
            const int node3_id, const int node4_id, double magnitude, const int original_id = NO_ORIGINAL_ID);
    VectorialValue getForceInGlobalCS(const int) const override;
    std::unique_ptr<Loading> clone() const override;
    void scale(const double factor) override;
    bool ineffective() const override;
};

/**
 * See Nastran PLOAD
 */
class StaticPressure: public NodalForce {
public:
    const int node_position1;
    const int node_position2;
    const int node_position3;
    const int node_position4 = Globals::UNAVAILABLE_INT;
    double magnitude;
    StaticPressure(Model&, const std::shared_ptr<LoadSet> loadset, const int node1_id, const int node2_id,
            const int node3_id, const int node4_id, double magnitude, const int original_id = NO_ORIGINAL_ID);
    VectorialValue getForceInGlobalCS(const int) const override;
    std::unique_ptr<Loading> clone() const override;
    void scale(const double factor) override;
    bool ineffective() const override;
};

/**
 * Represent loading applied on cells
 */
class CellLoading: public Loading, public CellContainer {
protected:
	CellLoading(Model&, const std::shared_ptr<LoadSet> loadset, Loading::Type, const int original_id = NO_ORIGINAL_ID,
			const Reference<CoordinateSystem> csref = CoordinateSystem::GLOBAL_COORDINATE_SYSTEM);
public:
	/**
	 * Return true if all cells are of the dimension passed as parameter.
	 */
	bool cellDimensionGreatherThan(SpaceDimension dimension);
	std::set<int> nodePositions() const override final;
	virtual SpaceDimension getLoadingDimension() const = 0;
    virtual std::vector<int> getApplicationFaceNodeIds() const = 0;
	bool isCellLoading() const noexcept override final {
		return true;
	}
	/** implements a function that tell if this force is applied to a
	 * geometrical element or to a Poutre.
	 */
	bool appliedToGeometry();
	void createSkin();
};

/**
 *  Responsible of being a force applied to a surface (over a Shell or over a Solid face)
 */
class ForceSurface: public CellLoading {
protected:
	VectorialValue force;
	VectorialValue moment;
public:
	ForceSurface(Model&, const std::shared_ptr<LoadSet> loadset, const VectorialValue& force, const VectorialValue& moment,
			const int original_id = NO_ORIGINAL_ID);
	VectorialValue getForce() const;
	VectorialValue getMoment() const;
	DOFS getDOFSForNode(int nodePosition) const override;
	std::unique_ptr<Loading> clone() const override;
	void scale(const double factor) override;
	bool ineffective() const override;
	SpaceDimension getLoadingDimension() const override {
		return SpaceDimension::DIMENSION_2D;
	}
	virtual std::vector<int> getApplicationFaceNodeIds() const override {
	    return {};
    }
	bool validate() const override;
};

/**
 Responsible of being a ForceSurface applied to an element face, identified by two nodes and a direction
 */
class ForceSurfaceTwoNodes: public ForceSurface {
public:
	const int nodePosition1;
	const int nodePosition2;

	ForceSurfaceTwoNodes(Model&, const std::shared_ptr<LoadSet> loadset, int nodeId1, int nodeId2, const VectorialValue& force,
			const VectorialValue& moment = {}, const int original_id = NO_ORIGINAL_ID);
	ForceSurfaceTwoNodes(Model&, const std::shared_ptr<LoadSet> loadset, int nodeId1, const VectorialValue& force,
			const VectorialValue& moment = {}, const int original_id = NO_ORIGINAL_ID);
	virtual std::vector<int> getApplicationFaceNodeIds() const override;
	std::unique_ptr<Loading> clone() const override;
};

/**
 Responsible of being a force applied to a line (i.e. a Pressure over a Bar)
 */
class ForceLine: public CellLoading {
public:

	std::shared_ptr<NamedValue> force;
	DOF dof;
    ForceLine(Model&, const std::shared_ptr<LoadSet> loadset, const std::shared_ptr<NamedValue> force, DOF component,
			const int original_id = NO_ORIGINAL_ID);

	std::unique_ptr<Loading> clone() const override;
	DOFS getDOFSForNode(const int nodePosition) const override;
	void scale(const double factor) override;
	bool ineffective() const override;
	SpaceDimension getLoadingDimension() const noexcept override {
		return SpaceDimension::DIMENSION_1D;
	}
	bool hasFunctions() const noexcept override {
	    return force->isfunction();
	}
	bool validate() const override;
	std::vector<int> getApplicationFaceNodeIds() const override { return {};};
};

/**
 A normal pression applied to a surface element
 */
class NormalPressionFace: public CellLoading {

public:
	double intensity; /**< positive in the direction of the normal (Aster convention) */
	NormalPressionFace(Model&, const std::shared_ptr<LoadSet> loadset, double intensity, const int original_id = NO_ORIGINAL_ID);
	DOFS getDOFSForNode(const int nodePosition) const override;
	bool validate() const override;
	SpaceDimension getLoadingDimension() const noexcept override {
		return SpaceDimension::DIMENSION_2D;
	}
	std::unique_ptr<Loading> clone() const override;
	void scale(const double factor) override;
	bool ineffective() const override;
	virtual std::vector<int> getApplicationFaceNodeIds() const override {
		return {};
	}
};

/**
 A normal pression applied to a shell element
 */
class NormalPressionShell: public CellLoading {
public:
	double intensity; /**< positive in the direction of the normal (Aster convention) */
	NormalPressionShell(Model&, const std::shared_ptr<LoadSet> loadset, double intensity, const int original_id = NO_ORIGINAL_ID);
	DOFS getDOFSForNode(const int nodePosition) const override;
	bool validate() const override;
	SpaceDimension getLoadingDimension() const noexcept override {
		return SpaceDimension::DIMENSION_2D;
	}
	std::unique_ptr<Loading> clone() const override;
	void scale(const double factor) override;
	bool ineffective() const override;
	virtual std::vector<int> getApplicationFaceNodeIds() const override {
		return {};
	}
};

/**
 A normal pression applied to an element face, identified by two nodes and a direction
 */
class NormalPressionFaceTwoNodes: public NormalPressionFace {

public:
	const int nodePosition1;
	const int nodePosition2;
	NormalPressionFaceTwoNodes(Model&, const std::shared_ptr<LoadSet> loadset, int nodeId1, int nodeId2, double intensity, const int original_id = NO_ORIGINAL_ID);
	NormalPressionFaceTwoNodes(Model&, const std::shared_ptr<LoadSet> loadset, int nodeId1, double intensity, const int original_id = NO_ORIGINAL_ID);
	std::vector<int> getApplicationFaceNodeIds() const override;
	std::unique_ptr<Loading> clone() const override;
};

/**
 Responsible of being a frequency-dependent dynamic excitation for use in frequency response problems { P( f) } = { A} ⋅ B( f) e^i { φ ( f ) + θ – 2πfτ }.
 */
class DynamicExcitation: public Loading {
private:
    Reference<NamedValue> dynaDelay;
    Reference<NamedValue> dynaPhase;
    Reference<NamedValue> functionTableB;
    Reference<NamedValue> functionTableP;
    Reference<LoadSet> loadSet;   /**< Excitation Loadset **/
public:
	enum class DynamicExcitationType {
		LOAD,
		DISPLACEMENT,
		VELOCITY,
		ACCELERATION
	};
    DynamicExcitation(Model&, const std::shared_ptr<LoadSet> loadset, const Reference<NamedValue> dynaDelay, const Reference<NamedValue> dynaPhase,
            const Reference<NamedValue> functionTableB, const Reference<NamedValue> functionTableP, const Reference<LoadSet>, const DynamicExcitationType excitType = DynamicExcitationType::LOAD, const int original_id =
                    NO_ORIGINAL_ID);

    const DynamicExcitationType excitType;
    std::shared_ptr<DynaPhase> getDynaDelay() const;
    std::shared_ptr<DynaPhase> getDynaPhase() const;
    std::shared_ptr<FunctionTable> getFunctionTableB() const;
    std::shared_ptr<FunctionTable> getFunctionTableP() const;
    std::shared_ptr<LoadSet> getLoadSet() const;
    std::shared_ptr<FunctionPlaceHolder> getFunctionTableBPlaceHolder() const;
    std::shared_ptr<FunctionPlaceHolder> getFunctionTablePPlaceHolder() const;
    std::set<int> nodePositions() const override;
    DOFS getDOFSForNode(const int nodePosition) const override;
    void scale(const double factor) override;
    std::unique_ptr<Loading> clone() const override;
    bool validate() const override;
    bool ineffective() const override;
};

class InitialTemperature: public NodeLoading {
protected:
	double temperature;
public:
    InitialTemperature(Model&, const std::shared_ptr<LoadSet> loadset, double temperature, const int original_id = NO_ORIGINAL_ID);
	DOFS getDOFSForNode(const int nodePosition) const override;
	std::unique_ptr<Loading> clone() const override;
	void scale(const double factor) override;
	bool ineffective() const override;
};

} /* namespace vega */

#endif /* LOADING_H_ */
