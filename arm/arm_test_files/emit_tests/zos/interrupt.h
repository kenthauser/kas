/*
 * Copyright (c) 1992 Twenty-First Designs, Inc.
 * Unpublished work. All rights reserved.  
 *
 * THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF 
 * TWENTY-FIRST DESIGNS, INC.
 *
 * This software source code contains trade secrets and other
 * proprietary information of Twenty-First Designs, Inc. All
 * disclosure or use of this source code is strictly restricted
 * by contract to that required for operation and maintenance 
 * of equipment provided by Twenty-First Designs, Inc.
 * All other disclosure or use of this source code is prohibited.
 *
 ***********************************************************
 *
 * $Id: interrupt.h,v 1.1.1.1 2000/10/18 04:29:56 kent Exp $
 *
 ***********************************************************
 *
 * interrupt.h
 *
 * Declare the interrupt vectors for a dual Z80-SIO system.
 *
 ***********************************************************
 */

VECT_0b_tx	= 0
VECT_0b_esc	= 2
VECT_0b_rx	= 4
VECT_0b_src	= 6

VECT_0a_tx	= 8
VECT_0a_esc	= 10
VECT_0a_rx	= 12
VECT_0a_src	= 14

VECT_1b_tx	= 16
VECT_1b_esc	= 18
VECT_1b_rx	= 20
VECT_1b_src	= 22

VECT_1a_tx	= 24
VECT_1a_esc	= 26
VECT_1a_rx	= 28
VECT_1a_src	= 30
