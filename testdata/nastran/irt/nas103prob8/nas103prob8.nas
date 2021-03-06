ID NAS103 WORKSHOP 8
TIME 5
SOL 106
CEND
TITLE=MATERIAL NONLINEAR PROBLEM - LIMIT ANALYSIS
SUBTITLE=REF: WHITE AND HODGE; COMP. AND STRUCT.; 12:769-776 (1980)
 DISP=ALL
 SPCF=ALL
 STRESS=ALL
 SPC=12
 NLPARM=10
BEGING BULK
PARAM,POST,-1
$ GEOMETRY
GRDSET,,,,,,,3
GRID,1,,0.,0.
GRID,2,,0.,10.
GRID,3,,10.,10.
GRID,4,,10.,0.
GRID,5,,5.,5.
$ CONNECTIVITY
CONROD,1,1,2,10,1.
CONROD,2,3,4,10,1.
CONROD,3,2,3,11,1.
CONROD,4,2,5,11,1.
CONROD,5,3,5,11,1.
CONROD,6,1,5,11,1.
CONROD,7,4,5,11,1.
$ PROPERTIES
MAT1,10,2.+5
MAT1,11,2.+5
$ CONSTRAINTS
SPC1,12,12,1,4
$ LOADING
$ PARAMETERS
$ SOLUTION STRATEGY
NLPARM,10,20,AUTO,5,25,PW,YES,,+ A
+ A,.001,1.-7
PARAM,LGDISP,1
ENDDATA
