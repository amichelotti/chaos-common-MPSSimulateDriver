/******************************************************************************/
/*                                                                            */
/*       --- CAEN Engineering Srl - Computing Division ---                    */
/*                                                                            */
/*   SY1527 Software Project                                                  */
/*                                                                            */
/*   DEFCOMANDI.C                                                             */
/*   This file contains some globals variables								  */
/*                                                                            */
/*   Created: January 2000                                                    */
/*                                                                            */
/*   Auth: E. Zanetti, A. Morachioli                                          */
/*                                                                            */
/*   Release: 1.0L                                                            */
/*                                                                            */
/*   Release 1.01L: introdotto S_HVCLOCKCFG, modificata define MAX_BOARD_DESC */
/*   Release 2.00L: introdotta O_EXECNET									  */
/*                                                                            */
/******************************************************************************/
#include <string.h>
#ifdef UNIX
#include <unistd.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include "libclient.h"

/******************************************************* // !!! CAEN
//#define _CRTDBG_MAP_ALLOC
//#include <stdlib.h>
//#include <crtdbg.h>
// ******************************************************* !!! CAEN */

PROPER ParamProp[] = {
        { "Type", 0 },
		{ "Mode", 1},      /* .Dir of structure PARAM_TYPE */
		{ "Sign", 2},
		{ "Minval", 3},
		{ "Maxval", 4},
		{ "Unit", 5},
		{ "Exp", 6},
		{ "Onstate", 7},  /* .On of structure PARAM_TYPE */
		{ "Offstate", 8}, /* .Off of structure PARAM_TYPE */
		{ "NULL", 9}
};

COMANDO CommandList[] = {
        { L_LOGIN, 1, 2, 3, 1},
        { L_LOGOUT, 2, 0, 4, 0},
        { O_ENAMSG, 2, 0, 4, 0},
        { O_DISAMSG, 2, 0, 4, 0},
        { O_ADDCHGRP, 2, 3, 4, 0},
        { O_REMCHGRP, 2, 3, 4, 0},
        { G_CHNAME, 2, 3, 4, 1},
        { S_CHNAME, 2, 4, 4, 0},
        { G_CHPARAM, 2, 4, 4, 1},
        { S_CHPARAM, 2, 5, 4, 0},
        { G_GRPCOMP, 2, 1, 4, 2},
        { G_GRPPARAM, 2, 3, 4, 1},
        { S_GRPPARAM , 2, 2, 4, 0},
        { G_SYSCOMP, 2, 0, 4, 3},
        { O_TESTBDPRES, 2, 1, 4, 4},
        { O_GETCRATEMAP, 2, 0, 4, 4},
        { G_SYSINFO, 2, 1, 4, 9},
        { G_BDPARAM, 2, 3, 4, 1},
        { S_BDPARAM, 2, 3, 4, 0},
        { O_G_ACTLOG, 2, 0, 4, 5},
        { O_G_SYSNAME, 2, 0, 4, 1},
        { O_S_SYSNAME, 2, 1, 4, 0},
        { O_G_SWREL, 2, 0, 4, 1},
        { O_KILL_SURE, 2, 0, 4, 0},
        { O_KILL_OK, 2, 0, 4, 0},
        { O_CLRALARM_SURE, 2, 0, 4, 0},
        { O_CLRALARM_OK, 2, 0, 4, 0},
        { G_GENCONF, 2, 0, 4, 1},
        { S_GENCONF, 2, 2, 4, 0},
        { O_FORMAT, 2, 0, 4, 0},
        { G_RESETCONF, 2, 0, 4, 1},
        { S_RESETCONF, 2, 2, 4, 0},
        { G_RESETFLAG, 2, 0, 4, 1},
        { G_IPADDR, 2, 0, 4, 1},
        { S_IPADDR, 2, 1, 4, 0},
        { G_IPMASK, 2, 0, 4, 1},
        { S_IPMASK, 2, 1, 4, 0},
        { G_IPGTW, 2, 0, 4, 1},
        { S_IPGTW , 2, 0, 4, 0},
        { G_RSPARAM, 2, 0, 4, 1},
        { S_RSPARAM, 2, 1, 4, 0},
        { G_CRATENUM, 2, 0, 4, 1},
        { S_CRATENUM, 2, 1, 4, 0},
        { G_HVPS, 2, 0, 4, 1},
        { G_FANSTAT, 2, 0, 4, 1},
        { G_CLOCKFREQ, 2, 0, 4, 1},
        { G_HVCLOCKCFG, 2, 0, 4, 1},
        { S_HVCLOCKCFG, 2, 1, 4, 0},   /* Ver. 1.01L */
        { O_GFPIN, 2, 0, 4, 1},
        { O_GFPOUT, 2, 0, 4, 1},
        { L_RSCMDOFF, 2, 0, 4, 0},
        { O_EXECNET, 2, 4, 4, 1},      /* Ver. 2.00L */
        { O_SUBSCR, 2, 3, 4, 2},       /* Ver. 3.00 */
        { O_UNSUBSCR, 2, 3, 4, 2},     /* Ver. 3.00 */
        { LIB_NULLCMD, 0, 0, 0, 0}
};

/* Get, Set property and Exec command */
GETSETEXE GSEList[] = {
        {"Sessions", LIBGET, "", O_G_ACTLOG, "", 1, 0},
        {"Kill", LIBEXEC, "", "", O_KILL,  1, 1},
        {"ClearAlarm", LIBEXEC, "", "", O_CLRALARM, 1, 2},
        {"EnMsg", LIBEXEC, "", "", O_ENAMSG, 0, 3},
        {"DisMsg", LIBEXEC, "", "", O_DISAMSG, 0, 4},
        {"Format", LIBEXEC, "", "", O_FORMAT, 0, 5},
        {"RS232CmdOff", LIBEXEC, "", "", L_RSCMDOFF, 0, 6},
        {"ModelName", LIBGET, "", O_G_SYSNAME, "", 1, 7},
        {"SwRelease", LIBGET, "", O_G_SWREL, "", 1, 8},
        {"GenSignCfg", LIBGETSET, S_GENCONF, G_GENCONF, "", 1, 9},
        {"FrontPanIn", LIBGET, "", O_GFPIN, "", 1, 10},
        {"FrontPanOut", LIBGET, "", O_GFPOUT, "", 1, 11},
        {"ResFlagCfg", LIBGETSET, S_RESETCONF, G_RESETCONF, "", 1, 12},
        {"ResFlag", LIBGET, "", G_RESETFLAG, "", 0, 13},
        {"HvPwSM", LIBGET, "", G_HVPS, "", 1, 14},
        {"FanStat", LIBGET, "", G_FANSTAT, "", 1, 15},
        {"ClkFreq", LIBGET, "", G_CLOCKFREQ, "", 1, 16},
/* Ver. 1.01L in realta' e' anche di set {"HVClkConf", LIBGET, "", G_HVCLOCKCFG, "", 1, 17}, */
        {"HVClkConf", LIBGETSET, S_HVCLOCKCFG, G_HVCLOCKCFG, "", 1, 17},
        {"IPAddr", LIBGETSET, S_IPADDR, G_IPADDR, "", 1, 18},
        {"IPNetMsk", LIBGETSET, S_IPMASK, G_IPMASK, "", 1, 19},
        {"IPGw", LIBGETSET, S_IPGTW, G_IPGTW, "", 1, 20},
        {"RS232Par", LIBGETSET, S_RSPARAM, G_RSPARAM, "", 1, 21},
        {"CnetCrNum", LIBGETSET, S_CRATENUM, G_CRATENUM, "", 1, 22},
        {"SymbolicName", LIBGETSET, O_S_SYSNAME, O_G_SYSNAME, "", 1, 23},
        {"Nothing", -1, "", "", "", 0, -1}
};
