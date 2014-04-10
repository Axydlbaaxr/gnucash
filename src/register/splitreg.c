/********************************************************************\
 * This program is free software; you can redistribute it and/or    *
 * modify it under the terms of the GNU General Public License as   *
 * published by the Free Software Foundation; either version 2 of   *
 * the License, or (at your option) any later version.              *
 *                                                                  *
 * This program is distributed in the hope that it will be useful,  *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of   *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the    *
 * GNU General Public License for more details.                     *
 *                                                                  *
 * You should have received a copy of the GNU General Public License*
 * along with this program; if not, contact:                        *
 *                                                                  *
 * Free Software Foundation           Voice:  +1-617-542-5942       *
 * 59 Temple Place - Suite 330        Fax:    +1-617-542-2652       *
 * Boston, MA  02111-1307,  USA       gnu@gnu.org                   *
 *                                                                  *
\********************************************************************/

/*
 * FILE:
 * splitreg.c
 *
 * FUNCTION:
 * Implements the register object.
 * Specifies the physical layout of the register cells.
 * See the header file for additional documentation.
 *
 * hack alert -- most of the code in this file should be 
 * replaced by a guile/scheme based config file.
 *
 * HISTORY:
 * Copyright (c) 1998, 1999, 2000 Linas Vepstas
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#include "recncell.h"
#include "splitreg.h"
#include "table-allgui.h"
#include "textcell.h"
#include "messages.h"
#include "util.h"

/* This static indicates the debugging module that this .o belongs to.  */
static short module = MOD_REGISTER;

static SRStringGetter debit_getter = NULL;
static SRStringGetter credit_getter = NULL;

typedef struct _CellBuffer CellBuffer;
struct _CellBuffer
{
  char * value;
  unsigned int changed;
};

struct _SplitRegisterBuffer
{
  CellBuffer dateCell;
  CellBuffer numCell;
  CellBuffer descCell;
  CellBuffer recnCell;
  CellBuffer shrsCell;
  CellBuffer balanceCell;
  CellBuffer actionCell;
  CellBuffer xfrmCell;
  CellBuffer mxfrmCell;
  CellBuffer xtoCell;
  CellBuffer memoCell;
  CellBuffer creditCell;
  CellBuffer debitCell;
  CellBuffer priceCell;
  CellBuffer valueCell;
  CellBuffer ncreditCell;
  CellBuffer ndebitCell;
};

static SplitRegisterColors reg_colors = {
  0xffdddd, /* pale red, single cursor active */
  0xccccff, /* pale blue, single cursor passive */
  0xccccff, /* pale blue, single cursor passive 2 */

  0xffdddd, /* pale red, double cursor active */
  0xccccff, /* pale blue, double cursor passive */
  0xffffff, /* white, double cursor passive 2 */

  GNC_F,    /* double mode alternate by physical row */

  0xffdddd, /* pale red, trans cursor active */
  0xccccff, /* pale blue, trans cursor passive */

  0xffffdd, /* pale yellow, split cursor active */
  0xffffff, /* white, split cursor passive */

  0xffffff  /* white, header color */
};


#define DATE_CELL_WIDTH    11
#define NUM_CELL_WIDTH      7
#define ACTN_CELL_WIDTH     7
#define XFRM_CELL_WIDTH    14
#define MXFRM_CELL_WIDTH   14
#define XTO_CELL_WIDTH     14
#define DESC_CELL_WIDTH    29
#define MEMO_CELL_WIDTH    29
#define RECN_CELL_WIDTH     1
#define DEBT_CELL_WIDTH    12
#define CRED_CELL_WIDTH    12
#define NDEBT_CELL_WIDTH   12
#define NCRED_CELL_WIDTH   12
#define PRIC_CELL_WIDTH     9
#define VALU_CELL_WIDTH    10
#define SHRS_CELL_WIDTH    10
#define BALN_CELL_WIDTH    12


#define DATE_CELL_ALIGN    ALIGN_RIGHT
#define NUM_CELL_ALIGN     ALIGN_LEFT
#define ACTN_CELL_ALIGN    ALIGN_LEFT
#define XFRM_CELL_ALIGN    ALIGN_RIGHT
#define MXFRM_CELL_ALIGN   ALIGN_RIGHT
#define XTO_CELL_ALIGN     ALIGN_RIGHT
#define DESC_CELL_ALIGN    ALIGN_LEFT
#define MEMO_CELL_ALIGN    ALIGN_LEFT
#define RECN_CELL_ALIGN    ALIGN_CENTER
#define DEBT_CELL_ALIGN    ALIGN_RIGHT
#define CRED_CELL_ALIGN    ALIGN_RIGHT
#define NDEBT_CELL_ALIGN   ALIGN_RIGHT
#define NCRED_CELL_ALIGN   ALIGN_RIGHT
#define PRIC_CELL_ALIGN    ALIGN_RIGHT
#define VALU_CELL_ALIGN    ALIGN_RIGHT
#define SHRS_CELL_ALIGN    ALIGN_RIGHT
#define BALN_CELL_ALIGN    ALIGN_RIGHT

/* ============================================== */

#define LABEL(NAME,label)					\
{								\
   BasicCell *hcell;						\
   hcell = reg->header_label_cells[NAME##_CELL];		\
   xaccSetBasicCellValue (hcell, label);			\
}

/* ============================================== */

void
xaccSplitRegisterSetDebitStringGetter(SRStringGetter getter)
{
  debit_getter = getter;
}

void
xaccSplitRegisterSetCreditStringGetter(SRStringGetter getter)
{
  credit_getter = getter;
}

/* ============================================== */
/* configLabels merely puts strings into the label cells 
 * it does *not* copy them to the header cursor */

static void
configLabels (SplitRegister *reg)
{
  BasicCell *hc;
  int type = (reg->type) & REG_TYPE_MASK;
  char *string;

  LABEL (DATE,  DATE_STR);
  LABEL (NUM,   NUM_STR);
  LABEL (DESC,  DESC_STR);
  LABEL (RECN,  "R");
  LABEL (SHRS,  TOTAL_SHARES_STR);
  LABEL (BALN,  BALN_STR);
  LABEL (ACTN,  ACTION_STR);
  LABEL (XFRM,  ACCOUNT_STR);
  LABEL (MXFRM, TRANSFER_STR);
  LABEL (XTO,   ACCOUNT_STR);
  LABEL (MEMO,  MEMO_STR);
  LABEL (CRED,  CREDIT_STR);
  LABEL (DEBT,  DEBIT_STR);
  LABEL (PRIC,  PRICE_STR);
  LABEL (VALU,  VALUE_STR);

  if (debit_getter != NULL)
  {
    string = debit_getter(type);
    if (string != NULL)
    {
      LABEL (DEBT, string);
      free(string);
    }
  }

  if (credit_getter != NULL)
  {
    string = credit_getter(type);
    if (string != NULL)
    {
      LABEL (CRED, string);
      free(string);
    }
  }

  /* copy debit, dredit strings to ndebit, ncredit cells */
  hc = reg->header_label_cells[DEBT_CELL];
  LABEL (NDEBT,  hc->value);
  hc = reg->header_label_cells[CRED_CELL];
  LABEL (NCRED,  hc->value);
}

/* ============================================== */
/* configAction strings into the action cell */
/* hack alert -- this stuff really, really should be in a config file ... */

static void
configAction (SplitRegister *reg)
{
   int type = (reg->type) & REG_TYPE_MASK;

   /* setup strings in the action pull-down */
   switch (type) {
      case BANK_REGISTER:
      case SEARCH_LEDGER:  /* broken ! FIXME bg */
         xaccAddComboCellMenuItem ( reg->actionCell, DEPOSIT_STR);
         xaccAddComboCellMenuItem ( reg->actionCell, WITHDRAW_STR);
         xaccAddComboCellMenuItem ( reg->actionCell, CHECK_STR);
         xaccAddComboCellMenuItem ( reg->actionCell, INT_STR);
         xaccAddComboCellMenuItem ( reg->actionCell, ATM_STR);
         xaccAddComboCellMenuItem ( reg->actionCell, TELLER_STR);
         xaccAddComboCellMenuItem ( reg->actionCell, POS_STR);
         xaccAddComboCellMenuItem ( reg->actionCell, ARU_STR);
         xaccAddComboCellMenuItem ( reg->actionCell, ONLINE_STR);
         xaccAddComboCellMenuItem ( reg->actionCell, ACH_STR);
         xaccAddComboCellMenuItem ( reg->actionCell, WIRE_STR);
         xaccAddComboCellMenuItem ( reg->actionCell, CREDIT_STR);
         xaccAddComboCellMenuItem ( reg->actionCell, DIRECTDEBIT_STR);
         xaccAddComboCellMenuItem ( reg->actionCell, TRANSFER_STR);
         break;
      case CASH_REGISTER:
         xaccAddComboCellMenuItem ( reg->actionCell, BUY_STR);
         xaccAddComboCellMenuItem ( reg->actionCell, SELL_STR);
         break;
      case ASSET_REGISTER:
         xaccAddComboCellMenuItem ( reg->actionCell, BUY_STR);
         xaccAddComboCellMenuItem ( reg->actionCell, SELL_STR);
         xaccAddComboCellMenuItem ( reg->actionCell, FEE_STR);
         break;
      case CREDIT_REGISTER:
         xaccAddComboCellMenuItem ( reg->actionCell, ATM_STR);
         xaccAddComboCellMenuItem ( reg->actionCell, BUY_STR);
         xaccAddComboCellMenuItem ( reg->actionCell, CREDIT_STR);
         xaccAddComboCellMenuItem ( reg->actionCell, FEE_STR);
         xaccAddComboCellMenuItem ( reg->actionCell, INT_STR);
         xaccAddComboCellMenuItem ( reg->actionCell, ONLINE_STR);
         xaccAddComboCellMenuItem ( reg->actionCell, SELL_STR);
         break;
      case LIABILITY_REGISTER:
         xaccAddComboCellMenuItem ( reg->actionCell, BUY_STR);
         xaccAddComboCellMenuItem ( reg->actionCell, SELL_STR);
         xaccAddComboCellMenuItem ( reg->actionCell, LOAN_STR);
         xaccAddComboCellMenuItem ( reg->actionCell, INT_STR);
         xaccAddComboCellMenuItem ( reg->actionCell, PAYMENT_STR);
         break;
      case INCOME_LEDGER:
      case INCOME_REGISTER:
         xaccAddComboCellMenuItem ( reg->actionCell, BUY_STR);
         xaccAddComboCellMenuItem ( reg->actionCell, SELL_STR);
         xaccAddComboCellMenuItem ( reg->actionCell, INT_STR);
         xaccAddComboCellMenuItem ( reg->actionCell, PAYMENT_STR);
         xaccAddComboCellMenuItem ( reg->actionCell, REBATE_STR);
         break;
      case EXPENSE_REGISTER:
         xaccAddComboCellMenuItem ( reg->actionCell, BUY_STR);
         xaccAddComboCellMenuItem ( reg->actionCell, SELL_STR);
         break;
      case GENERAL_LEDGER:
      case EQUITY_REGISTER:
         xaccAddComboCellMenuItem ( reg->actionCell, BUY_STR);
         xaccAddComboCellMenuItem ( reg->actionCell, SELL_STR);
         xaccAddComboCellMenuItem ( reg->actionCell, EQUITY_STR);
         break;
      case STOCK_REGISTER:
      case PORTFOLIO_LEDGER:
      case CURRENCY_REGISTER:
         xaccAddComboCellMenuItem ( reg->actionCell, BUY_STR);
         xaccAddComboCellMenuItem ( reg->actionCell, SELL_STR);
         xaccAddComboCellMenuItem ( reg->actionCell, PRICE_STR);
         xaccAddComboCellMenuItem ( reg->actionCell, FEE_STR);
         xaccAddComboCellMenuItem ( reg->actionCell, DIV_STR);
         xaccAddComboCellMenuItem ( reg->actionCell, INT_STR);
         xaccAddComboCellMenuItem ( reg->actionCell, LTCG_STR);
         xaccAddComboCellMenuItem ( reg->actionCell, STCG_STR);
         xaccAddComboCellMenuItem ( reg->actionCell, INCOME_STR);
         xaccAddComboCellMenuItem ( reg->actionCell, DIST_STR);
         xaccAddComboCellMenuItem ( reg->actionCell, SPLIT_STR);
         break;
      default:
         xaccAddComboCellMenuItem ( reg->actionCell, BUY_STR);
         xaccAddComboCellMenuItem ( reg->actionCell, SELL_STR);
         break;
   }
}

/* ============================================== */

#define SET(NAME,col,row,handler)				\
{								\
   BasicCell *hcell;						\
   hcell = reg->header_label_cells[NAME##_CELL];		\
								\
   if ((0<=row) && (0<=col)) {					\
      curs->cells [row][col] = (handler);			\
      curs->cell_types[row][col] = NAME##_CELL;                 \
      header->widths[col] = NAME##_CELL_WIDTH;			\
      header->alignments[col] = NAME##_CELL_ALIGN;		\
      curs->widths[col] = NAME##_CELL_WIDTH;			\
      curs->alignments[col] = NAME##_CELL_ALIGN;		\
      if (hcell) {						\
         if (row < reg->num_header_rows) {			\
            header->cells[row][col] = hcell;			\
         }							\
      }								\
   }								\
}

/* BASIC & FANCY macros initialize cells in the register */

#define BASIC(NAME,CN,col,row) {			\
   SET (NAME, col, row, reg->CN##Cell);			\
}

#define FANCY(NAME,CN,col,row) {			\
   SET (NAME, col, row, &(reg->CN##Cell->cell));	\
}

/* ============================================== */

static void
configLayout (SplitRegister *reg)
{
   CellBlock *curs, *header;
   int type = (reg->type) & REG_TYPE_MASK;
   int i;

   /* define header for macros */
   header = reg->header;

   /* fill things up with null cells */
   for (i=0; i<reg->num_cols; i++) {
      header->cells[0][i] = reg->nullCell;
      reg->split_cursor->cells[0][i] = reg->nullCell;
      reg->trans_cursor->cells[0][i] = reg->nullCell;
      reg->single_cursor->cells[0][i] = reg->nullCell;
      reg->double_cursor->cells[0][i] = reg->nullCell;
      reg->double_cursor->cells[1][i] = reg->nullCell;
   }

   switch (type) {
      case BANK_REGISTER:
      case CASH_REGISTER:
      case ASSET_REGISTER:
      case CREDIT_REGISTER:
      case LIABILITY_REGISTER:
      case INCOME_REGISTER:
      case EXPENSE_REGISTER:
      case EQUITY_REGISTER:
      {
         curs = reg->double_cursor;
         FANCY (DATE,   date,     0,  0);
         FANCY (NUM,    num,      1,  0);
         FANCY (DESC,   desc,     2,  0);
         FANCY (MXFRM,  mxfrm,    3,  0);
         BASIC (RECN,   recn,     4,  0);
         FANCY (DEBT,   debit,    5,  0);
         FANCY (CRED,   credit,   6,  0);
         FANCY (BALN,   balance,  7,  0);

         FANCY (ACTN,   action,   1,  1);
         FANCY (MEMO,   memo,     2,  1);

         curs = reg->trans_cursor;
         FANCY (DATE,   date,     0,  0);
         FANCY (NUM,    num,      1,  0);
         FANCY (DESC,   desc,     2,  0);
         FANCY (XTO,    xto,      3,  0);
         BASIC (RECN,   recn,     4,  0);
         FANCY (DEBT,   debit,    5,  0);
         FANCY (CRED,   credit,   6,  0);
         FANCY (BALN,   balance,  7,  0);

         curs = reg->split_cursor;
         FANCY (ACTN,   action,   1,  0);
         FANCY (MEMO,   memo,     2,  0);
         FANCY (XFRM,   xfrm,     3,  0);
         FANCY (NDEBT,  ndebit,   5,  0);
         FANCY (NCRED,  ncredit,  6,  0);

         curs = reg->single_cursor;
         FANCY (DATE,   date,     0,  0);
         FANCY (NUM,    num,      1,  0);
         FANCY (DESC,   desc,     2,  0);
         FANCY (MXFRM,  mxfrm,    3,  0);
         BASIC (RECN,   recn,     4,  0);
         FANCY (DEBT,   debit,    5,  0);
         FANCY (CRED,   credit,   6,  0);
         FANCY (BALN,   balance,  7,  0);

         break;
      }

      /* --------------------------------------------------------- */
      case INCOME_LEDGER:
      case GENERAL_LEDGER:
      case SEARCH_LEDGER:
      {
         curs = reg->double_cursor;
         FANCY (DATE,   date,     0,  0);
         FANCY (NUM,    num,      1,  0);
         FANCY (DESC,   desc,     2,  0);
         FANCY (XTO,    xto,      3,  0);
         FANCY (MXFRM,  mxfrm,    4,  0);
         BASIC (RECN,   recn,     5,  0);
         FANCY (DEBT,   debit,    6,  0);
         FANCY (CRED,   credit,   7,  0);

         FANCY (ACTN,   action,   1,  1);
         FANCY (MEMO,   memo,     2,  1);

         curs = reg->trans_cursor;
         FANCY (DATE,   date,     0,  0);
         FANCY (NUM,    num,      1,  0);
         FANCY (DESC,   desc,     2,  0);
         FANCY (XTO,    mxfrm,    3,  0);
         FANCY (XFRM,   xto,      4,  0);
         BASIC (RECN,   recn,     5,  0);
         FANCY (DEBT,   debit,    6,  0);
         FANCY (CRED,   credit,   7,  0);

         curs = reg->split_cursor;
         FANCY (ACTN,   action,   1,  0);
         FANCY (MEMO,   memo,     2,  0);
         FANCY (XFRM,   xfrm,     4,  0);
         FANCY (NDEBT,  ndebit,   6,  0);
         FANCY (NCRED,  ncredit,  7,  0);

         curs = reg->single_cursor;
         FANCY (DATE,   date,     0,  0);
         FANCY (NUM,    num,      1,  0);
         FANCY (DESC,   desc,     2,  0);
         FANCY (XTO,    xto,      3,  0);
         FANCY (MXFRM,  mxfrm,    4,  0);
         BASIC (RECN,   recn,     5,  0);
         FANCY (DEBT,   debit,    6,  0);
         FANCY (CRED,   credit,   7,  0);

         break;
      }

      /* --------------------------------------------------------- */
      case STOCK_REGISTER:
      case CURRENCY_REGISTER:
      {
         curs = reg->double_cursor;
         FANCY (DATE,   date,     0,  0);
         FANCY (NUM,    num,      1,  0);
         FANCY (DESC,   desc,     2,  0);
         FANCY (MXFRM,  mxfrm,    3,  0);
         BASIC (RECN,   recn,     4,  0);
         FANCY (DEBT,   debit,    5,  0);
         FANCY (CRED,   credit,   6,  0);
         FANCY (PRIC,   price,    7,  0);
         FANCY (VALU,   value,    8,  0);
         FANCY (SHRS,   shrs,     9,  0);
         FANCY (BALN,   balance, 10,  0);

         FANCY (ACTN,   action,   1,  1);
         FANCY (MEMO,   memo,     2,  1);

         curs = reg->trans_cursor;
         FANCY (DATE,   date,     0,  0);
         FANCY (NUM,    num,      1,  0);
         FANCY (DESC,   desc,     2,  0);
         FANCY (XTO,    xto,      3,  0);
         BASIC (RECN,   recn,     4,  0);
         FANCY (DEBT,   debit,    5,  0);
         FANCY (CRED,   credit,   6,  0);
         FANCY (PRIC,   price,    7,  0);
         FANCY (VALU,   value,    8,  0);
         FANCY (SHRS,   shrs,     9,  0);
         FANCY (BALN,   balance,  10, 0);

         curs = reg->split_cursor;
         FANCY (ACTN,   action,   1,  0);
         FANCY (MEMO,   memo,     2,  0);
         FANCY (XFRM,   xfrm,     3,  0);
         FANCY (NDEBT,  ndebit,   5,  0);
         FANCY (NCRED,  ncredit,  6,  0);

         curs = reg->single_cursor;
         FANCY (DATE,   date,     0,  0);
         FANCY (NUM,    num,      1,  0);
         FANCY (DESC,   desc,     2,  0);
         FANCY (MXFRM,  mxfrm,    3,  0);
         BASIC (RECN,   recn,     4,  0);
         FANCY (DEBT,   debit,    5,  0);
         FANCY (CRED,   credit,   6,  0);
         FANCY (PRIC,   price,    7,  0);
         FANCY (VALU,   value,    8,  0);
         FANCY (SHRS,   shrs,     9,  0);
         FANCY (BALN,   balance, 10,  0);

         break;
      }

      /* --------------------------------------------------------- */
      case PORTFOLIO_LEDGER:
      {
         curs = reg->double_cursor;
         FANCY (DATE,   date,     0,  0);
         FANCY (NUM,    num,      1,  0);
         FANCY (DESC,   desc,     2,  0);
         FANCY (XTO,    xto,      3,  0);
         FANCY (MXFRM,  mxfrm,    4,  0);
         BASIC (RECN,   recn,     5,  0);
         FANCY (DEBT,   debit,    6,  0);
         FANCY (CRED,   credit,   7,  0);
         FANCY (PRIC,   price,    8,  0);
         FANCY (VALU,   value,    9,  0);
         FANCY (SHRS,   shrs,    10,  0);

         FANCY (ACTN,   action,   1,  1);
         FANCY (MEMO,   memo,     2,  1);

         curs = reg->trans_cursor;
         FANCY (DATE,   date,     0,  0);
         FANCY (NUM,    num,      1,  0);
         FANCY (DESC,   desc,     2,  0);
         FANCY (XTO,    mxfrm,    3,  0);
         FANCY (XFRM,   xto,      4,  0);
         BASIC (RECN,   recn,     5,  0);
         FANCY (DEBT,   debit,    6,  0);
         FANCY (CRED,   credit,   7,  0);
         FANCY (PRIC,   price,    8,  0);
         FANCY (VALU,   value,    9,  0);
         FANCY (SHRS,   shrs,    10,  0);

         curs = reg->split_cursor;
         FANCY (ACTN,   action,   1,  0);
         FANCY (MEMO,   memo,     2,  0);
         FANCY (XFRM,   xfrm,     4,  0);
         FANCY (NDEBT,  ndebit,   6,  0);
         FANCY (NCRED,  ncredit,  7,  0);

         curs = reg->single_cursor;
         FANCY (DATE,   date,     0,  0);
         FANCY (NUM,    num,      1,  0);
         FANCY (DESC,   desc,     2,  0);
         FANCY (XTO,    xto,      3,  0);
         FANCY (MXFRM,  mxfrm,    4,  0);
         BASIC (RECN,   recn,     5,  0);
         FANCY (DEBT,   debit,    6,  0);
         FANCY (CRED,   credit,   7,  0);
         FANCY (PRIC,   price,    8,  0);
         FANCY (VALU,   value,    9,  0);
         FANCY (SHRS,   shrs,    10,  0);

         break;
      }

      /* --------------------------------------------------------- */
      default:
         PERR ("unknown register type %d \n", type);
         break;
   }
}

/* ============================================== */
/* define the traversal order -- negative cells mean "traverse out of table" */

/* Right Traversals */

#define FIRST_RIGHT(r,c) {				\
   prev_r = r; prev_c = c;				\
}


#define NEXT_RIGHT(r,c) {			        \
   xaccNextRight (curs, prev_r, prev_c, (r), (c));	\
   prev_r = r; prev_c = c;				\
}


#define TRAVERSE_NON_NULL_CELLS() {			\
   i = prev_r;						\
   for (j=prev_c+1; j<curs->numCols; j++) {		\
      if ((reg->nullCell != curs->cells[i][j]) &&	\
          (reg->recnCell != curs->cells[i][j]) &&	\
          (XACC_CELL_ALLOW_INPUT & curs->cells[i][j]->input_output)) \
      {							\
         NEXT_RIGHT  (i, j);				\
      }							\
   }							\
   for (i=prev_r+1; i<curs->numRows; i++) {		\
      for (j=0; j<curs->numCols; j++) {			\
         if ((reg->nullCell != curs->cells[i][j]) &&	\
             (reg->recnCell != curs->cells[i][j]) &&	\
             (XACC_CELL_ALLOW_INPUT & curs->cells[i][j]->input_output)) \
         {						\
            NEXT_RIGHT  (i, j);				\
         }						\
      }							\
   }							\
}


#define FIRST_NON_NULL(r,c) {				\
   i = r;						\
   for (j=c; j<curs->numCols; j++) {			\
      if ((reg->nullCell != curs->cells[i][j]) &&	\
          (reg->recnCell != curs->cells[i][j]) &&	\
          (XACC_CELL_ALLOW_INPUT & curs->cells[i][j]->input_output)) \
      {							\
         FIRST_RIGHT  (i, j);				\
         break;						\
      }							\
   }							\
}


#define NEXT_NON_NULL(r,c) {				\
   i = r;						\
   for (j=c+1; j<curs->numCols; j++) {			\
      if ((reg->nullCell != curs->cells[i][j]) &&	\
          (reg->recnCell != curs->cells[i][j]) &&	\
          (XACC_CELL_ALLOW_INPUT & curs->cells[i][j]->input_output)) \
      {							\
         NEXT_RIGHT  (i, j);				\
         break;						\
      }							\
   }							\
}

#define EXIT_RIGHT() {                                  \
  curs->right_exit_r = prev_r;     curs->right_exit_c = prev_c;  \
}


#define NEXT_SPLIT() {					\
   i = 0;						\
   for (j=0; j<reg->split_cursor->numCols; j++) {	\
      if ((reg->nullCell != reg->split_cursor->cells[i][j]) &&	\
          (reg->recnCell != reg->split_cursor->cells[i][j]) &&	\
          (XACC_CELL_ALLOW_INPUT & reg->split_cursor->cells[i][j]->input_output)) \
      {							\
         NEXT_RIGHT  (i+1, j);				\
         break;						\
      }							\
   }							\
}


/* Left Traversals */

#define LAST_LEFT(r,c) {				\
   prev_r = r; prev_c = c;				\
}

#define NEXT_LEFT(r,c) {				\
   xaccNextLeft (curs, prev_r, prev_c, (r), (c));	\
   prev_r = r; prev_c = c;				\
}

#define TRAVERSE_NON_NULL_CELLS_LEFT() {		\
   i = prev_r;						\
   for (j=prev_c -1; j>=0; j--) {		        \
      if ((reg->nullCell != curs->cells[i][j]) &&	\
          (reg->recnCell != curs->cells[i][j]) &&	\
          (XACC_CELL_ALLOW_INPUT & curs->cells[i][j]->input_output)) \
      {	                                                \
         NEXT_LEFT  (i, j);				\
      }							\
   }							\
   for (i=prev_r-1; i>=0; i--) {	                \
      for (j=curs->numCols-1; j>=0; j--) {		\
         if ((reg->nullCell != curs->cells[i][j]) &&	\
             (reg->recnCell != curs->cells[i][j]) &&	\
             (XACC_CELL_ALLOW_INPUT & curs->cells[i][j]->input_output)) \
         {					        \
            NEXT_LEFT  (i, j);				\
         }						\
      }							\
   }							\
}

#define LAST_NON_NULL(r,c) {				\
   i = r;						\
   for (j=c; j>=0; j--) {			        \
      if ((reg->nullCell != curs->cells[i][j]) &&	\
          (reg->recnCell != curs->cells[i][j]) &&	\
          (XACC_CELL_ALLOW_INPUT & curs->cells[i][j]->input_output)) \
      {							\
         LAST_LEFT  (i, j);				\
         break;						\
      }							\
   }							\
}

#define PREVIOUS_NON_NULL(r,c) {			\
   i = r;						\
   for (j=c-1; j>=0; j--) {			        \
      if ((reg->nullCell != curs->cells[i][j]) &&	\
          (reg->recnCell != curs->cells[i][j]) &&	\
          (XACC_CELL_ALLOW_INPUT & curs->cells[i][j]->input_output)) \
      {							\
         NEXT_LEFT  (i, j);				\
         break;						\
      }							\
   }							\
}

#define EXIT_LEFT() {                                   \
  curs->left_exit_r = prev_r;  curs->left_exit_c = prev_c;      \
}

#define PREVIOUS_SPLIT() {					\
   i = reg->split_cursor->numRows-1;				\
   for (j=reg->split_cursor->numCols-1; j>=0; j--) {	        \
      if ((reg->nullCell != reg->split_cursor->cells[i][j]) &&	\
          (reg->recnCell != reg->split_cursor->cells[i][j]) &&	\
          (XACC_CELL_ALLOW_INPUT & reg->split_cursor->cells[i][j]->input_output)) \
      {							\
         NEXT_LEFT  (i-1, j);				\
         break;						\
      }							\
   }							\
}

static void
configTraverse (SplitRegister *reg)
{
   int i,j;
   int prev_r=0, prev_c=0;
   int first_r, first_c;
   CellBlock *curs = NULL;

   curs = reg->single_cursor;
   /* lead in with the date cell, return to the date cell */
   FIRST_NON_NULL (0, 0);
   first_r = prev_r; first_c = prev_c;
   TRAVERSE_NON_NULL_CELLS ();
   /* set the exit row, col */
   EXIT_RIGHT ();
   /* wrap back to start of row after hitting the commit button */
   NEXT_RIGHT (first_r, first_c);

   /* left traverses */
   LAST_NON_NULL (curs->numRows-1, curs->numCols - 1);
   first_r = prev_r;  first_c = prev_c;
   TRAVERSE_NON_NULL_CELLS_LEFT();
   EXIT_LEFT ();
   NEXT_LEFT (first_r, first_c);

   curs = reg->double_cursor;
   /* lead in with the date cell, return to the date cell */
   FIRST_NON_NULL (0, 0);
   first_r = prev_r; first_c = prev_c;
   TRAVERSE_NON_NULL_CELLS ();
   /* set the exit row,col */
   EXIT_RIGHT ();
   /* for double-line, hop back one row */
   NEXT_RIGHT (first_r, first_c);

   /* left traverses */
   LAST_NON_NULL (curs->numRows-1, curs->numCols - 1);
   first_r = prev_r;  first_c = prev_c;
   TRAVERSE_NON_NULL_CELLS_LEFT ();
   EXIT_LEFT ();
   NEXT_LEFT (first_r, first_c);

   curs = reg->trans_cursor;
   FIRST_NON_NULL (0,0);
   TRAVERSE_NON_NULL_CELLS ();
   /* set the exit row, col */
   EXIT_RIGHT ();
   /* hop to start of next row (the split cursor) */
   NEXT_SPLIT();

   /* left_traverses */
   LAST_NON_NULL (curs->numRows-1, curs->numCols - 1);
   TRAVERSE_NON_NULL_CELLS_LEFT ();
   EXIT_LEFT ();
   PREVIOUS_SPLIT ();

   curs = reg->split_cursor;
   FIRST_NON_NULL (0,0);
   TRAVERSE_NON_NULL_CELLS ();
   /* set the exit row, col */
   EXIT_RIGHT ();
   /* hop to start of next row (the split cursor) */
   NEXT_SPLIT();

   /* left_traverses */
   LAST_NON_NULL (curs->numRows-1, curs->numCols - 1);
   TRAVERSE_NON_NULL_CELLS_LEFT ();
   EXIT_LEFT ();
   PREVIOUS_SPLIT ();
}

/* ============================================== */

SplitRegister *
xaccMallocSplitRegister (int type)
{
   SplitRegister * reg;
   reg = (SplitRegister *) malloc (sizeof (SplitRegister));
   xaccInitSplitRegister (reg, type);
   return reg;
}

/* ============================================== */

void
xaccSetSplitRegisterColors (SplitRegisterColors reg_colors_new)
{
  reg_colors = reg_colors_new;
}

/* ============================================== */

static void
configTable(SplitRegister *reg)
{
  int style = reg->type & REG_STYLE_MASK;

  if ((reg == NULL) || (reg->table == NULL))
    return;

  switch (style) {
    case REG_SINGLE_LINE:
    case REG_SINGLE_DYNAMIC:
      reg->table->alternate_bg_colors = GNC_T;
      break;
    case REG_DOUBLE_LINE:
    case REG_DOUBLE_DYNAMIC:
      reg->table->alternate_bg_colors = reg_colors.double_alternate_virt;
      break;
    default:
      reg->table->alternate_bg_colors = GNC_F;
      break;
  }
}

/* ============================================== */

void
xaccSplitRegisterConfigColors (SplitRegister *reg)
{
   reg->single_cursor->active_bg_color =
     reg_colors.single_cursor_active_bg_color;
   reg->single_cursor->passive_bg_color =
     reg_colors.single_cursor_passive_bg_color;
   reg->single_cursor->passive_bg_color2 =
     reg_colors.single_cursor_passive_bg_color2;

   reg->double_cursor->active_bg_color =
     reg_colors.double_cursor_active_bg_color;
   reg->double_cursor->passive_bg_color =
     reg_colors.double_cursor_passive_bg_color;
   reg->double_cursor->passive_bg_color2 =
     reg_colors.double_cursor_passive_bg_color2;

   reg->trans_cursor->active_bg_color =
     reg_colors.trans_cursor_active_bg_color;
   reg->trans_cursor->passive_bg_color =
     reg_colors.trans_cursor_passive_bg_color;
   reg->trans_cursor->passive_bg_color2 =
     reg_colors.trans_cursor_passive_bg_color;

   reg->split_cursor->active_bg_color =
     reg_colors.split_cursor_active_bg_color;
   reg->split_cursor->passive_bg_color =
     reg_colors.split_cursor_passive_bg_color;
   reg->split_cursor->passive_bg_color2 =
     reg_colors.split_cursor_passive_bg_color;

   reg->header->active_bg_color = reg_colors.header_bg_color;
   reg->header->passive_bg_color = reg_colors.header_bg_color;
   reg->header->passive_bg_color2 = reg_colors.header_bg_color;

   configTable(reg);
}

/* ============================================== */

static void
configCursors (SplitRegister *reg)
{
  xaccSplitRegisterConfigColors(reg);
}

/* ============================================== */

static void
mallocCursors (SplitRegister *reg)
{
  int type = (reg->type) & REG_TYPE_MASK;

  switch (type) {
    case BANK_REGISTER:
    case CASH_REGISTER:
    case ASSET_REGISTER:
    case CREDIT_REGISTER:
    case LIABILITY_REGISTER:
    case INCOME_REGISTER:
    case EXPENSE_REGISTER:
    case EQUITY_REGISTER:
      reg->num_cols = 8;
      break;

    case INCOME_LEDGER:
    case GENERAL_LEDGER:
    case SEARCH_LEDGER:
      reg->num_cols = 8;
      break;

    case STOCK_REGISTER:
    case CURRENCY_REGISTER:
      reg->num_cols = 11;
      break;

    case PORTFOLIO_LEDGER:
      reg->num_cols = 11;
      break;

    default:
      break;
  }

  reg->num_header_rows = 1;
  reg->header = xaccMallocCellBlock (reg->num_header_rows, reg->num_cols);

  /* cursors used in the single & double line displays */
  reg->single_cursor = xaccMallocCellBlock (1, reg->num_cols);
  reg->double_cursor = xaccMallocCellBlock (2, reg->num_cols);

  /* the two cursors used for multi-line and dynamic displays */
  reg->trans_cursor = xaccMallocCellBlock (1, reg->num_cols);
  reg->split_cursor = xaccMallocCellBlock (1, reg->num_cols);
}

/* ============================================== */

#define HDR(NAME)						\
{								\
   BasicCell *hcell;						\
   hcell = xaccMallocTextCell();				\
   reg->header_label_cells[NAME##_CELL] = hcell;		\
}

#define NEW(CN,TYPE)						\
   reg->CN##Cell = xaccMalloc##TYPE##Cell();			\

void 
xaccInitSplitRegister (SplitRegister *reg, int type)
{
   Table * table;
   CellBlock *header;
   int phys_r, phys_c;

   reg->table = NULL;
   reg->user_data = NULL;
   reg->destroy = NULL;
   reg->type = type;

   /* --------------------------- */
   /* define the number of columns in the display, malloc the cursors */
   mallocCursors (reg);

   /* --------------------------- */
   /* malloc the header (label) cells */
   header = reg->header;

   HDR (DATE);
   HDR (NUM);
   HDR (ACTN);
   HDR (XFRM);
   HDR (MXFRM);
   HDR (XTO);
   HDR (DESC);
   HDR (MEMO);
   HDR (RECN);
   HDR (CRED);
   HDR (DEBT);
   HDR (PRIC);
   HDR (VALU);
   HDR (SHRS);
   HDR (BALN);

   HDR (NCRED);
   HDR (NDEBT);

   /* --------------------------- */
   /* malloc the workhorse cells */

   NEW (null,    Basic);
   NEW (date,    Date);
   NEW (num,     Num);
   NEW (desc,    QuickFill);
   NEW (recn,    Recn);
   NEW (shrs,    Price);
   NEW (balance, Price);

   NEW (xfrm,    Combo);
   NEW (mxfrm,   Combo);
   NEW (xto,     Combo);
   NEW (action,  Combo);
   NEW (memo,    QuickFill);
   NEW (credit,  Price);
   NEW (debit,   Price);
   NEW (price,   Price);
   NEW (value,   Price);

   NEW (ncredit, Price);
   NEW (ndebit,  Price);

   /* --------------------------- */
   /* configLabels merely puts strings into the label cells 
    * it does *not* copy them to the header cursor */
   configLabels (reg);

   /* config the layout of the cells in the cursors */
   configLayout (reg);

   /* --------------------------- */
   /* do some misc cell config */
   configCursors (reg);

   /* 
    * The Null Cell is used to make sure that "empty"
    * cells stay empty.  This solves the problem of 
    * having the table be reformatted, the result of
    * which is that an empty cell has landed on a cell
    * that was previously non-empty.  We want to make 
    * sure that we erase those cell contents. The null
    * cells handles this for us.
    */

   reg->nullCell->input_output = XACC_CELL_ALLOW_NONE;
   xaccSetBasicCellValue (reg->nullCell, "");

   /* The num cell is the transaction number */
   xaccSetBasicCellBlankHelp (&reg->numCell->cell, NUM_CELL_HELP);

   /* the xfer cells */
   xaccSetBasicCellBlankHelp (&reg->mxfrmCell->cell, XFER_CELL_HELP);
   xaccSetBasicCellBlankHelp (&reg->xfrmCell->cell, XFER_CELL_HELP);
   xaccSetBasicCellBlankHelp (&reg->xtoCell->cell, XFER_TO_CELL_HELP);

   xaccComboCellSetIgnoreString (reg->mxfrmCell, SPLIT_STR);
   xaccComboCellSetIgnoreString (reg->xtoCell, SPLIT_STR);

   xaccComboCellSetIgnoreHelp (reg->mxfrmCell, TOOLTIP_MULTI_SPLIT);
   xaccComboCellSetIgnoreHelp (reg->xtoCell, TOOLTIP_MULTI_SPLIT);

   /* the memo cell */
   xaccSetBasicCellBlankHelp (&reg->memoCell->cell, MEMO_CELL_HELP);

   /* the desc cell */
   xaccSetBasicCellBlankHelp (&reg->descCell->cell, DESC_CELL_HELP);

   /* The balance cell does not accept input; it's for display only.
    * however, we *do* want it to shadow the true cell contents when 
    * the cursor is repositioned.  Othewise, it will just display 
    * whatever previous bogus value it contained.
    */
   reg->balanceCell->cell.input_output = XACC_CELL_ALLOW_SHADOW;
   reg->shrsCell->cell.input_output = XACC_CELL_ALLOW_SHADOW;

   /* by default, don't blank zeros on the balance or price cells. */
   xaccSetPriceCellBlankZero(reg->balanceCell, GNC_F);
   xaccSetPriceCellBlankZero(reg->priceCell, GNC_F);

   /* The reconcile cell should only be entered with the pointer,
    * and only then when the user clicks directly on the cell.
    */
   reg->recnCell->input_output |= XACC_CELL_ALLOW_EXACT_ONLY;

   /* Initialize price cells */
   xaccSetPriceCellValue (reg->debitCell, 0.0);
   xaccSetPriceCellValue (reg->creditCell, 0.0);
   xaccSetPriceCellValue (reg->valueCell, 0.0);
   xaccSetPriceCellValue (reg->ndebitCell, 0.0);
   xaccSetPriceCellValue (reg->ncreditCell, 0.0);

   /* Initialize shares cell */
   xaccSetPriceCellSharesValue (reg->shrsCell, GNC_T);

   /* The action cell should accept strings not in the list */
   xaccComboCellSetStrict (reg->actionCell, GNC_F);
   xaccSetBasicCellBlankHelp (&reg->actionCell->cell, ACTION_CELL_HELP);

   /* number format for share quantities in stock ledgers */
   switch (type & REG_TYPE_MASK) {
      case CURRENCY_REGISTER:
        xaccSetPriceCellIsCurrency (reg->priceCell, GNC_T);

      case STOCK_REGISTER:
      case PORTFOLIO_LEDGER:
         xaccSetPriceCellSharesValue (reg->debitCell, GNC_T);
         xaccSetPriceCellSharesValue (reg->creditCell, GNC_T);
         xaccSetPriceCellSharesValue (reg->ndebitCell, GNC_T);
         xaccSetPriceCellSharesValue (reg->ncreditCell, GNC_T);
         xaccSetPriceCellIsCurrency (reg->priceCell, GNC_T);

         xaccSetBasicCellBlankHelp (&reg->priceCell->cell, PRICE_CELL_HELP);
         xaccSetBasicCellBlankHelp (&reg->valueCell->cell, VALUE_CELL_HELP);
         break;
      default:
         break;
   }

   /* -------------------------------- */   
   /* define how traversal works. This must be done *after* the balance, etc.
    * cells have been marked read-only, since otherwise config will try
    * to pick them up.
    */
   configTraverse (reg);

   /* add menu items for the action cell */
   configAction (reg);

   /* -------------------------------- */   
   phys_r = header->numRows;
   reg->cursor_phys_row = phys_r;  /* cursor on first line past header */
   reg->cursor_virt_row = 1;

   phys_r += reg->single_cursor->numRows;
   reg->num_phys_rows = phys_r;
   reg->num_virt_rows = 2;  /* one header, one single_cursor */

   phys_c = header->numCols;
   reg->num_cols = phys_c;

   table = xaccMallocTable ();
   xaccSetTableSize (table, phys_r, phys_c, reg->num_virt_rows, 1);
   xaccSetCursor (table, header, 0, 0, 0, 0);

   /* the SetCursor call below is for most practical purposes useless.
    * It simply installs a cursor (the single-line cursor, but it could 
    * of been any of them), and moves it to the first editable row.
    * Whoop-de-doo, since this is promptly over-ridden when real data 
    * gets loaded.  Its just sort of here as a fail-safe fallback,
    * in case someone just creates a register but doesn't do anything 
    * with it.  Don't want to freak out any programmers.
    */
   xaccSetCursor (table, reg->single_cursor, 
                         reg->cursor_phys_row, 0, 
                         reg->cursor_virt_row, 0);
   xaccMoveCursor (table, header->numRows, 0);

   reg->table = table;

   configTable(reg);
}

/* ============================================== */

void
xaccConfigSplitRegister (SplitRegister *reg, int newtype)
{
   if (!reg) return;

   reg->type = newtype;

   /* Make sure that any GUI elements associated with this reconfig 
    * are properly initialized. */
   xaccCreateCursor (reg->table, reg->single_cursor);
   xaccCreateCursor (reg->table, reg->double_cursor);
   xaccCreateCursor (reg->table, reg->trans_cursor);
   xaccCreateCursor (reg->table, reg->split_cursor);

   configTable(reg);
}

/* ============================================== */

void 
xaccDestroySplitRegister (SplitRegister *reg)
{
   /* give the user a chance to clean up */
   if (reg->destroy) {
      (*(reg->destroy)) (reg);
   }
   reg->destroy = NULL;
   reg->user_data = NULL;

   xaccDestroyTable (reg->table);
   reg->table = NULL;

   xaccDestroyCellBlock (reg->header);
   xaccDestroyCellBlock (reg->single_cursor);
   xaccDestroyCellBlock (reg->double_cursor);
   xaccDestroyCellBlock (reg->trans_cursor);
   xaccDestroyCellBlock (reg->split_cursor);
   reg->header = NULL;
   reg->single_cursor = NULL;
   reg->double_cursor = NULL;
   reg->trans_cursor = NULL;
   reg->split_cursor = NULL;

   xaccDestroyDateCell      (reg->dateCell);
   xaccDestroyNumCell       (reg->numCell);
   xaccDestroyQuickFillCell (reg->descCell);
   xaccDestroyBasicCell     (reg->recnCell);
   xaccDestroyPriceCell     (reg->shrsCell);
   xaccDestroyPriceCell     (reg->balanceCell);

   xaccDestroyComboCell     (reg->actionCell);
   xaccDestroyComboCell     (reg->xfrmCell);
   xaccDestroyComboCell     (reg->mxfrmCell);
   xaccDestroyComboCell     (reg->xtoCell);
   xaccDestroyQuickFillCell (reg->memoCell);
   xaccDestroyPriceCell     (reg->creditCell);
   xaccDestroyPriceCell     (reg->debitCell);
   xaccDestroyPriceCell     (reg->priceCell);
   xaccDestroyPriceCell     (reg->valueCell);

   xaccDestroyPriceCell     (reg->ncreditCell);
   xaccDestroyPriceCell     (reg->ndebitCell);

   reg->dateCell    = NULL;
   reg->numCell     = NULL;
   reg->descCell    = NULL;
   reg->recnCell    = NULL;
   reg->shrsCell    = NULL;
   reg->balanceCell = NULL;

   reg->actionCell  = NULL;
   reg->xfrmCell    = NULL;
   reg->mxfrmCell   = NULL;
   reg->xtoCell     = NULL;
   reg->memoCell    = NULL;
   reg->creditCell  = NULL;
   reg->debitCell   = NULL;
   reg->priceCell   = NULL;
   reg->valueCell   = NULL;

   reg->ncreditCell  = NULL;
   reg->ndebitCell   = NULL;

   /* free the memory itself */
   free (reg);
}

/* ============================================== */

unsigned int
xaccSplitRegisterGetChangeFlag (SplitRegister *reg)
{

   unsigned int changed = 0;

   /* be careful to use bitwise ands and ors to assemble bit flag */
   changed |= MOD_DATE  & reg->dateCell->cell.changed;
   changed |= MOD_NUM   & reg->numCell->cell.changed;
   changed |= MOD_DESC  & reg->descCell->cell.changed;
   changed |= MOD_RECN  & reg->recnCell->changed;

   changed |= MOD_ACTN  & reg->actionCell->cell.changed;
   changed |= MOD_XFRM  & reg->xfrmCell->cell.changed;
   changed |= MOD_MXFRM & reg->mxfrmCell->cell.changed;
   changed |= MOD_XTO   & reg->xtoCell->cell.changed; 
   changed |= MOD_MEMO  & reg->memoCell->cell.changed;
   changed |= MOD_AMNT  & reg->creditCell->cell.changed;
   changed |= MOD_AMNT  & reg->debitCell->cell.changed;
   changed |= MOD_PRIC  & reg->priceCell->cell.changed;
   changed |= MOD_VALU  & reg->valueCell->cell.changed; 

   changed |= MOD_NAMNT & reg->ncreditCell->cell.changed;
   changed |= MOD_NAMNT & reg->ndebitCell->cell.changed;

   return changed;
}

/* ============================================== */

void
xaccSplitRegisterClearChangeFlag (SplitRegister *reg)
{
   reg->dateCell->cell.changed = 0;
   reg->numCell->cell.changed = 0;
   reg->descCell->cell.changed = 0;
   reg->recnCell->changed = 0;

   reg->actionCell->cell.changed = 0;
   reg->xfrmCell->cell.changed = 0;
   reg->mxfrmCell->cell.changed = 0;
   reg->xtoCell->cell.changed = 0;
   reg->memoCell->cell.changed = 0;
   reg->creditCell->cell.changed = 0;
   reg->debitCell->cell.changed = 0;
   reg->priceCell->cell.changed = 0;
   reg->valueCell->cell.changed = 0;

   reg->ncreditCell->cell.changed = 0;
   reg->ndebitCell->cell.changed = 0;
}

/* ============================================== */

static CursorType
sr_cellblock_cursor_type(SplitRegister *reg, CellBlock *cursor)
{
  if (cursor == NULL)
    return CURSOR_NONE;

  if ((cursor == reg->single_cursor) ||
      (cursor == reg->double_cursor) ||
      (cursor == reg->trans_cursor))
    return CURSOR_TRANS;

  if (cursor == reg->split_cursor)
    return CURSOR_SPLIT;

  return CURSOR_NONE;
}

/* ============================================== */

CursorType
xaccSplitRegisterGetCursorType (SplitRegister *reg)
{
  Table *table;

  if (reg == NULL)
    return CURSOR_NONE;

  table = reg->table;
  if (table == NULL)
    return CURSOR_NONE;

  return sr_cellblock_cursor_type(reg, table->current_cursor);
}

/* ============================================== */

CursorType
xaccSplitRegisterGetCursorTypeRowCol (SplitRegister *reg,
                                      int virt_row, int virt_col)
{
  Table *table;

  if (reg == NULL)
    return CURSOR_NONE;

  table = reg->table;
  if (table == NULL)
    return CURSOR_NONE;

  if ((virt_row < 0) || (virt_row >= table->num_virt_rows) ||
      (virt_col < 0) || (virt_col >= table->num_virt_cols))
    return CURSOR_NONE;

  return sr_cellblock_cursor_type(reg, table->handlers[virt_row][virt_col]);
}

/* ============================================== */

CellType
sr_cell_type (SplitRegister *reg, void * cell)
{
  if (cell == reg->dateCell)
    return DATE_CELL;

  if (cell == reg->numCell)
    return NUM_CELL;

  if (cell == reg->descCell)
    return DESC_CELL;

  if (cell == reg->recnCell)
    return RECN_CELL;

  if (cell == reg->shrsCell)
    return SHRS_CELL;

  if (cell == reg->balanceCell)
    return BALN_CELL;

  if (cell == reg->actionCell)
    return ACTN_CELL;

  if (cell == reg->xfrmCell)
    return XFRM_CELL;

  if (cell == reg->mxfrmCell)
    return MXFRM_CELL;

  if (cell == reg->xtoCell)
    return XTO_CELL;

  if (cell == reg->memoCell)
    return MEMO_CELL;

  if (cell == reg->creditCell)
    return CRED_CELL;

  if (cell == reg->debitCell)
    return DEBT_CELL;

  if (cell == reg->priceCell)
    return PRIC_CELL;

  if (cell == reg->valueCell)
    return VALU_CELL;

  if (cell == reg->ncreditCell)
    return NCRED_CELL;

  if (cell == reg->ndebitCell)
    return NDEBT_CELL;

  return NO_CELL;
}

/* ============================================== */

CellType
xaccSplitRegisterGetCellType (SplitRegister *reg)
{
  Table *table;

  if (reg == NULL)
    return NO_CELL;

  table = reg->table;
  if (table == NULL)
    return NO_CELL;

  return xaccSplitRegisterGetCellTypeRowCol(reg,
                                            table->current_cursor_phys_row,
                                            table->current_cursor_phys_col);
}

/* ============================================== */

static BasicCell *
sr_current_cell (SplitRegister *reg)
{
  Table *table;
  Locator *locator;
  CellBlock *cellblock;
  int phys_row, phys_col;
  int virt_row, virt_col;
  int cell_row, cell_col;

  if (reg == NULL)
    return NULL;

  table = reg->table;
  if (table == NULL)
    return NULL;

  phys_row = table->current_cursor_phys_row;
  phys_col = table->current_cursor_phys_col;

  if ((phys_row < 0) || (phys_row >= table->num_phys_rows) ||
      (phys_col < 0) || (phys_col >= table->num_phys_cols))
    return NULL;

  locator = table->locators[phys_row][phys_col];

  virt_row = locator->virt_row;
  virt_col = locator->virt_col;
  cell_row = locator->phys_row_offset;
  cell_col = locator->phys_col_offset;

  cellblock = table->handlers[virt_row][virt_col];

  return cellblock->cells[cell_row][cell_col];
}

/* ============================================== */

CellType
xaccSplitRegisterGetCellTypeRowCol (SplitRegister *reg,
                                    int phys_row, int phys_col)
{
  BasicCell *cell;

  cell = sr_current_cell (reg);
  if (cell == NULL)
    return NO_CELL;

  return sr_cell_type (reg, cell);
}

/* ============================================== */

gncBoolean
xaccSplitRegisterGetCellRowCol (SplitRegister *reg, CellType cell_type,
                                int *p_phys_row, int *p_phys_col)
{
  Table *table;
  Locator *locator;
  CellBlock *cellblock;
  int phys_row, phys_col;
  int virt_row, virt_col;
  int cell_row, cell_col;

  if (reg == NULL)
    return GNC_F;

  table = reg->table;
  if (table == NULL)
    return GNC_F;

  phys_row = table->current_cursor_phys_row;
  phys_col = table->current_cursor_phys_col;

  if ((phys_row < 0) || (phys_row >= table->num_phys_rows) ||
      (phys_col < 0) || (phys_col >= table->num_phys_cols))
    return GNC_F;

  locator = table->locators[phys_row][phys_col];

  virt_row = locator->virt_row;
  virt_col = locator->virt_col;

  cellblock = table->handlers[virt_row][virt_col];

  for (cell_row = 0; cell_row < cellblock->numRows; cell_row++)
    for (cell_col = 0; cell_col < cellblock->numCols; cell_col++)
    {
      BasicCell *cell = cellblock->cells[cell_row][cell_col];

      if (sr_cell_type (reg, cell) == cell_type)
      {
        RevLocator *rev_locator;

        rev_locator = table->rev_locators[virt_row][virt_col];

        phys_row = rev_locator->phys_row + cell_row;
        phys_col = rev_locator->phys_col + cell_col;

        if (p_phys_row != NULL)
          *p_phys_row = phys_row;

        if (p_phys_col != NULL)
          *p_phys_col = phys_col;

        return GNC_T;
      }
    }

  return GNC_F;
}

/* ============================================== */

SplitRegisterBuffer *
xaccMallocSplitRegisterBuffer ()
{
  SplitRegisterBuffer *srb;

  srb = calloc(1, sizeof(SplitRegisterBuffer));

  assert(srb != NULL);

  return srb;
}

/* ============================================== */

static void
destroyCellBuffer(CellBuffer *cb)
{
  if (cb == NULL)
    return;

  if (cb->value != NULL)
    free(cb->value);

  cb->value = NULL;
}

void
xaccDestroySplitRegisterBuffer (SplitRegisterBuffer *srb)
{
  if (srb == NULL)
    return;

  destroyCellBuffer(&srb->dateCell);
  destroyCellBuffer(&srb->numCell);
  destroyCellBuffer(&srb->descCell);
  destroyCellBuffer(&srb->recnCell);
  destroyCellBuffer(&srb->shrsCell);
  destroyCellBuffer(&srb->balanceCell);
  destroyCellBuffer(&srb->actionCell);
  destroyCellBuffer(&srb->xfrmCell);
  destroyCellBuffer(&srb->mxfrmCell);
  destroyCellBuffer(&srb->xtoCell);
  destroyCellBuffer(&srb->memoCell);
  destroyCellBuffer(&srb->creditCell);
  destroyCellBuffer(&srb->debitCell);
  destroyCellBuffer(&srb->priceCell);
  destroyCellBuffer(&srb->valueCell);
  destroyCellBuffer(&srb->ncreditCell);
  destroyCellBuffer(&srb->ndebitCell);

  free(srb);
}

/* ============================================== */

static void
saveCell(BasicCell *bcell, CellBuffer *cb)
{
  if ((bcell == NULL) || (cb == NULL))
    return;

  if (cb->value != NULL)
    free(cb->value);

  cb->value = bcell->value;

  if (cb->value != NULL)
  {
    cb->value = strdup(cb->value);
    assert(cb->value != NULL);
  }

  cb->changed = bcell->changed;
}

void
xaccSplitRegisterSaveCursor(SplitRegister *sr, SplitRegisterBuffer *srb)
{
  if ((sr == NULL) || (srb == NULL))
    return;

  saveCell(&sr->dateCell->cell, &srb->dateCell);
  saveCell(&sr->numCell->cell, &srb->numCell);
  saveCell(&sr->descCell->cell, &srb->descCell);
  saveCell(sr->recnCell, &srb->recnCell);
  saveCell(&sr->shrsCell->cell, &srb->shrsCell);
  saveCell(&sr->balanceCell->cell, &srb->balanceCell);
  saveCell(&sr->actionCell->cell, &srb->actionCell);
  saveCell(&sr->xfrmCell->cell, &srb->xfrmCell);
  saveCell(&sr->mxfrmCell->cell, &srb->mxfrmCell);
  saveCell(&sr->xtoCell->cell, &srb->xtoCell);
  saveCell(&sr->memoCell->cell, &srb->memoCell);
  saveCell(&sr->creditCell->cell, &srb->creditCell);
  saveCell(&sr->debitCell->cell, &srb->debitCell);
  saveCell(&sr->priceCell->cell, &srb->priceCell);
  saveCell(&sr->valueCell->cell, &srb->valueCell);
  saveCell(&sr->ncreditCell->cell, &srb->ncreditCell);
  saveCell(&sr->ndebitCell->cell, &srb->ndebitCell);
}

/* ============================================== */

static void
restoreCellChanged(BasicCell *bcell, CellBuffer *cb)
{
  if ((bcell == NULL) || (cb == NULL))
    return;

  if (cb->changed)
  {
    xaccSetBasicCellValue(bcell, cb->value);
    bcell->changed = cb->changed;
  }
}

void
xaccSplitRegisterRestoreCursorChanged(SplitRegister *sr,
                                      SplitRegisterBuffer *srb)
{
  if ((sr == NULL) || (srb == NULL))
    return;

  restoreCellChanged(&sr->dateCell->cell, &srb->dateCell);
  restoreCellChanged(&sr->numCell->cell, &srb->numCell);
  restoreCellChanged(&sr->descCell->cell, &srb->descCell);
  restoreCellChanged(sr->recnCell, &srb->recnCell);
  restoreCellChanged(&sr->shrsCell->cell, &srb->shrsCell);
  restoreCellChanged(&sr->balanceCell->cell, &srb->balanceCell);
  restoreCellChanged(&sr->actionCell->cell, &srb->actionCell);
  restoreCellChanged(&sr->xfrmCell->cell, &srb->xfrmCell);
  restoreCellChanged(&sr->mxfrmCell->cell, &srb->mxfrmCell);
  restoreCellChanged(&sr->xtoCell->cell, &srb->xtoCell);
  restoreCellChanged(&sr->memoCell->cell, &srb->memoCell);
  restoreCellChanged(&sr->creditCell->cell, &srb->creditCell);
  restoreCellChanged(&sr->debitCell->cell, &srb->debitCell);
  restoreCellChanged(&sr->priceCell->cell, &srb->priceCell);
  restoreCellChanged(&sr->valueCell->cell, &srb->valueCell);
  restoreCellChanged(&sr->ncreditCell->cell, &srb->ncreditCell);
  restoreCellChanged(&sr->ndebitCell->cell, &srb->ndebitCell);
}

/* ============ END OF FILE ===================== */