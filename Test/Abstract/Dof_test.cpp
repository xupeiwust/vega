/*
 * Copyright (C) Alneos, s. a r. l. (contact@alneos.fr)
 * Released under the GNU General Public License
 */

#define BOOST_TEST_MODULE Dof_test
#include <boost/test/unit_test.hpp>
#include "../../Abstract/Dof.h"

using namespace std;
using namespace vega;

BOOST_AUTO_TEST_CASE( test_dof ) {
	BOOST_CHECK_EQUAL(DOF::DX, DOF::findByPosition(0));
	BOOST_CHECK_EQUAL(DOF::RZ, DOF::findByPosition(5));
}

BOOST_AUTO_TEST_CASE( test_dofS ) {
	DOFS testTranslations = DOF::DX + DOF::DY + DOF::DZ;
	BOOST_CHECK_EQUAL(DOFS::TRANSLATIONS, testTranslations);
	DOFS testDifference = DOFS::TRANSLATIONS - DOF::DY - DOF::DZ;
	BOOST_CHECK_EQUAL(testDifference, DOFS(DOF::DX));
	BOOST_CHECK(DOFS::TRANSLATIONS.containsAll(testTranslations));
	BOOST_CHECK(!DOFS::TRANSLATIONS.containsAll(DOF::DZ + DOF::RZ));
	BOOST_CHECK_EQUAL(123, DOFS::TRANSLATIONS.nastranCode());
	BOOST_CHECK_EQUAL(456, DOFS::ROTATIONS.nastranCode());
	BOOST_CHECK_EQUAL(123456, DOFS::ALL_DOFS.nastranCode());
	BOOST_CHECK_EQUAL(0, DOFS::NO_DOFS.nastranCode());
}

BOOST_AUTO_TEST_CASE( test_DOFS ) {
    BOOST_CHECK(DOFS().empty());
    BOOST_CHECK(not DOFS(DOF::DX).empty());

	//DX
	DOFS dofs(true, false, true);

	dofs += DOF::RZ;
	int iteration_count = 0;
	bool hasDX = false;
	bool hasDRZ = false;
	//test iteration
	cout << dofs << endl;
	for (DOF dof : dofs) {
		iteration_count++;
		hasDX |= dof == DOF::DX;
		hasDRZ |= dof == DOF::RZ;
		cout << dof << endl;
	}

	BOOST_CHECK_MESSAGE(hasDX, "the dofs contains DX");
	BOOST_CHECK_MESSAGE(hasDRZ, "the dofs contains DRZ");
	BOOST_CHECK_EQUAL(3, iteration_count);

	BOOST_CHECK(!dofs.contains(DOF::RY));
	dofs += DOF::RY;
	BOOST_CHECK(dofs.contains(DOF::RY));
	BOOST_CHECK(!dofs.contains(DOF::RX));
	dofs += DOFS(DOF::RX);
	BOOST_CHECK(dofs.contains(DOF::RX));
}

BOOST_AUTO_TEST_CASE( empty_DOFS ) {
	DOFS dofs;
	BOOST_CHECK_EQUAL(dofs.begin(), dofs.end());
	for (DOF dof : dofs) {
		BOOST_FAIL("Empty dofs should exit for loop immediately");
	}
}

BOOST_AUTO_TEST_CASE( hash_DOF ) {
	BOOST_CHECK_EQUAL(hash<DOF>()(DOF(DOF::RZ)), hash<DOF>()(DOF(DOF::RZ)));
	BOOST_CHECK_NE(hash<DOF>()(DOF(DOF::RZ)), hash<DOF>()(DOF(DOF::DX)));
}

BOOST_AUTO_TEST_CASE( hash_DOFS ) {
	BOOST_CHECK_EQUAL(hash<DOFS>()(DOFS(static_cast<char>(123456))), hash<DOFS>()(DOFS(static_cast<char>(123456))));
	BOOST_CHECK_NE(hash<DOFS>()(DOFS(static_cast<char>(123))), hash<DOFS>()(DOFS(static_cast<char>(1234))));
    //BOOST_CHECK_EQUAL(hash<DOFS>()(DOFS::ALL_DOFS), hash<DOFS>()(DOFS((char) 123456)));
	BOOST_CHECK_EQUAL(hash<DOFS>()(DOFS::NO_DOFS), hash<DOFS>()(DOFS(static_cast<char>(0))));
}

BOOST_AUTO_TEST_CASE( test_differentnodes ) {
	double expected = -25000;
	DOFMatrix matrix(MatrixType::SYMMETRIC);
	matrix.addComponent(DOF::RY, DOF::DX, expected);
	double found = matrix.findComponent(DOF::RY, DOF::DX);
	BOOST_CHECK(is_equal(found, expected));
	BOOST_CHECK(!matrix.isDiagonal());
	BOOST_CHECK(matrix.isSymmetric());
	BOOST_CHECK(matrix.hasRotations());
	double found2 = matrix.findComponent(DOF::DX, DOF::RY);
	BOOST_CHECK(is_equal(found2, expected));
}

BOOST_AUTO_TEST_CASE( test_diagonal ) {
	double expected = -25000;
	DOFMatrix matrix(MatrixType::DIAGONAL);
	matrix.addComponent(DOF::RY, DOF::RY, expected);
	double found = matrix.findComponent(DOF::RY, DOF::RY);
	BOOST_CHECK(is_equal(found, expected));
	BOOST_CHECK(matrix.isDiagonal());
	BOOST_CHECK(matrix.isSymmetric());
	BOOST_CHECK(matrix.hasRotations());
}

BOOST_AUTO_TEST_CASE( test_detectsymmetric ) {
	DOFMatrix matrix(MatrixType::FULL);
	matrix.addComponent(DOF::DX, DOF::DX, 10.0);
	matrix.addComponent(DOF::DY, DOF::DY, 10.0);
	matrix.addComponent(DOF::DZ, DOF::DZ, 10.0);
	double found = matrix.findComponent(DOF::DY, DOF::DY);
	BOOST_CHECK(is_equal(found, 10.0));
	BOOST_CHECK(matrix.isDiagonal());
	BOOST_CHECK(matrix.isSymmetric());
	BOOST_CHECK(matrix.hasTranslations());
	BOOST_CHECK(!matrix.hasRotations());
}

BOOST_AUTO_TEST_CASE( dofs_intersection ) {
    DOFS a = DOF::DX + DOF::RY + DOF::RZ;
    DOFS b = DOF::DX + DOF::DY + DOF::RZ;
    DOFS expected = DOF::DX + DOF::RZ;
    DOFS found = a.intersection(b);
	BOOST_CHECK(is_equal(found, expected));
	BOOST_CHECK(is_equal(a & b, expected));
}
