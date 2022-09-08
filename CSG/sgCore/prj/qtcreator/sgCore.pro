#-------------------------------------------------
#
# Project created by QtCreator 2011-12-19T21:36:47
#
#-------------------------------------------------

QT       -= core gui

TARGET = sgCore
TEMPLATE = lib

DEFINES += sgCore_API
DEFINES += SGCORE_WINDOWS

SOURCES += \
    ../../UndoRedo.cpp \
    ../../sgErrors.cpp \
    ../../sgCore.cpp \
    ../../Scene.cpp \
    ../../Object.cpp \
    ../../Matrix.cpp \
    ../../FileMan.cpp \
    ../../Afin.cpp \
    ../../Core/APLICAT/Delone/Delone.cpp \
    ../../Core/APLICAT/Delone/BREPTriangulator.cpp \
    ../../Core/APLICAT/DXF/PUTDXFGR.cpp \
    ../../Core/APLICAT/DXF/PUT_FACE.cpp \
    ../../Core/APLICAT/DXF/POL_PATH.cpp \
    ../../Core/APLICAT/DXF/POL_MESH.cpp \
    ../../Core/APLICAT/DXF/POL_M64.cpp \
    ../../Core/APLICAT/DXF/OCS_GCS.cpp \
    ../../Core/APLICAT/DXF/I_DXF.cpp \
    ../../Core/APLICAT/DXF/GET_GROU.cpp \
    ../../Core/APLICAT/DXF/EXP_DXF.cpp \
    ../../Core/APLICAT/DXF/DXF_UTIL.cpp \
    ../../Core/APLICAT/DXF/DXF_TEXT.cpp \
    ../../Core/APLICAT/DXF/DXF_SOL.cpp \
    ../../Core/APLICAT/DXF/DXF_POLY.cpp \
    ../../Core/APLICAT/DXF/DXF_LINE.cpp \
    ../../Core/APLICAT/DXF/DXF_KEYS.cpp \
    ../../Core/APLICAT/DXF/DXF_INS.cpp \
    ../../Core/APLICAT/DXF/DXF_FACE.cpp \
    ../../Core/APLICAT/DXF/DXF_EXTD.cpp \
    ../../Core/APLICAT/DXF/DXF_CIRC.cpp \
    ../../Core/APLICAT/DXF/DXF_ARC.cpp \
    ../../Core/APLICAT/TRIAN_NP.cpp \
    ../../Core/APLICAT/SG_LS.cpp \
    ../../Core/APLICAT/IMPORT.cpp \
    ../../Core/APLICAT/I_STL.cpp \
    ../../Core/APLICAT/I_MESH.cpp \
    ../../Core/APLICAT/I_HP.cpp \
    ../../Core/APLICAT/I_FRG.cpp \
    ../../Core/APLICAT/EXP_STL2.cpp \
    ../../Core/APLICAT/EXP_STL.cpp \
    ../../Core/APLICAT/EXP_INTF.cpp \
    ../../Core/APLICAT/EXP_INT.cpp \
    ../../Core/APLICAT/DIST_OBJ.cpp \
    ../../Core/APLICAT/CHECK_PO.cpp \
    ../../Core/APLICAT/CHECK_BR.cpp \
    ../../Core/APLICAT/CALC_I.cpp \
    ../../Core/APLICAT/A_ERR.cpp \
    ../../Core/BETAR/DIAL_SG/MP.cpp \
    ../../Core/BETAR/DLG_FILE/TODOFILE.cpp \
    ../../Core/BETAR/LEGACY/U2TINY.cpp \
    ../../Core/BETAR/LEGACY/PUT_MSG.cpp \
    ../../Core/BETAR/TOOLS.cpp \
    ../../Core/BO/CHECK_NP.cpp \
    ../../Core/BO/CHECK_ED.cpp \
    ../../Core/BO/CH_VR.cpp \
    ../../Core/BO/CH_DEL.cpp \
    ../../Core/BO/BO.cpp \
    ../../Core/BO/B_VRR.cpp \
    ../../Core/BO/B_VR_LIN.cpp \
    ../../Core/BO/B_VR_IDN.cpp \
    ../../Core/BO/B_VR.cpp \
    ../../Core/BO/B_USER.cpp \
    ../../Core/BO/B_SV.cpp \
    ../../Core/BO/B_PV.cpp \
    ../../Core/BO/B_POINT.cpp \
    ../../Core/BO/B_NP_LAB.cpp \
    ../../Core/BO/B_LNP.cpp \
    ../../Core/BO/B_LINE.cpp \
    ../../Core/BO/B_GRUP.cpp \
    ../../Core/BO/B_FA_LAB.cpp \
    ../../Core/BO/B_FA.cpp \
    ../../Core/BO/B_DIVE.cpp \
    ../../Core/BO/B_DIV_F.cpp \
    ../../Core/BO/B_DEL_SV.cpp \
    ../../Core/BO/B_DEL.cpp \
    ../../Core/BO/B_DEFC.cpp \
    ../../Core/BO/B_CUT1.cpp \
    ../../Core/BO/B_AV.cpp \
    ../../Core/BO/B_AS.cpp \
    ../../Core/BO/B_ARC.cpp \
    ../../Core/COMMANDS/C_UCS.cpp \
    ../../Core/COMMANDS/C_SKIN.cpp \
    ../../Core/COMMANDS/C_SAVE.cpp \
    ../../Core/COMMANDS/C_RECT.cpp \
    ../../Core/COMMANDS/C_QUIT.cpp \
    ../../Core/COMMANDS/C_POLY.cpp \
    ../../Core/COMMANDS/C_PIPE.cpp \
    ../../Core/COMMANDS/C_PEDIT.cpp \
    ../../Core/COMMANDS/C_PATH.cpp \
    ../../Core/COMMANDS/C_OPEN.cpp \
    ../../Core/COMMANDS/C_GINFO.cpp \
    ../../Core/COMMANDS/C_EQU_P.cpp \
    ../../Core/DBGMEM/STOPS.cpp \
    ../../Core/GEO/TR_G_INF.cpp \
    ../../Core/GEO/TEST_WRD.cpp \
    ../../Core/GEO/TEST_PE.cpp \
    ../../Core/GEO/TATOHOBJ.cpp \
    ../../Core/GEO/T_TO_ACS.cpp \
    ../../Core/GEO/T_O_PLAN.cpp \
    ../../Core/GEO/T_CONT_I.cpp \
    ../../Core/GEO/SETELGEO.cpp \
    ../../Core/GEO/SEL_FRAG.cpp \
    ../../Core/GEO/Q_OFHOBJ.cpp \
    ../../Core/GEO/PER_UTIL.cpp \
    ../../Core/GEO/NTOHOBJ.cpp \
    ../../Core/GEO/NEA_UTIL.cpp \
    ../../Core/GEO/NE_TO_VP.cpp \
    ../../Core/GEO/M_OFHOBJ.cpp \
    ../../Core/GEO/LN_CALC.cpp \
    ../../Core/GEO/INT_UTIL.cpp \
    ../../Core/GEO/INT_HOBJ.cpp \
    ../../Core/GEO/GEOTYPES.cpp \
    ../../Core/GEO/GEOSVP.cpp \
    ../../Core/GEO/C_OFHOBJ.cpp \
    ../../Core/GEO/BREA_OBJ.cpp \
    ../../Core/GEO/ARC_CALC.cpp \
    ../../Core/INITS/STEPS/BEFOMAIN.cpp \
    ../../Core/INITS/SYSATTRS.cpp \
    ../../Core/INITS/STATDATA.cpp \
    ../../Core/INITS/OBJECTS.cpp \
    ../../Core/INITS/METHODS.cpp \
    ../../Core/INITS/ENTRYS.cpp \
    ../../Core/MODEL/BODY/TOR_old.cpp \
    ../../Core/MODEL/BODY/SPHERE_old.cpp \
    ../../Core/MODEL/BODY/SPH_BAND_old.cpp \
    ../../Core/MODEL/BODY/REV_BAND_old.cpp \
    ../../Core/MODEL/BODY/PRIMITIV_old.cpp \
    ../../Core/MODEL/BODY/ELIPSOID_old.cpp \
    ../../Core/MODEL/BODY/CYL_old.cpp \
    ../../Core/MODEL/BODY/CONE_old.cpp \
    ../../Core/MODEL/BODY/BOX_old.cpp \
    ../../Core/MODEL/NP_WORK/SURF_END.cpp \
    ../../Core/MODEL/NP_WORK/NP_WRITE.cpp \
    ../../Core/MODEL/NP_WORK/NP_WORK.cpp \
    ../../Core/MODEL/NP_WORK/NP_VER.cpp \
    ../../Core/MODEL/NP_WORK/NP_RW_TM.cpp \
    ../../Core/MODEL/NP_WORK/NP_POINT.cpp \
    ../../Core/MODEL/NP_WORK/NP_PL.cpp \
    ../../Core/MODEL/NP_WORK/NP_NEW.cpp \
    ../../Core/MODEL/NP_WORK/NP_MTRI.cpp \
    ../../Core/MODEL/NP_WORK/NP_MESHV.cpp \
    ../../Core/MODEL/NP_WORK/NP_M24.cpp \
    ../../Core/MODEL/NP_WORK/NP_LOOP.cpp \
    ../../Core/MODEL/NP_WORK/NP_GAB.cpp \
    ../../Core/MODEL/NP_WORK/NP_FACE.cpp \
    ../../Core/MODEL/NP_WORK/NP_ERR.cpp \
    ../../Core/MODEL/NP_WORK/NP_DIV_F.cpp \
    ../../Core/MODEL/NP_WORK/NP_DIV_E.cpp \
    ../../Core/MODEL/NP_WORK/NP_DELET.cpp \
    ../../Core/MODEL/NP_WORK/CIN_END.cpp \
    ../../Core/MODEL/SRC/O_TYPES.cpp \
    ../../Core/MODEL/SRC/O_TRANS.cpp \
    ../../Core/MODEL/SRC/O_TEXT.cpp \
    ../../Core/MODEL/SRC/O_SCAN.cpp \
    ../../Core/MODEL/SRC/O_SAVE.cpp \
    ../../Core/MODEL/SRC/O_NPBUF.cpp \
    ../../Core/MODEL/SRC/O_NP.cpp \
    ../../Core/MODEL/SRC/O_LOAD_D.cpp \
    ../../Core/MODEL/SRC/O_LOAD.cpp \
    ../../Core/MODEL/SRC/O_LNTYPE.cpp \
    ../../Core/MODEL/SRC/O_LIMITS.cpp \
    ../../Core/MODEL/SRC/O_LCS.cpp \
    ../../Core/MODEL/SRC/O_INVERT.cpp \
    ../../Core/MODEL/SRC/O_INIT.cpp \
    ../../Core/MODEL/SRC/O_FREE.cpp \
    ../../Core/MODEL/SRC/O_FRAME.cpp \
    ../../Core/MODEL/SRC/O_ERR.cpp \
    ../../Core/MODEL/SRC/O_DIM.cpp \
    ../../Core/MODEL/SRC/O_CSG.cpp \
    ../../Core/MODEL/SRC/O_COPY.cpp \
    ../../Core/MODEL/SRC/O_BLOCK.cpp \
    ../../Core/MODEL/SRC/O_ATTRIB.cpp \
    ../../Core/MODEL/SRC/O_ATTOBJ.cpp \
    ../../Core/MODEL/SRC/O_ATTITM.cpp \
    ../../Core/MODEL/SRC/O_ARCMAT.cpp \
    ../../Core/MODEL/SRC/O_ALLOC.cpp \
    ../../Core/NURBS/SPL_UTIL.cpp \
    ../../Core/NURBS/SPL_USE.cpp \
    ../../Core/NURBS/SPL_LEN.cpp \
    ../../Core/NURBS/SPL_INTR.cpp \
    ../../Core/NURBS/SPL_FIND.cpp \
    ../../Core/NURBS/SPL_ERR.cpp \
    ../../Core/NURBS/SPL_CREA.cpp \
    ../../Core/NURBS/NRB_WORK.cpp \
    ../../Core/NURBS/NRB_MATR.cpp \
    ../../Core/NURBS/NRB_KNOT.cpp \
    ../../Core/NURBS/NRB_INTR.cpp \
    ../../Core/NURBS/NRB_GEO.cpp \
    ../../Core/NURBS/NRB_CREA.cpp \
    ../../Core/NURBS/NRB_BASE.cpp \
    ../../Core/NURBS/NRB_ARC.cpp \
    ../../Core/NURBS/MEMORY.cpp \
    ../../Core/NURBS/FLETPAUR.cpp \
    ../../Core/O_UTIL/U_VTRACE.cpp \
    ../../Core/O_UTIL/U_VLD.cpp \
    ../../Core/O_UTIL/U_VIRTBD.cpp \
    ../../Core/O_UTIL/U_RCD.cpp \
    ../../Core/O_UTIL/U_LIST.cpp \
    ../../Core/O_UTIL/U_ILIST.cpp \
    ../../Core/O_UTIL/U_ERR.cpp \
    ../../Core/O_UTIL/U_DTT.cpp \
    ../../Core/O_UTIL/U_DIMVIR.cpp \
    ../../Core/O_UTIL/U_BUFSCN.cpp \
    ../../Core/O_UTIL/U_BUFFER.cpp \
    ../../Core/O_UTIL/U_AVALUE.cpp \
    ../../Core/S_DESIGN/SURFACE/SURFACE.cpp \
    ../../Core/S_DESIGN/SURFACE/SURF_UTI.cpp \
    ../../Core/S_DESIGN/SURFACE/SURF_USE.cpp \
    ../../Core/S_DESIGN/SURFACE/SURF_PRM.cpp \
    ../../Core/S_DESIGN/SURFACE/SURF_FIN.cpp \
    ../../Core/S_DESIGN/SURFACE/SURF_CRE.cpp \
    ../../Core/S_DESIGN/SURFACE/SURF_BAS.cpp \
    ../../Core/S_DESIGN/SKINNING.cpp \
    ../../Core/S_DESIGN/SKIN_BRP.cpp \
    ../../Core/S_DESIGN/SEW.cpp \
    ../../Core/S_DESIGN/SCREW.cpp \
    ../../Core/S_DESIGN/RULED.cpp \
    ../../Core/S_DESIGN/REV.cpp \
    ../../Core/S_DESIGN/PIPE_R.cpp \
    ../../Core/S_DESIGN/PIPE.cpp \
    ../../Core/S_DESIGN/MESH_SUR.cpp \
    ../../Core/S_DESIGN/FACED.cpp \
    ../../Core/S_DESIGN/EXTRUD.cpp \
    ../../Core/S_DESIGN/EQUID.cpp \
    ../../Core/S_DESIGN/DIV_BOTT.cpp \
    ../../Core/S_DESIGN/COONS.cpp \
    ../../Core/S_DESIGN/CIN_ERR.cpp \
    ../../Core/S_DESIGN/CASTED.cpp \
    ../../Core/S_DESIGN/BOTTOMS.cpp \
    ../../Core/S_DESIGN/APR_SMOT.cpp \
    ../../Core/S_DESIGN/APR_RIB.cpp \
    ../../Core/S_DESIGN/APR_PATH.cpp \
    ../../Core/SG_LIB/Z_STRING.cpp \
    ../../Core/SG_LIB/Z_MATH.cpp \
    ../../Core/SG_LIB/O_AFIN1.cpp \
    ../../Core/SG_LIB/K_MATH.cpp \
    ../../Core/ZMDI/OBJREGS.cpp \
    ../../Core/ZMDI/MODFLAG.cpp \
    ../../ExtendedClasses/Torus.cpp \
    ../../ExtendedClasses/Text.cpp \
    ../../ExtendedClasses/Surfaces.cpp \
    ../../ExtendedClasses/Spline.cpp \
    ../../ExtendedClasses/SphericBand.cpp \
    ../../ExtendedClasses/Sphere.cpp \
    ../../ExtendedClasses/Point.cpp \
    ../../ExtendedClasses/Line.cpp \
    ../../ExtendedClasses/KinematicOperations.cpp \
    ../../ExtendedClasses/GroupAndContour.cpp \
    ../../ExtendedClasses/Font.cpp \
    ../../ExtendedClasses/Ellipsoid.cpp \
    ../../ExtendedClasses/Dimensions.cpp \
    ../../ExtendedClasses/Cylinder.cpp \
    ../../ExtendedClasses/Cone.cpp \
    ../../ExtendedClasses/Circle.cpp \
    ../../ExtendedClasses/Box.cpp \
    ../../ExtendedClasses/BooleanOperations.cpp \
    ../../ExtendedClasses/Arc.cpp \
    ../../ExtendedClasses/3DObject.cpp \
    ../../ExtendedClasses/2DObject.cpp \
    ../../Core/LINE_ARC/LAST_DIR.cpp \
    ../../Core/LINE_ARC/DEFCNDPL.cpp \
    ../../Core/LINE_ARC/CONDTSTR.cpp \
    ../../Core/LINE_ARC/ARC_COND.cpp \
    ../../Core/Z_SVP/ZCUTS.cpp \
    ../../Core/Z_SVP/ZC_EQUID.cpp \
    ../../Core/Z_SVP/TR_SEL.cpp \
    ../../Core/Z_SVP/test_correct_paths_for_face.cpp \
    ../../Core/Z_SVP/T_ORIENT.cpp \
    ../../Core/Z_SVP/T_INSIDE.cpp \
    ../../Core/Z_SVP/STR_UTIL.cpp \
    ../../Core/Z_SVP/SELF_CRO.cpp \
    ../../Core/Z_SVP/SEL_L_IT.cpp \
    ../../Core/Z_SVP/S_FLAT.cpp \
    ../../Core/Z_SVP/S_CONTAC.cpp \
    ../../Core/Z_SVP/PNT_TR.cpp \
    ../../Core/Z_SVP/OFF_SEL.cpp \
    ../../Core/Z_SVP/OBJT_STR.cpp \
    ../../Core/Z_SVP/GROUPOPR.cpp \
    ../../Core/Z_SVP/GLOBAL.cpp \
    ../../Core/Z_SVP/GET_OLIM.cpp \
    ../../Core/Z_SVP/GEO_UTIL.cpp \
    ../../Core/Z_SVP/DEL_OBJ.cpp \
    ../../Core/Z_SVP/D_TO_TXT.cpp \
    ../../Core/Z_SVP/CURR_PNT.cpp \
    ../../Core/Z_SVP/CS_UTIL.cpp \
    ../../Core/Z_SVP/CREATOBJ.cpp \
    ../../Core/Z_SVP/CRE_BAK.cpp \
    ../../Core/Z_SVP/ARC_UTIL.cpp \
    ../../Core/Z_SVP/A_REGEN.cpp

HEADERS += \
    ../../UndoRedo.h \
    ../../sgTD.h \
    ../../sgSpaceMath.h \
    ../../sgScene.h \
    ../../sgObject.h \
    ../../sgMatrix.h \
    ../../sgIApp.h \
    ../../sgGroup.h \
    ../../sgFileManager.h \
    ../../sgErrors.h \
    ../../sgDefs.h \
    ../../sgCore.h \
    ../../sgAlgs.h \
    ../../sg3D.h \
    ../../sg2D.h \
    ../../resource.h \
    ../../Core/txt_utils_hdr.h \
    ../../Core/spln_utils_hdr.h \
    ../../Core/space_math_hdr.h \
    ../../Core/smthnk_utils_hdr.h \
    ../../Core/sg.h \
    ../../Core/obj_utils_hdr.h \
    ../../Core/impexp_utils_hdr.h \
    ../../Core/DraftData.h \
    ../../Core/common_hdr.h \
    ../../Core/boolean_utils_hdr.h \
    ../../Core/attrib_utils_hdr.h \
    ../../Core/arr_utils_hdr.h \
    ../../Core/APLICAT/Delone/Delone.h \
    ../../Core/APLICAT/Delone/BREPTriangulator.h \
    ../../Core/txt_utils_hdr.h \
    ../../Core/spln_utils_hdr.h \
    ../../Core/space_math_hdr.h \
    ../../Core/smthnk_utils_hdr.h \
    ../../Core/sg.h \
    ../../Core/obj_utils_hdr.h \
    ../../Core/impexp_utils_hdr.h \
    ../../Core/DraftData.h \
    ../../Core/common_hdr.h \
    ../../Core/boolean_utils_hdr.h \
    ../../Core/attrib_utils_hdr.h \
    ../../Core/arr_utils_hdr.h

win32:CONFIG *= dll

symbian {
    MMP_RULES += EXPORTUNFROZEN
    TARGET.UID3 = 0xE7D79DEE
    TARGET.CAPABILITY = 
    TARGET.EPOCALLOWDLLDATA = 1
    addFiles.sources = sgCore.dll
    addFiles.path = !:/sys/bin
    DEPLOYMENT += addFiles
}

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}

OTHER_FILES +=
