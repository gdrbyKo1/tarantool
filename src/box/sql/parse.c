/*
** 2000-05-29
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** Driver template for the LEMON parser generator.
**
** The "lemon" program processes an LALR(1) input grammar file, then uses
** this template to construct a parser.  The "lemon" program inserts text
** at each "%%" line.  Also, any "P-a-r-s-e" identifer prefix (without the
** interstitial "-" characters) contained in this template is changed into
** the value of the %name directive from the grammar.  Otherwise, the content
** of this template is copied straight through into the generate parser
** source file.
**
** The following is the concatenation of all %include directives from the
** input grammar file:
*/
#include <stdio.h>
#include <stdbool.h>
/************ Begin %include sections from the grammar ************************/
#line 52 "/home/kir/tarantool/src/box/sql/parse.y"

#include "sqliteInt.h"
#include "box/fkey.h"

/*
** Disable all error recovery processing in the parser push-down
** automaton.
*/
#define YYNOERRORRECOVERY 1

/*
** Make yytestcase() the same as testcase()
*/
#define yytestcase(X) testcase(X)

/*
** Indicate that sqlite3ParserFree() will never be called with a null
** pointer.
*/
#define YYPARSEFREENEVERNULL 1

/*
** Alternative datatype for the argument to the malloc() routine passed
** into sqlite3ParserAlloc().  The default is size_t.
*/
#define YYMALLOCARGTYPE  u64

/*
** An instance of this structure holds information about the
** LIMIT clause of a SELECT statement.
*/
struct LimitVal {
  Expr *pLimit;    /* The LIMIT expression.  NULL if there is no limit */
  Expr *pOffset;   /* The OFFSET expression.  NULL if there is none */
};

/*
** An instance of the following structure describes the event of a
** TRIGGER.  "a" is the event type, one of TK_UPDATE, TK_INSERT,
** TK_DELETE, or TK_INSTEAD.  If the event is of the form
**
**      UPDATE ON (a,b,c)
**
** Then the "b" IdList records the list "a,b,c".
*/
struct TrigEvent { int a; IdList * b; };

/*
** Disable lookaside memory allocation for objects that might be
** shared across database connections.
*/
static void disableLookaside(Parse *pParse){
  pParse->disableLookaside++;
  pParse->db->lookaside.bDisable++;
}

#line 384 "/home/kir/tarantool/src/box/sql/parse.y"

  /**
   * For a compound SELECT statement, make sure
   * p->pPrior->pNext==p for all elements in the list. And make
   * sure list length does not exceed SQL_LIMIT_COMPOUND_SELECT.
   */
  static void parserDoubleLinkSelect(Parse *pParse, Select *p){
    if( p->pPrior ){
      Select *pNext = 0, *pLoop;
      int mxSelect, cnt = 0;
      for(pLoop=p; pLoop; pNext=pLoop, pLoop=pLoop->pPrior, cnt++){
        pLoop->pNext = pNext;
        pLoop->selFlags |= SF_Compound;
      }
      if( (p->selFlags & SF_MultiValue)==0 && 
        (mxSelect = pParse->db->aLimit[SQL_LIMIT_COMPOUND_SELECT])>0 &&
        cnt>mxSelect
      ){
        sqlite3ErrorMsg(pParse, "Too many UNION or EXCEPT or INTERSECT "
                        "operations (limit %d is set)",
                        pParse->db->aLimit[SQL_LIMIT_COMPOUND_SELECT]);
      }
    }
  }
#line 803 "/home/kir/tarantool/src/box/sql/parse.y"

  /* This is a utility routine used to set the ExprSpan.zStart and
  ** ExprSpan.zEnd values of pOut so that the span covers the complete
  ** range of text beginning with pStart and going to the end of pEnd.
  */
  static void spanSet(ExprSpan *pOut, Token *pStart, Token *pEnd){
    pOut->zStart = pStart->z;
    pOut->zEnd = &pEnd->z[pEnd->n];
  }

  /* Construct a new Expr object from a single identifier.  Use the
  ** new Expr to populate pOut.  Set the span of pOut to be the identifier
  ** that created the expression.
  */
  static void spanExpr(ExprSpan *pOut, Parse *pParse, int op, Token t){
    Expr *p = sqlite3DbMallocRawNN(pParse->db, sizeof(Expr)+t.n+1);
    if( p ){
      memset(p, 0, sizeof(Expr));
      switch (op) {
      case TK_STRING:
        p->affinity = AFFINITY_TEXT;
        break;
      case TK_BLOB:
        p->affinity = AFFINITY_BLOB;
        break;
      case TK_INTEGER:
        p->affinity = AFFINITY_INTEGER;
        break;
      case TK_FLOAT:
        p->affinity = AFFINITY_REAL;
        break;
      }
      p->op = (u8)op;
      p->flags = EP_Leaf;
      p->iAgg = -1;
      p->u.zToken = (char*)&p[1];
      memcpy(p->u.zToken, t.z, t.n);
      p->u.zToken[t.n] = 0;
      if (op != TK_VARIABLE){
        sqlite3NormalizeName(p->u.zToken);
      }
#if SQLITE_MAX_EXPR_DEPTH>0
      p->nHeight = 1;
#endif  
    }
    pOut->pExpr = p;
    pOut->zStart = t.z;
    pOut->zEnd = &t.z[t.n];
  }
#line 941 "/home/kir/tarantool/src/box/sql/parse.y"

  /* This routine constructs a binary expression node out of two ExprSpan
  ** objects and uses the result to populate a new ExprSpan object.
  */
  static void spanBinaryExpr(
    Parse *pParse,      /* The parsing context.  Errors accumulate here */
    int op,             /* The binary operation */
    ExprSpan *pLeft,    /* The left operand, and output */
    ExprSpan *pRight    /* The right operand */
  ){
    pLeft->pExpr = sqlite3PExpr(pParse, op, pLeft->pExpr, pRight->pExpr);
    pLeft->zEnd = pRight->zEnd;
  }

  /* If doNot is true, then add a TK_NOT Expr-node wrapper around the
  ** outside of *ppExpr.
  */
  static void exprNot(Parse *pParse, int doNot, ExprSpan *pSpan){
    if( doNot ){
      pSpan->pExpr = sqlite3PExpr(pParse, TK_NOT, pSpan->pExpr, 0);
    }
  }
#line 1015 "/home/kir/tarantool/src/box/sql/parse.y"

  /* Construct an expression node for a unary postfix operator
  */
  static void spanUnaryPostfix(
    Parse *pParse,         /* Parsing context to record errors */
    int op,                /* The operator */
    ExprSpan *pOperand,    /* The operand, and output */
    Token *pPostOp         /* The operand token for setting the span */
  ){
    pOperand->pExpr = sqlite3PExpr(pParse, op, pOperand->pExpr, 0);
    pOperand->zEnd = &pPostOp->z[pPostOp->n];
  }                           
#line 1036 "/home/kir/tarantool/src/box/sql/parse.y"

  /* Construct an expression node for a unary prefix operator
  */
  static void spanUnaryPrefix(
    ExprSpan *pOut,        /* Write the new expression node here */
    Parse *pParse,         /* Parsing context to record errors */
    int op,                /* The operator */
    ExprSpan *pOperand,    /* The operand */
    Token *pPreOp         /* The operand token for setting the span */
  ){
    pOut->zStart = pPreOp->z;
    pOut->pExpr = sqlite3PExpr(pParse, op, pOperand->pExpr, 0);
    pOut->zEnd = pOperand->zEnd;
  }
#line 1236 "/home/kir/tarantool/src/box/sql/parse.y"

  /* Add a single new term to an ExprList that is used to store a
  ** list of identifiers.  Report an error if the ID list contains
  ** a COLLATE clause or an ASC or DESC keyword, except ignore the
  ** error while parsing a legacy schema.
  */
  static ExprList *parserAddExprIdListTerm(
    Parse *pParse,
    ExprList *pPrior,
    Token *pIdToken,
    int hasCollate,
    int sortOrder
  ){
    ExprList *p = sql_expr_list_append(pParse->db, pPrior, NULL);
    if( (hasCollate || sortOrder != SORT_ORDER_UNDEF)
        && pParse->db->init.busy==0
    ){
      sqlite3ErrorMsg(pParse, "syntax error after column name \"%.*s\"",
                         pIdToken->n, pIdToken->z);
    }
    sqlite3ExprListSetName(pParse, p, pIdToken, 1);
    return p;
  }
#line 236 "parse.c"
/**************** End of %include directives **********************************/
/* These constants specify the various numeric values for terminal symbols
** in a format understandable to "makeheaders".  This section is blank unless
** "lemon" is run with the "-m" command-line option.
***************** Begin makeheaders token definitions *************************/
/**************** End makeheaders token definitions ***************************/

/* The next sections is a series of control #defines.
** various aspects of the generated parser.
**    YYCODETYPE         is the data type used to store the integer codes
**                       that represent terminal and non-terminal symbols.
**                       "unsigned char" is used if there are fewer than
**                       256 symbols.  Larger types otherwise.
**    YYNOCODE           is a number of type YYCODETYPE that is not used for
**                       any terminal or nonterminal symbol.
**    YYFALLBACK         If defined, this indicates that one or more tokens
**                       (also known as: "terminal symbols") have fall-back
**                       values which should be used if the original symbol
**                       would not parse.  This permits keywords to sometimes
**                       be used as identifiers, for example.
**    YYACTIONTYPE       is the data type used for "action codes" - numbers
**                       that indicate what to do in response to the next
**                       token.
**    sqlite3ParserTOKENTYPE     is the data type used for minor type for terminal
**                       symbols.  Background: A "minor type" is a semantic
**                       value associated with a terminal or non-terminal
**                       symbols.  For example, for an "ID" terminal symbol,
**                       the minor type might be the name of the identifier.
**                       Each non-terminal can have a different minor type.
**                       Terminal symbols all have the same minor type, though.
**                       This macros defines the minor type for terminal 
**                       symbols.
**    YYMINORTYPE        is the data type used for all minor types.
**                       This is typically a union of many types, one of
**                       which is sqlite3ParserTOKENTYPE.  The entry in the union
**                       for terminal symbols is called "yy0".
**    YYSTACKDEPTH       is the maximum depth of the parser's stack.  If
**                       zero the stack is dynamically sized using realloc()
**    sqlite3ParserARG_SDECL     A static variable declaration for the %extra_argument
**    sqlite3ParserARG_PDECL     A parameter declaration for the %extra_argument
**    sqlite3ParserARG_STORE     Code to store %extra_argument into yypParser
**    sqlite3ParserARG_FETCH     Code to extract %extra_argument from yypParser
**    YYERRORSYMBOL      is the code number of the error symbol.  If not
**                       defined, then do no error processing.
**    YYNSTATE           the combined number of states.
**    YYNRULE            the number of rules in the grammar
**    YY_MAX_SHIFT       Maximum value for shift actions
**    YY_MIN_SHIFTREDUCE Minimum value for shift-reduce actions
**    YY_MAX_SHIFTREDUCE Maximum value for shift-reduce actions
**    YY_MIN_REDUCE      Maximum value for reduce actions
**    YY_ERROR_ACTION    The yy_action[] code for syntax error
**    YY_ACCEPT_ACTION   The yy_action[] code for accept
**    YY_NO_ACTION       The yy_action[] code for no-op
*/
#ifndef INTERFACE
# define INTERFACE 1
#endif
/************* Begin control #defines *****************************************/
#define YYCODETYPE unsigned char
#define YYNOCODE 247
#define YYACTIONTYPE unsigned short int
#define YYWILDCARD 71
#define sqlite3ParserTOKENTYPE Token
typedef union {
  int yyinit;
  sqlite3ParserTOKENTYPE yy0;
  struct TrigEvent yy30;
  With* yy43;
  Expr* yy62;
  struct type_def yy66;
  SrcList* yy151;
  struct LimitVal yy220;
  IdList* yy240;
  int yy280;
  struct {int value; int mask;} yy359;
  TriggerStep* yy360;
  ExprSpan yy370;
  Select* yy375;
  ExprList* yy418;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define sqlite3ParserARG_SDECL Parse *pParse;
#define sqlite3ParserARG_PDECL ,Parse *pParse
#define sqlite3ParserARG_FETCH Parse *pParse = yypParser->pParse
#define sqlite3ParserARG_STORE yypParser->pParse = pParse
#define YYFALLBACK 1
#define YYNSTATE             420
#define YYNRULE              304
#define YY_MAX_SHIFT         419
#define YY_MIN_SHIFTREDUCE   628
#define YY_MAX_SHIFTREDUCE   931
#define YY_MIN_REDUCE        932
#define YY_MAX_REDUCE        1235
#define YY_ERROR_ACTION      1236
#define YY_ACCEPT_ACTION     1237
#define YY_NO_ACTION         1238
/************* End control #defines *******************************************/

/* Define the yytestcase() macro to be a no-op if is not already defined
** otherwise.
**
** Applications can choose to define yytestcase() in the %include section
** to a macro that can assist in verifying code coverage.  For production
** code the yytestcase() macro should be turned off.  But it is useful
** for testing.
*/
#ifndef yytestcase
# define yytestcase(X)
#endif


/* Next are the tables used to determine what action to take based on the
** current state and lookahead token.  These tables are used to implement
** functions that take a state number and lookahead value and return an
** action integer.  
**
** Suppose the action integer is N.  Then the action is determined as
** follows
**
**   0 <= N <= YY_MAX_SHIFT             Shift N.  That is, push the lookahead
**                                      token onto the stack and goto state N.
**
**   N between YY_MIN_SHIFTREDUCE       Shift to an arbitrary state then
**     and YY_MAX_SHIFTREDUCE           reduce by rule N-YY_MIN_SHIFTREDUCE.
**
**   N between YY_MIN_REDUCE            Reduce by rule N-YY_MIN_REDUCE
**     and YY_MAX_REDUCE
**
**   N == YY_ERROR_ACTION               A syntax error has occurred.
**
**   N == YY_ACCEPT_ACTION              The parser accepts its input.
**
**   N == YY_NO_ACTION                  No such action.  Denotes unused
**                                      slots in the yy_action[] table.
**
** The action table is constructed as a single large table named yy_action[].
** Given state S and lookahead X, the action is computed as either:
**
**    (A)   N = yy_action[ yy_shift_ofst[S] + X ]
**    (B)   N = yy_default[S]
**
** The (A) formula is preferred.  The B formula is used instead if:
**    (1)  The yy_shift_ofst[S]+X value is out of range, or
**    (2)  yy_lookahead[yy_shift_ofst[S]+X] is not equal to X, or
**    (3)  yy_shift_ofst[S] equal YY_SHIFT_USE_DFLT.
** (Implementation note: YY_SHIFT_USE_DFLT is chosen so that
** YY_SHIFT_USE_DFLT+X will be out of range for all possible lookaheads X.
** Hence only tests (1) and (2) need to be evaluated.)
**
** The formulas above are for computing the action when the lookahead is
** a terminal symbol.  If the lookahead is a non-terminal (as occurs after
** a reduce action) then the yy_reduce_ofst[] array is used in place of
** the yy_shift_ofst[] array and YY_REDUCE_USE_DFLT is used in place of
** YY_SHIFT_USE_DFLT.
**
** The following are the tables generated in this section:
**
**  yy_action[]        A single table containing all actions.
**  yy_lookahead[]     A table containing the lookahead for each entry in
**                     yy_action.  Used to detect hash collisions.
**  yy_shift_ofst[]    For each state, the offset into yy_action for
**                     shifting terminals.
**  yy_reduce_ofst[]   For each state, the offset into yy_action for
**                     shifting non-terminals after a reduce.
**  yy_default[]       Default action for each state.
**
*********** Begin parsing tables **********************************************/
#define YY_ACTTAB_COUNT (1443)
static const YYACTIONTYPE yy_action[] = {
 /*     0 */    55,   56,  281,  303,  792,  792,  802,  805,   53,   53,
 /*    10 */    54,   54,   54,   54,  739,   52,   52,   52,   52,   51,
 /*    20 */    51,   50,   50,   50,   49,  304,  886,  880,  887,  872,
 /*    30 */   662,  881,  883,  266,   50,   50,   50,   49,  304,  872,
 /*    40 */   209,  168,  912,   51,   51,   50,   50,   50,   49,  304,
 /*    50 */   879,  882,  266,  886,  886,  887,  265,  265,  265,  793,
 /*    60 */   793,  803,  806,  304,   48,   46,  175,  912, 1237,  419,
 /*    70 */     8,  645,  645,   55,   56,  281,  303,  792,  792,  802,
 /*    80 */   805,   53,   53,   54,   54,   54,   54,  120,   52,   52,
 /*    90 */    52,   52,   51,   51,   50,   50,   50,   49,  304,   55,
 /*   100 */    56,  281,  303,  792,  792,  802,  805,   53,   53,   54,
 /*   110 */    54,   54,   54,  283,   52,   52,   52,   52,   51,   51,
 /*   120 */    50,   50,   50,   49,  304,  760,   55,   56,  281,  303,
 /*   130 */   792,  792,  802,  805,   53,   53,   54,   54,   54,   54,
 /*   140 */    33,   52,   52,   52,   52,   51,   51,   50,   50,   50,
 /*   150 */    49,  304,  176,  176,  763,  414,  105,  201,  167,   57,
 /*   160 */   359,  356,  355,  142,  372,  244,  711,   88,   88,  238,
 /*   170 */   333,  237,   48,   46,  175,  354,  740,  741,  321,   48,
 /*   180 */    46,  175,   55,   56,  281,  303,  792,  792,  802,  805,
 /*   190 */    53,   53,   54,   54,   54,   54,  647,   52,   52,   52,
 /*   200 */    52,   51,   51,   50,   50,   50,   49,  304,   52,   52,
 /*   210 */    52,   52,   51,   51,   50,   50,   50,   49,  304,  405,
 /*   220 */   650,  655, 1178, 1178,  260,  120,   30,   55,   56,  281,
 /*   230 */   303,  792,  792,  802,  805,   53,   53,   54,   54,   54,
 /*   240 */    54,  413,   52,   52,   52,   52,   51,   51,   50,   50,
 /*   250 */    50,   49,  304,  738,  782,  862,  774,  418,  418,  769,
 /*   260 */   137,  220,  120,  117,  645,  645,  648,  698,  374,  317,
 /*   270 */   629,  307,   55,   56,  281,  303,  792,  792,  802,  805,
 /*   280 */    53,   53,   54,   54,   54,   54,  105,   52,   52,   52,
 /*   290 */    52,   51,   51,   50,   50,   50,   49,  304,   63,  105,
 /*   300 */   773,  773,  775,   48,   46,  175,  685,  662,  138,  796,
 /*   310 */   334,  783,  645,  645,  160,  173,  685,   55,   56,  281,
 /*   320 */   303,  792,  792,  802,  805,   53,   53,   54,   54,   54,
 /*   330 */    54,  343,   52,   52,   52,   52,   51,   51,   50,   50,
 /*   340 */    50,   49,  304,  317,  316,  317,  700,    4,  649,  209,
 /*   350 */   111,  912,  849,  849,  328,  272,  768,  372,  276,  667,
 /*   360 */   668,  669,   55,   56,  281,  303,  792,  792,  802,  805,
 /*   370 */    53,   53,   54,   54,   54,   54,  912,   52,   52,   52,
 /*   380 */    52,   51,   51,   50,   50,   50,   49,  304,   55,   56,
 /*   390 */   281,  303,  792,  792,  802,  805,   53,   53,   54,   54,
 /*   400 */    54,   54,  904,   52,   52,   52,   52,   51,   51,   50,
 /*   410 */    50,   50,   49,  304,   55,   56,  281,  303,  792,  792,
 /*   420 */   802,  805,   53,   53,   54,   54,   54,   54,  261,   52,
 /*   430 */    52,   52,   52,   51,   51,   50,   50,   50,   49,  304,
 /*   440 */    55,   56,  281,  303,  792,  792,  802,  805,   53,   53,
 /*   450 */    54,   54,   54,   54,  112,   52,   52,   52,   52,   51,
 /*   460 */    51,   50,   50,   50,   49,  304,  159,  158,  157,   55,
 /*   470 */    45,  281,  303,  792,  792,  802,  805,   53,   53,   54,
 /*   480 */    54,   54,   54,   35,   52,   52,   52,   52,   51,   51,
 /*   490 */    50,   50,   50,   49,  304,   43,  666,  313,  286,  285,
 /*   500 */   665,  846,  171,  845,  725,  155,  154,   49,  304,   38,
 /*   510 */   763,  326,   40,   41,  401,  314,  176,  176,  713,   42,
 /*   520 */   865,  645,  645,  217,   59,  238,  323,  226,  372,  261,
 /*   530 */   403,    2, 1123,   20,  866,  305,  305,  414,  308,  645,
 /*   540 */   645,  218,  369,  767,  867,  691,  921,  318,  925,   18,
 /*   550 */    18,  782,  692,  774,  392,  923,  769,  924,  782,  718,
 /*   560 */   774,  163,  725,  769,  291,   56,  281,  303,  792,  792,
 /*   570 */   802,  805,   53,   53,   54,   54,   54,   54,   43,   52,
 /*   580 */    52,   52,   52,   51,   51,   50,   50,   50,   49,  304,
 /*   590 */   926,  771,  926,  644,  297,   40,   41,  773,  773,  775,
 /*   600 */   664,  405,   42,  327,  773,  773,  775,  776,  412,  916,
 /*   610 */   917,  918,   15,  403,    2,  397,   43,  294,  305,  305,
 /*   620 */   725,  835,  837,  835,  645,  645,  120,  218,  369,  351,
 /*   630 */   379,  168,  414,   40,   41,  712,  414,  392,  645,  645,
 /*   640 */    42,  782,  862,  774,   18,   18,  769,  364,   89,   89,
 /*   650 */   201,  403,    2,  359,  356,  355,  305,  305,  865,  293,
 /*   660 */   219,  153,  250,  362,  654,  361,  204,  368,  354,  703,
 /*   670 */   703,  644,  866,  105,  771,  392,  245,  163,  725,  782,
 /*   680 */   208,  774,  867,  391,  769,  387,  370,  773,  773,  775,
 /*   690 */   776,  412,  916,  917,  918,   15,  405,  334,  162,  411,
 /*   700 */   405,  390,  373,  283,  231,  913,  913,  414,  336,  120,
 /*   710 */   414,  245,  771,   64,   17,   17,  708,  342,  332,   75,
 /*   720 */    75,  707,   89,   89,  165,  773,  773,  775,  776,  412,
 /*   730 */   916,  917,  918,   15,  281,  303,  792,  792,  802,  805,
 /*   740 */    53,   53,   54,   54,   54,   54,   43,   52,   52,   52,
 /*   750 */    52,   51,   51,   50,   50,   50,   49,  304,  230,  387,
 /*   760 */   375,  914,  727,   40,   41,  672,  405,  900,  111,  185,
 /*   770 */    42,  405,  331,  360,  405,  657,  279,  411,  673,  174,
 /*   780 */   173,  403,    2,  209,   65,  912,  305,  305,  645,  645,
 /*   790 */   414,  843,  202,  229,  202,  338,  228,  342,  295,  342,
 /*   800 */   763,  190,   76,   76,  336,  392,  414,  663,    1,  782,
 /*   810 */   912,  774,  645,  645,  769,  238,  333,  237,   77,   77,
 /*   820 */   772,   54,   54,   54,   54,   47,   52,   52,   52,   52,
 /*   830 */    51,   51,   50,   50,   50,   49,  304,  252,   54,   54,
 /*   840 */    54,   54,  771,   52,   52,   52,   52,   51,   51,   50,
 /*   850 */    50,   50,   49,  304,  405,  773,  773,  775,  776,  412,
 /*   860 */   916,  917,  918,   15,  417,  191,  633,  416,  222,  273,
 /*   870 */   405,  120,  639,  690,  690,  688,  688,  262,  320,  724,
 /*   880 */   414,  236,  414,  193,  414,   32,  414,  262,  414,  385,
 /*   890 */   414,  366,   89,   89,   89,   89,   71,   71,   89,   89,
 /*   900 */    89,   89,   18,   18,  347,  284,  724,  913,  913,  344,
 /*   910 */   740,  741,  767,  414,  282,  292,  724,  200,  724,  660,
 /*   920 */   105,  254,  105,  724,  280,   89,   89,  414,   61,  387,
 /*   930 */   384,  387,  386,  367,  678,  387,  404,  387,  365,   89,
 /*   940 */    89,  752,  120,  414,  405,  319,  405,  414,  405,  860,
 /*   950 */   405,  225,  405,  186,  405,   78,   78,   60,  187,   79,
 /*   960 */    79,  184,  298,  914,  726,  911,  414,  679,  182,  315,
 /*   970 */   120,  414,  288,  414,  256,  414,  380,  405,   18,   18,
 /*   980 */   105,  161,  414,   67,   67,   68,   68,   70,   70,  414,
 /*   990 */   724,  405,  414,  382,   80,   80,  414,  259,  696,  724,
 /*  1000 */   414,   81,   81,  414,   82,   82,  414,  405,   19,   19,
 /*  1010 */   414,  405,   83,   83,  414,   84,   84,  724,   85,   85,
 /*  1020 */   414,  151,   72,   72,  376,  868,   86,   86,  414,  105,
 /*  1030 */   405,  724,   87,   87,  324,  405, 1207,  405,  680,  405,
 /*  1040 */    73,   73,    7,  767,  643,  414,  405,  724,  767,  414,
 /*  1050 */   767,  262,  697,  405,  658,  658,  405,  108,  108,  414,
 /*  1060 */   405,  109,  109,  144,  405,  377,  693,  405,  671,  670,
 /*  1070 */   405,  110,  110,  242,  405,  105,  873,  414,  405,  301,
 /*  1080 */   340,  414,  233,  414,  405,   39,  414,   37,  414,   93,
 /*  1090 */    93,  394,  405,   74,   74,   96,   96,  398,   90,   90,
 /*  1100 */    97,   97,  414,  289,  414,  224,  402,  414,  290,  405,
 /*  1110 */   299,  825,  414,  405,   95,   95,  107,  107,  414,  104,
 /*  1120 */   104,  341,  105,  405,  101,  101,  414,  241,  105,  249,
 /*  1130 */   100,  100,  106,  312,  147,  708,  414,  105,   98,   98,
 /*  1140 */   707,  405,  414,  248,  414,  405,  414,  405,   99,   99,
 /*  1150 */   405,  414,  405,  871,   92,   92,   94,   94,   91,   91,
 /*  1160 */   414,  832,  832,   66,   66,  234,  405,  309,  405,  213,
 /*  1170 */   177,  405,   69,   69,  870,  842,  405,  842,  841,  765,
 /*  1180 */   841,  207,  405,  335,  337,  207,  207,  824,  839,   32,
 /*  1190 */   405,  352,  683,  213,   32,  705,  734,   34,  207,  828,
 /*  1200 */   405,  213,  890,  287,  407,  755,  405,  833,  405,  166,
 /*  1210 */   405,  777,  777,  322,  227,  405,  235,  857,  859,  339,
 /*  1220 */   856,  345,  346,  240,  405,  638,  243,  357,  949,  156,
 /*  1230 */   681,  246,  662,  732,  247,  766,  258,  714,  393,  830,
 /*  1240 */   263,  264,  829,  271,  646,    6,  152,  636,  897,  635,
 /*  1250 */   637,  143,  135,  275,  124,  115,   30,  751,  325,  126,
 /*  1260 */   844,   21,  330,  232,  350,  189,  128,  196,  129,  348,
 /*  1270 */   197,  145,  130,  131,  139,  198,  277,  363,  684,  762,
 /*  1280 */   296,  676,  278,  660,  653,  861,  652,  651,  378,    5,
 /*  1290 */   675,   29,   36,  396,  300,  400,  408,  410,   58,  383,
 /*  1300 */    31,  381,  888,  884,   62,  895,  415,  102,  221,  722,
 /*  1310 */   251,  223,  723,  253,  113,  721,  631,  255,  306,  103,
 /*  1320 */   720,  178,  114,  810,  203,  269,  257,  704,  267,  268,
 /*  1330 */   270,  179,  121,  311,  310,  180,  181,  838,  122,  836,
 /*  1340 */   183,  123,  761,  125,  210,  118,  127,  188,  211,  694,
 /*  1350 */   212,  207,  847,  132,  133,  855,  136,  928,  329,   22,
 /*  1360 */    23,  134,   24,   25,  119,  858,  192,  854,  194,    9,
 /*  1370 */    16,  195,  146,  239,  641,  349,  248,  140,  353,  199,
 /*  1380 */    26,   10,  358,  682,   11,  674,   27,  205,  781,  116,
 /*  1390 */   733,  779,  702,  169,  808,   12,    3,   28,  728,  706,
 /*  1400 */   388,  170,  371,  172,  141,  395,  206,  823,   34,   13,
 /*  1410 */    32,   14,  399,  809,  807,  864,  812,  863,  389,  164,
 /*  1420 */   214,  877,  148,  216,  878,  149,  406,  811,  150,  302,
 /*  1430 */   780,  215,  632,  891,  885,  778,   44,  409,  642,  274,
 /*  1440 */   797,  628, 1196,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */     5,    6,    7,    8,    9,   10,   11,   12,   13,   14,
 /*    10 */    15,   16,   17,   18,  171,   20,   21,   22,   23,   24,
 /*    20 */    25,   26,   27,   28,   29,   30,  116,  117,  118,  175,
 /*    30 */   176,  121,  122,  123,   26,   27,   28,   29,   30,  185,
 /*    40 */    45,    9,   47,   24,   25,   26,   27,   28,   29,   30,
 /*    50 */   140,  141,  142,  143,  144,  145,  146,  147,  148,    9,
 /*    60 */    10,   11,   12,   30,  221,  222,  223,   72,  150,  151,
 /*    70 */   152,   47,   48,    5,    6,    7,    8,    9,   10,   11,
 /*    80 */    12,   13,   14,   15,   16,   17,   18,  138,   20,   21,
 /*    90 */    22,   23,   24,   25,   26,   27,   28,   29,   30,    5,
 /*   100 */     6,    7,    8,    9,   10,   11,   12,   13,   14,   15,
 /*   110 */    16,   17,   18,   81,   20,   21,   22,   23,   24,   25,
 /*   120 */    26,   27,   28,   29,   30,  163,    5,    6,    7,    8,
 /*   130 */     9,   10,   11,   12,   13,   14,   15,   16,   17,   18,
 /*   140 */    46,   20,   21,   22,   23,   24,   25,   26,   27,   28,
 /*   150 */    29,   30,  192,  193,   67,  156,  194,   73,   46,   65,
 /*   160 */    76,   77,   78,  139,  204,   44,  208,  168,  169,   82,
 /*   170 */    83,   84,  221,  222,  223,   91,  108,  109,  218,  221,
 /*   180 */   222,  223,    5,    6,    7,    8,    9,   10,   11,   12,
 /*   190 */    13,   14,   15,   16,   17,   18,  168,   20,   21,   22,
 /*   200 */    23,   24,   25,   26,   27,   28,   29,   30,   20,   21,
 /*   210 */    22,   23,   24,   25,   26,   27,   28,   29,   30,  220,
 /*   220 */   168,   44,   98,   99,  225,  138,  114,    5,    6,    7,
 /*   230 */     8,    9,   10,   11,   12,   13,   14,   15,   16,   17,
 /*   240 */    18,    7,   20,   21,   22,   23,   24,   25,   26,   27,
 /*   250 */    28,   29,   30,  171,   70,  163,   72,  153,  154,   75,
 /*   260 */    43,  157,  138,  159,   47,   48,   44,  163,  163,  156,
 /*   270 */     1,    2,    5,    6,    7,    8,    9,   10,   11,   12,
 /*   280 */    13,   14,   15,   16,   17,   18,  194,   20,   21,   22,
 /*   290 */    23,   24,   25,   26,   27,   28,   29,   30,  194,  194,
 /*   300 */   116,  117,  118,  221,  222,  223,  175,  176,   43,   75,
 /*   310 */   218,   44,   47,   48,  209,  210,  185,    5,    6,    7,
 /*   320 */     8,    9,   10,   11,   12,   13,   14,   15,   16,   17,
 /*   330 */    18,  239,   20,   21,   22,   23,   24,   25,   26,   27,
 /*   340 */    28,   29,   30,  230,  231,  232,  193,   43,  168,   45,
 /*   350 */   156,   47,   82,   83,   84,  161,   44,  204,  164,   85,
 /*   360 */    86,   87,    5,    6,    7,    8,    9,   10,   11,   12,
 /*   370 */    13,   14,   15,   16,   17,   18,   72,   20,   21,   22,
 /*   380 */    23,   24,   25,   26,   27,   28,   29,   30,    5,    6,
 /*   390 */     7,    8,    9,   10,   11,   12,   13,   14,   15,   16,
 /*   400 */    17,   18,  182,   20,   21,   22,   23,   24,   25,   26,
 /*   410 */    27,   28,   29,   30,    5,    6,    7,    8,    9,   10,
 /*   420 */    11,   12,   13,   14,   15,   16,   17,   18,  156,   20,
 /*   430 */    21,   22,   23,   24,   25,   26,   27,   28,   29,   30,
 /*   440 */     5,    6,    7,    8,    9,   10,   11,   12,   13,   14,
 /*   450 */    15,   16,   17,   18,   45,   20,   21,   22,   23,   24,
 /*   460 */    25,   26,   27,   28,   29,   30,   82,   83,   84,    5,
 /*   470 */     6,    7,    8,    9,   10,   11,   12,   13,   14,   15,
 /*   480 */    16,   17,   18,  126,   20,   21,   22,   23,   24,   25,
 /*   490 */    26,   27,   28,   29,   30,    7,  178,   51,   24,   25,
 /*   500 */   178,   52,   44,   54,   46,   24,   25,   29,   30,  126,
 /*   510 */    67,   62,   24,   25,  242,   69,  192,  193,   26,   31,
 /*   520 */    35,   47,   48,  197,   43,   82,   83,   84,  204,  156,
 /*   530 */    42,   43,   44,  207,   49,   47,   48,  156,   92,   47,
 /*   540 */    48,   98,   99,  156,   59,   60,   72,  156,   74,  168,
 /*   550 */   169,   70,   67,   72,   66,   81,   75,   83,   70,  211,
 /*   560 */    72,  103,  104,   75,  183,    6,    7,    8,    9,   10,
 /*   570 */    11,   12,   13,   14,   15,   16,   17,   18,    7,   20,
 /*   580 */    21,   22,   23,   24,   25,   26,   27,   28,   29,   30,
 /*   590 */   116,  103,  118,  166,    7,   24,   25,  116,  117,  118,
 /*   600 */   178,  220,   31,  216,  116,  117,  118,  119,  120,  121,
 /*   610 */   122,  123,  124,   42,   43,  242,    7,   30,   47,   48,
 /*   620 */    46,  230,  231,  232,   47,   48,  138,   98,   99,    7,
 /*   630 */   215,    9,  156,   24,   25,   26,  156,   66,   47,   48,
 /*   640 */    31,   70,  163,   72,  168,  169,   75,   26,  168,  169,
 /*   650 */    73,   42,   43,   76,   77,   78,   47,   48,   35,  183,
 /*   660 */    73,   74,   75,   76,   77,   78,   79,   94,   91,   96,
 /*   670 */    97,  244,   49,  194,  103,   66,   89,  103,  104,   70,
 /*   680 */   208,   72,   59,   60,   75,  205,  206,  116,  117,  118,
 /*   690 */   119,  120,  121,  122,  123,  124,  220,  218,  215,  166,
 /*   700 */   220,  189,  156,   81,   39,   47,   48,  156,  156,  138,
 /*   710 */   156,   89,  103,  234,  168,  169,   95,  156,  239,  168,
 /*   720 */   169,  100,  168,  169,  235,  116,  117,  118,  119,  120,
 /*   730 */   121,  122,  123,  124,    7,    8,    9,   10,   11,   12,
 /*   740 */    13,   14,   15,   16,   17,   18,    7,   20,   21,   22,
 /*   750 */    23,   24,   25,   26,   27,   28,   29,   30,   93,  205,
 /*   760 */   206,  103,  104,   24,   25,   55,  220,  167,  156,  217,
 /*   770 */    31,  220,  236,   63,  220,  175,  164,  244,   68,  209,
 /*   780 */   210,   42,   43,   45,   43,   47,   47,   48,   47,   48,
 /*   790 */   156,   58,  180,  128,  182,  156,  131,  156,   88,  156,
 /*   800 */    67,  240,  168,  169,  156,   66,  156,  177,   43,   70,
 /*   810 */    72,   72,   47,   48,   75,   82,   83,   84,  168,  169,
 /*   820 */   156,   15,   16,   17,   18,   19,   20,   21,   22,   23,
 /*   830 */    24,   25,   26,   27,   28,   29,   30,  208,   15,   16,
 /*   840 */    17,   18,  103,   20,   21,   22,   23,   24,   25,   26,
 /*   850 */    27,   28,   29,   30,  220,  116,  117,  118,  119,  120,
 /*   860 */   121,  122,  123,  124,   32,  217,   34,   35,   36,   37,
 /*   870 */   220,  138,   40,  188,  189,  188,  189,  156,   74,  156,
 /*   880 */   156,  240,  156,  240,  156,   46,  156,  156,  156,  163,
 /*   890 */   156,  163,  168,  169,  168,  169,  168,  169,  168,  169,
 /*   900 */   168,  169,  168,  169,  228,  184,  156,   47,   48,  233,
 /*   910 */   108,  109,  156,  156,  191,  184,  156,  183,  156,   80,
 /*   920 */   194,  208,  194,  156,   92,  168,  169,  156,   14,  205,
 /*   930 */   206,  205,  206,  205,   57,  205,  206,  205,  206,  168,
 /*   940 */   169,  191,  138,  156,  220,  113,  220,  156,  220,  163,
 /*   950 */   220,  191,  220,  191,  220,  168,  169,   43,  191,  168,
 /*   960 */   169,  129,  205,  103,  104,   46,  156,   90,  136,  137,
 /*   970 */   138,  156,  216,  156,  208,  156,  205,  220,  168,  169,
 /*   980 */   194,  156,  156,  168,  169,  168,  169,  168,  169,  156,
 /*   990 */   156,  220,  156,  183,  168,  169,  156,  156,  163,  156,
 /*  1000 */   156,  168,  169,  156,  168,  169,  156,  220,  168,  169,
 /*  1010 */   156,  220,  168,  169,  156,  168,  169,  156,  168,  169,
 /*  1020 */   156,  102,  168,  169,    7,  191,  168,  169,  156,  194,
 /*  1030 */   220,  156,  168,  169,  191,  220,   44,  220,   46,  220,
 /*  1040 */   168,  169,  196,  156,  163,  156,  220,  156,  156,  156,
 /*  1050 */   156,  156,  191,  220,   47,   48,  220,  168,  169,  156,
 /*  1060 */   220,  168,  169,  195,  220,   48,  191,  220,   74,   75,
 /*  1070 */   220,  168,  169,   39,  220,  194,  156,  156,  220,  184,
 /*  1080 */     7,  156,  191,  156,  220,  125,  156,  127,  156,  168,
 /*  1090 */   169,  163,  220,  168,  169,  168,  169,  163,  168,  169,
 /*  1100 */   168,  169,  156,  216,  156,  156,  163,  156,  216,  220,
 /*  1110 */   216,   77,  156,  220,  168,  169,  168,  169,  156,  168,
 /*  1120 */   169,   48,  194,  220,  168,  169,  156,   93,  194,   75,
 /*  1130 */   168,  169,   43,  156,   45,   95,  156,  194,  168,  169,
 /*  1140 */   100,  220,  156,   89,  156,  220,  156,  220,  168,  169,
 /*  1150 */   220,  156,  220,  156,  168,  169,  168,  169,  168,  169,
 /*  1160 */   156,   47,   48,  168,  169,  131,  220,   44,  220,   46,
 /*  1170 */    43,  220,  168,  169,  156,  116,  220,  118,  116,   44,
 /*  1180 */   118,   46,  220,   44,   44,   46,   46,   44,  156,   46,
 /*  1190 */   220,   44,   44,   46,   46,   44,   44,   46,   46,   44,
 /*  1200 */   220,   46,   44,  156,   46,  199,  220,  156,  220,  156,
 /*  1210 */   220,   47,   48,  212,  212,  220,  241,  156,  199,  241,
 /*  1220 */   156,  156,  156,  156,  220,  156,  156,  172,  101,  181,
 /*  1230 */   156,  156,  176,  156,  171,  156,  212,  156,  227,  171,
 /*  1240 */   156,  156,  171,  198,  156,  196,  196,  156,  158,  156,
 /*  1250 */   156,   43,   43,  173,  219,    5,  114,  199,   41,  187,
 /*  1260 */   238,  125,  133,  237,   41,  160,  190,  160,  190,  173,
 /*  1270 */   160,  219,  190,  190,  187,  160,  173,   81,  170,  187,
 /*  1280 */    61,  179,  173,   80,  172,  199,  170,  170,  106,   43,
 /*  1290 */   179,   81,  125,  173,   30,  173,   43,   43,  112,  107,
 /*  1300 */   111,  110,  245,  243,   46,   36,  162,  174,  155,  214,
 /*  1310 */   213,  155,  214,  213,  165,  214,    4,  213,    3,  174,
 /*  1320 */   214,   73,  165,  224,  174,  200,  213,  203,  202,  201,
 /*  1330 */   199,   79,   43,   91,   64,   73,   38,   44,   39,   44,
 /*  1340 */   101,   39,   99,  115,  226,   88,  102,   81,  229,   42,
 /*  1350 */   229,   46,  132,  132,   81,    1,  115,  135,  134,   14,
 /*  1360 */    14,  102,   14,   14,   88,   48,  105,    1,  101,   43,
 /*  1370 */   130,   81,   45,  128,   42,    7,   89,   43,   64,   79,
 /*  1380 */    43,   43,   64,   44,   43,   50,   43,   64,   44,   56,
 /*  1390 */    48,   44,   95,  101,   44,   43,   43,   46,  104,   44,
 /*  1400 */    72,   44,   46,   44,   43,   45,  105,   44,   46,  105,
 /*  1410 */    46,  105,   45,   44,   44,   44,   58,   44,   46,   43,
 /*  1420 */    46,   44,   43,  101,   44,   43,  118,   44,   43,  118,
 /*  1430 */    44,   43,   33,   44,   44,   44,   43,  118,   44,   38,
 /*  1440 */    75,    1,    0,
};
#define YY_SHIFT_USE_DFLT (1443)
#define YY_SHIFT_COUNT    (419)
#define YY_SHIFT_MIN      (-90)
#define YY_SHIFT_MAX      (1442)
static const short yy_shift_ofst[] = {
 /*     0 */   269,  488,  571,  739,  739,  739,  739,  739,  832,  739,
 /*    10 */   739,  739,  739,  739,  739,  739,   87,   -5,   68,   68,
 /*    20 */   609,  739,  739,  739,  739,  739,  739,  739,  739,  739,
 /*    30 */   739,  739,  739,  739,  739,  739,  739,  739,  739,  739,
 /*    40 */   739,  739,  739,  739,  739,  739,  739,  739,  739,  739,
 /*    50 */   739,  739,  739,  739,  739,  739,  739,  739,  739,  739,
 /*    60 */   474,  474,  577,  443,  733,  124,   94,  121,  177,  222,
 /*    70 */   267,  312,  357,  383,  409,  435,  435,  435,  435,  435,
 /*    80 */   435,  435,  435,  435,  435,  435,  435,  435,  435,  435,
 /*    90 */   464,  435,  559,  727,  727,  806,  823,  823,  823,  188,
 /*   100 */    19,    8,  622,  622,  478,  529,  591,   33, 1443, 1443,
 /*   110 */  1443,  -90,  -90,  587,  587,  485,  485, 1034,  217,  217,
 /*   120 */    24,  591,  591,  591,  804,  591,  591,  591,  591,  591,
 /*   130 */   591,  591,  591,  591,  591,  591,  591,  591,  591,  591,
 /*   140 */   591,  591,  591,  591,  529,  -51,  -51,  -51,  -51,  -51,
 /*   150 */   -51, 1443, 1443,  481,  184,  184,   84,  710,  710,  710,
 /*   160 */   458,  304,  658,  860,  623,  270,  449,  265,  274,  492,
 /*   170 */   738,  738,  738,  741,  574,  765,  573,  621,  591,  591,
 /*   180 */   591,  591,  591,  591,  591,  112, 1017, 1017,  591,  591,
 /*   190 */  1073,  112,  591, 1073,  591,  591,  591,  591,  591,  591,
 /*   200 */   839,  591,  992,   32,  591,  802,  591,  591, 1017,  591,
 /*   210 */   960,  802,  802,  591,  591, 1040,  591,  919, 1040,  591,
 /*   220 */  1089,  591,  591,  591, 1208, 1209, 1250, 1142, 1217, 1217,
 /*   230 */  1217, 1217, 1136, 1129, 1223, 1142, 1209, 1250, 1250, 1223,
 /*   240 */  1208, 1223, 1223, 1208, 1196, 1219, 1208, 1203, 1219, 1196,
 /*   250 */  1196, 1182, 1210, 1182, 1210, 1182, 1210, 1182, 1210, 1246,
 /*   260 */  1167, 1208, 1264, 1264, 1208, 1253, 1254, 1186, 1192, 1189,
 /*   270 */  1191, 1142, 1258, 1269, 1269, 1443, 1443, 1443, 1443, 1443,
 /*   280 */   665,   50,  446,  384, 1123, 1059, 1062,  914, 1135, 1139,
 /*   290 */  1140, 1143, 1147, 1148, 1007,  994,  877, 1054, 1151, 1152,
 /*   300 */  1114, 1155, 1158,  234, 1164, 1127, 1312, 1315, 1248, 1252,
 /*   310 */  1289, 1270, 1242, 1262, 1298, 1299, 1293, 1295, 1239, 1302,
 /*   320 */  1243, 1228, 1257, 1244, 1266, 1307, 1220, 1305, 1221, 1222,
 /*   330 */  1224, 1273, 1354, 1259, 1241, 1345, 1346, 1348, 1349, 1276,
 /*   340 */  1317, 1261, 1267, 1366, 1240, 1326, 1290, 1245, 1327, 1332,
 /*   350 */  1368, 1287, 1300, 1334, 1314, 1337, 1338, 1339, 1341, 1318,
 /*   360 */  1335, 1343, 1323, 1333, 1344, 1347, 1350, 1351, 1297, 1352,
 /*   370 */  1355, 1353, 1356, 1292, 1357, 1359, 1342, 1301, 1361, 1294,
 /*   380 */  1362, 1304, 1364, 1306, 1363, 1369, 1370, 1362, 1371, 1328,
 /*   390 */  1372, 1373, 1376, 1358, 1377, 1379, 1360, 1374, 1380, 1382,
 /*   400 */  1367, 1374, 1383, 1385, 1386, 1388, 1389, 1308, 1311, 1390,
 /*   410 */  1319, 1391, 1393, 1365, 1322, 1394, 1401, 1399, 1440, 1442,
};
#define YY_REDUCE_USE_DFLT (-158)
#define YY_REDUCE_COUNT (279)
#define YY_REDUCE_MIN   (-157)
#define YY_REDUCE_MAX   (1157)
static const short yy_reduce_ofst[] = {
 /*     0 */   -82,  726,  728,  480,  554,  724,  730,  732,  104,  381,
 /*    10 */   476,  734,  757,  771,  810,   -1,  479,  -42, -157,   82,
 /*    20 */   546,  551,  634,  650,  787,  791,  815,  817,  819,  826,
 /*    30 */   833,  836,  840,  844,  847,  850,  854,  858,  864,  872,
 /*    40 */   889,  893,  903,  921,  925,  927,  930,  932,  946,  948,
 /*    50 */   951,  956,  962,  970,  980,  986,  988,  990,  995, 1004,
 /*    60 */   113,  391,  612,  -40,   92,  105,  -49,  -49,  -49,  -49,
 /*    70 */   -49,  -49,  -49,  -49,  -49,  -49,  -49,  -49,  -49,  -49,
 /*    80 */   -49,  -49,  -49,  -49,  -49,  -49,  -49,  -49,  -49,  -49,
 /*    90 */   -49,  -49,  -49,  -49,  -49,  -49,  -49,  -49,  -49,  -49,
 /*   100 */   -49,  -49, -146,  131,  -49,  324,  194,  -49,  -49,  -49,
 /*   110 */   -49,  427,  533,  600,  600,  685,  687,  676,  552,  648,
 /*   120 */   272,  721,  723,  750,  -38,  760,  762,  767,  834,  843,
 /*   130 */   861,  875,  387,  891,  561,  756,  641,  887,  892,  643,
 /*   140 */   731,  894,  373,  895,  153,  786,  835,  881,  928,  934,
 /*   150 */   943,  570,  326,   28,   52,  180,  220,  318,  322,  422,
 /*   160 */   348,  472,  415,  483,  512,  536,  489,  639,  630,  664,
 /*   170 */   629,  713,  766,  825,  348,  841,  868,  846,  920,  949,
 /*   180 */   977,  997, 1018, 1032, 1047, 1006, 1001, 1002, 1051, 1053,
 /*   190 */   975, 1019, 1061,  978, 1064, 1065, 1066, 1067, 1069, 1070,
 /*   200 */  1055, 1074, 1048, 1056, 1075, 1063, 1077, 1079, 1024, 1081,
 /*   210 */  1011, 1068, 1071, 1084, 1085, 1049,  664, 1045, 1050, 1088,
 /*   220 */  1090, 1091, 1093, 1094, 1080, 1035, 1072, 1058, 1076, 1078,
 /*   230 */  1082, 1083, 1022, 1026, 1105, 1086, 1052, 1087, 1092, 1107,
 /*   240 */  1096, 1110, 1115, 1103, 1108, 1102, 1109, 1112, 1111, 1116,
 /*   250 */  1117, 1095, 1097, 1098, 1100, 1101, 1104, 1106, 1113, 1099,
 /*   260 */  1118, 1120, 1119, 1121, 1122, 1057, 1060, 1124, 1126, 1128,
 /*   270 */  1125, 1131, 1144, 1153, 1156, 1133, 1149, 1145, 1150, 1157,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */  1197, 1178, 1178, 1123, 1123, 1123, 1123, 1123, 1178, 1236,
 /*    10 */  1236, 1236, 1236, 1236, 1236, 1122, 1178, 1019, 1046, 1046,
 /*    20 */  1236, 1236, 1236, 1236, 1236, 1236, 1236, 1236, 1236, 1236,
 /*    30 */  1236, 1236, 1236, 1236, 1236, 1236, 1236, 1236, 1236, 1236,
 /*    40 */  1236, 1236, 1236, 1236, 1236, 1236, 1236, 1236, 1236, 1236,
 /*    50 */  1236, 1236, 1236, 1236, 1236, 1236, 1236, 1236, 1236, 1236,
 /*    60 */  1236, 1236, 1236, 1236, 1178, 1023, 1052, 1236, 1236, 1236,
 /*    70 */  1124, 1125, 1236, 1236, 1236, 1157, 1063, 1062, 1061, 1060,
 /*    80 */  1033, 1058, 1050, 1054, 1124, 1118, 1119, 1117, 1121, 1125,
 /*    90 */  1236, 1053, 1089, 1102, 1088, 1098, 1108, 1099, 1091, 1090,
 /*   100 */  1092, 1093,  990,  990, 1094, 1236, 1236, 1095, 1105, 1104,
 /*   110 */  1103, 1236, 1236, 1203, 1202, 1236, 1236, 1130, 1236, 1236,
 /*   120 */  1236, 1236, 1236, 1236, 1178, 1236, 1236, 1236, 1236, 1236,
 /*   130 */  1236, 1236, 1236, 1236, 1236, 1236, 1236, 1236, 1236, 1236,
 /*   140 */  1236, 1236, 1236, 1236, 1236, 1178, 1178, 1178, 1178, 1178,
 /*   150 */  1178, 1023, 1014, 1236, 1236, 1236, 1236, 1236, 1236, 1236,
 /*   160 */  1236, 1019, 1236, 1236, 1236, 1236, 1152, 1236, 1236, 1236,
 /*   170 */  1019, 1019, 1019, 1236, 1021, 1236, 1003, 1013, 1236, 1236,
 /*   180 */  1236, 1236, 1173, 1236, 1144, 1057, 1035, 1035, 1236, 1236,
 /*   190 */  1234, 1057, 1236, 1234, 1236, 1236, 1236, 1236, 1236, 1236,
 /*   200 */   963, 1236, 1210,  960, 1236, 1046, 1236, 1236, 1035, 1236,
 /*   210 */  1120, 1046, 1046, 1236, 1236, 1013, 1236, 1020, 1013, 1236,
 /*   220 */  1236, 1236, 1236, 1236, 1131, 1068,  993, 1057,  999,  999,
 /*   230 */   999,  999, 1156, 1231,  944, 1057, 1068,  993,  993,  944,
 /*   240 */  1131,  944,  944, 1131,  991,  981, 1131,  963,  981,  991,
 /*   250 */   991, 1039, 1034, 1039, 1034, 1039, 1034, 1039, 1034, 1126,
 /*   260 */  1236, 1131, 1135, 1135, 1131, 1193, 1236, 1051, 1040, 1049,
 /*   270 */  1047, 1057, 1206, 1200, 1200,  965, 1205,  965,  965, 1205,
 /*   280 */  1236, 1236, 1236, 1236, 1236, 1236, 1236, 1138, 1236, 1236,
 /*   290 */  1236, 1236, 1236, 1236, 1236, 1236, 1236, 1236, 1236, 1236,
 /*   300 */  1236, 1236, 1236, 1236, 1236, 1074, 1236,  934, 1236, 1236,
 /*   310 */  1236, 1236, 1236, 1236, 1236, 1236, 1236, 1236, 1226, 1236,
 /*   320 */  1236, 1236, 1236, 1236, 1236, 1236, 1236, 1155, 1154, 1236,
 /*   330 */  1236, 1236, 1236, 1236, 1236, 1236, 1236, 1236, 1236, 1236,
 /*   340 */  1236, 1236, 1233, 1236, 1236, 1236, 1236, 1236, 1236, 1236,
 /*   350 */  1236, 1236, 1236, 1236, 1236, 1236, 1236, 1236, 1236, 1236,
 /*   360 */  1236, 1236, 1236, 1236, 1236, 1236, 1236, 1236, 1005, 1236,
 /*   370 */  1236, 1236, 1214, 1236, 1236, 1236, 1236, 1236, 1236, 1236,
 /*   380 */  1048, 1236, 1041, 1236, 1236, 1236, 1236, 1223, 1236, 1236,
 /*   390 */  1236, 1236, 1236, 1236, 1236, 1236, 1236, 1180, 1236, 1236,
 /*   400 */  1236, 1179, 1236, 1236, 1236, 1236, 1236, 1236, 1236, 1236,
 /*   410 */  1236, 1236, 1236, 1236, 1236, 1236,  938, 1236, 1236, 1236,
};
/********** End of lemon-generated parsing tables *****************************/

/* The next table maps tokens (terminal symbols) into fallback tokens.  
** If a construct like the following:
** 
**      %fallback ID X Y Z.
**
** appears in the grammar, then ID becomes a fallback token for X, Y,
** and Z.  Whenever one of the tokens X, Y, or Z is input to the parser
** but it does not parse, the type of the token is changed to ID and
** the parse is retried before an error is thrown.
**
** This feature can be used, for example, to cause some keywords in a language
** to revert to identifiers if they keyword does not apply in the context where
** it appears.
*/
#ifdef YYFALLBACK
static const YYCODETYPE yyFallback[] = {
    0,  /*          $ => nothing */
    0,  /*       SEMI => nothing */
    0,  /*    EXPLAIN => nothing */
   47,  /*      QUERY => ID */
   47,  /*       PLAN => ID */
    0,  /*         OR => nothing */
    0,  /*        AND => nothing */
    0,  /*        NOT => nothing */
    0,  /*         IS => nothing */
   47,  /*      MATCH => ID */
    0,  /*    LIKE_KW => nothing */
    0,  /*    BETWEEN => nothing */
    0,  /*         IN => nothing */
    0,  /*         NE => nothing */
    0,  /*         EQ => nothing */
    0,  /*         GT => nothing */
    0,  /*         LE => nothing */
    0,  /*         LT => nothing */
    0,  /*         GE => nothing */
    0,  /*     ESCAPE => nothing */
    0,  /*     BITAND => nothing */
    0,  /*      BITOR => nothing */
    0,  /*     LSHIFT => nothing */
    0,  /*     RSHIFT => nothing */
    0,  /*       PLUS => nothing */
    0,  /*      MINUS => nothing */
    0,  /*       STAR => nothing */
    0,  /*      SLASH => nothing */
    0,  /*        REM => nothing */
    0,  /*     CONCAT => nothing */
    0,  /*    COLLATE => nothing */
    0,  /*     BITNOT => nothing */
    0,  /*      START => nothing */
    0,  /* TRANSACTION => nothing */
    0,  /*     COMMIT => nothing */
    0,  /*   ROLLBACK => nothing */
    0,  /*  SAVEPOINT => nothing */
   47,  /*    RELEASE => ID */
    0,  /*         TO => nothing */
    0,  /*      TABLE => nothing */
    0,  /*     CREATE => nothing */
   47,  /*         IF => ID */
    0,  /*     EXISTS => nothing */
    0,  /*         LP => nothing */
    0,  /*         RP => nothing */
    0,  /*         AS => nothing */
    0,  /*      COMMA => nothing */
    0,  /*         ID => nothing */
    0,  /*    INDEXED => nothing */
   47,  /*      ABORT => ID */
   47,  /*     ACTION => ID */
   47,  /*        ADD => ID */
   47,  /*      AFTER => ID */
   47,  /* AUTOINCREMENT => ID */
   47,  /*     BEFORE => ID */
   47,  /*    CASCADE => ID */
   47,  /*   CONFLICT => ID */
   47,  /*   DEFERRED => ID */
   47,  /*        END => ID */
   47,  /*       FAIL => ID */
   47,  /*     IGNORE => ID */
   47,  /*  INITIALLY => ID */
   47,  /*    INSTEAD => ID */
   47,  /*         NO => ID */
   47,  /*        KEY => ID */
   47,  /*     OFFSET => ID */
   47,  /*      RAISE => ID */
   47,  /*    REPLACE => ID */
   47,  /*   RESTRICT => ID */
   47,  /*     RENAME => ID */
   47,  /*   CTIME_KW => ID */
};
#endif /* YYFALLBACK */

/* The following structure represents a single element of the
** parser's stack.  Information stored includes:
**
**   +  The state number for the parser at this level of the stack.
**
**   +  The value of the token stored at this level of the stack.
**      (In other words, the "major" token.)
**
**   +  The semantic value stored at this level of the stack.  This is
**      the information used by the action routines in the grammar.
**      It is sometimes called the "minor" token.
**
** After the "shift" half of a SHIFTREDUCE action, the stateno field
** actually contains the reduce action for the second half of the
** SHIFTREDUCE.
*/
struct yyStackEntry {
  YYACTIONTYPE stateno;  /* The state-number, or reduce action in SHIFTREDUCE */
  YYCODETYPE major;      /* The major token value.  This is the code
                         ** number for the token at this stack level */
  YYMINORTYPE minor;     /* The user-supplied minor token value.  This
                         ** is the value of the token  */
};
typedef struct yyStackEntry yyStackEntry;

/* The state of the parser is completely contained in an instance of
** the following structure */
struct yyParser {
  yyStackEntry *yytos;          /* Pointer to top element of the stack */
#ifdef YYTRACKMAXSTACKDEPTH
  int yyhwm;                    /* High-water mark of the stack */
#endif
#ifndef YYNOERRORRECOVERY
  int yyerrcnt;                 /* Shifts left before out of the error */
#endif
  bool is_fallback_failed;      /* Shows if fallback failed or not */
  sqlite3ParserARG_SDECL                /* A place to hold %extra_argument */
#if YYSTACKDEPTH<=0
  int yystksz;                  /* Current side of the stack */
  yyStackEntry *yystack;        /* The parser's stack */
  yyStackEntry yystk0;          /* First stack entry */
#else
  yyStackEntry yystack[YYSTACKDEPTH];  /* The parser's stack */
#endif
};
typedef struct yyParser yyParser;

#ifndef NDEBUG
#include <stdio.h>
static FILE *yyTraceFILE = 0;
static char *yyTracePrompt = 0;
#endif /* NDEBUG */

#ifndef NDEBUG
/* 
** Turn parser tracing on by giving a stream to which to write the trace
** and a prompt to preface each trace message.  Tracing is turned off
** by making either argument NULL 
**
** Inputs:
** <ul>
** <li> A FILE* to which trace output should be written.
**      If NULL, then tracing is turned off.
** <li> A prefix string written at the beginning of every
**      line of trace output.  If NULL, then tracing is
**      turned off.
** </ul>
**
** Outputs:
** None.
*/
void sqlite3ParserTrace(FILE *TraceFILE, char *zTracePrompt){
  yyTraceFILE = TraceFILE;
  yyTracePrompt = zTracePrompt;
  if( yyTraceFILE==0 ) yyTracePrompt = 0;
  else if( yyTracePrompt==0 ) yyTraceFILE = 0;
}
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing shifts, the names of all terminals and nonterminals
** are required.  The following table supplies these names */
static const char *const yyTokenName[] = { 
  "$",             "SEMI",          "EXPLAIN",       "QUERY",       
  "PLAN",          "OR",            "AND",           "NOT",         
  "IS",            "MATCH",         "LIKE_KW",       "BETWEEN",     
  "IN",            "NE",            "EQ",            "GT",          
  "LE",            "LT",            "GE",            "ESCAPE",      
  "BITAND",        "BITOR",         "LSHIFT",        "RSHIFT",      
  "PLUS",          "MINUS",         "STAR",          "SLASH",       
  "REM",           "CONCAT",        "COLLATE",       "BITNOT",      
  "START",         "TRANSACTION",   "COMMIT",        "ROLLBACK",    
  "SAVEPOINT",     "RELEASE",       "TO",            "TABLE",       
  "CREATE",        "IF",            "EXISTS",        "LP",          
  "RP",            "AS",            "COMMA",         "ID",          
  "INDEXED",       "ABORT",         "ACTION",        "ADD",         
  "AFTER",         "AUTOINCREMENT",  "BEFORE",        "CASCADE",     
  "CONFLICT",      "DEFERRED",      "END",           "FAIL",        
  "IGNORE",        "INITIALLY",     "INSTEAD",       "NO",          
  "KEY",           "OFFSET",        "RAISE",         "REPLACE",     
  "RESTRICT",      "RENAME",        "CTIME_KW",      "ANY",         
  "STRING",        "CONSTRAINT",    "DEFAULT",       "NULL",        
  "PRIMARY",       "UNIQUE",        "CHECK",         "REFERENCES",  
  "AUTOINCR",      "ON",            "INSERT",        "DELETE",      
  "UPDATE",        "SIMPLE",        "PARTIAL",       "FULL",        
  "SET",           "DEFERRABLE",    "IMMEDIATE",     "FOREIGN",     
  "DROP",          "VIEW",          "UNION",         "ALL",         
  "EXCEPT",        "INTERSECT",     "SELECT",        "VALUES",      
  "DISTINCT",      "DOT",           "FROM",          "JOIN_KW",     
  "JOIN",          "BY",            "USING",         "ORDER",       
  "ASC",           "DESC",          "GROUP",         "HAVING",      
  "LIMIT",         "TRUNCATE",      "WHERE",         "INTO",        
  "FLOAT",         "BLOB",          "INTEGER",       "VARIABLE",    
  "CAST",          "DATE",          "DATETIME",      "CHAR",        
  "CASE",          "WHEN",          "THEN",          "ELSE",        
  "INDEX",         "PRAGMA",        "BEGIN",         "TRIGGER",     
  "OF",            "FOR",           "EACH",          "ROW",         
  "ANALYZE",       "ALTER",         "WITH",          "RECURSIVE",   
  "TEXT",          "TIME",          "VARCHAR",       "REAL",        
  "DOUBLE",        "INT",           "DECIMAL",       "NUMERIC",     
  "NUM",           "error",         "input",         "ecmd",        
  "explain",       "cmdx",          "cmd",           "savepoint_opt",
  "nm",            "create_table",  "create_table_args",  "createkw",    
  "ifnotexists",   "columnlist",    "conslist_opt",  "select",      
  "columnname",    "carglist",      "typedef",       "ccons",       
  "term",          "expr",          "onconf",        "sortorder",   
  "autoinc",       "eidlist_opt",   "refargs",       "defer_subclause",
  "refarg",        "matcharg",      "refact",        "init_deferred_pred_opt",
  "conslist",      "tconscomma",    "tcons",         "sortlist",    
  "eidlist",       "defer_subclause_opt",  "index_onconf",  "orconf",      
  "resolvetype",   "raisetype",     "ifexists",      "fullname",    
  "selectnowith",  "oneselect",     "with",          "multiselect_op",
  "distinct",      "selcollist",    "from",          "where_opt",   
  "groupby_opt",   "having_opt",    "orderby_opt",   "limit_opt",   
  "values",        "nexprlist",     "exprlist",      "sclp",        
  "as",            "seltablist",    "stl_prefix",    "joinop",      
  "indexed_opt",   "on_opt",        "using_opt",     "join_nm",     
  "idlist",        "setlist",       "insert_cmd",    "idlist_opt",  
  "type_func",     "likeop",        "between_op",    "in_op",       
  "paren_exprlist",  "case_operand",  "case_exprlist",  "case_else",   
  "uniqueflag",    "collate",       "nmnum",         "minus_num",   
  "plus_num",      "trigger_decl",  "trigger_cmd_list",  "trigger_time",
  "trigger_event",  "foreach_clause",  "when_clause",   "trigger_cmd", 
  "trnm",          "tridxby",       "wqlist",        "char_len_typedef",
  "number_typedef",  "number_len_typedef",
};
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing reduce actions, the names of all rules are required.
*/
static const char *const yyRuleName[] = {
 /*   0 */ "ecmd ::= explain cmdx SEMI",
 /*   1 */ "ecmd ::= SEMI",
 /*   2 */ "explain ::= EXPLAIN",
 /*   3 */ "explain ::= EXPLAIN QUERY PLAN",
 /*   4 */ "cmd ::= START TRANSACTION",
 /*   5 */ "cmd ::= COMMIT",
 /*   6 */ "cmd ::= ROLLBACK",
 /*   7 */ "cmd ::= SAVEPOINT nm",
 /*   8 */ "cmd ::= RELEASE savepoint_opt nm",
 /*   9 */ "cmd ::= ROLLBACK TO savepoint_opt nm",
 /*  10 */ "create_table ::= createkw TABLE ifnotexists nm",
 /*  11 */ "createkw ::= CREATE",
 /*  12 */ "ifnotexists ::=",
 /*  13 */ "ifnotexists ::= IF NOT EXISTS",
 /*  14 */ "create_table_args ::= LP columnlist conslist_opt RP",
 /*  15 */ "create_table_args ::= AS select",
 /*  16 */ "columnname ::= nm typedef",
 /*  17 */ "nm ::= ID|INDEXED",
 /*  18 */ "ccons ::= CONSTRAINT nm",
 /*  19 */ "ccons ::= DEFAULT term",
 /*  20 */ "ccons ::= DEFAULT LP expr RP",
 /*  21 */ "ccons ::= DEFAULT PLUS term",
 /*  22 */ "ccons ::= DEFAULT MINUS term",
 /*  23 */ "ccons ::= NULL onconf",
 /*  24 */ "ccons ::= NOT NULL onconf",
 /*  25 */ "ccons ::= PRIMARY KEY sortorder autoinc",
 /*  26 */ "ccons ::= UNIQUE",
 /*  27 */ "ccons ::= CHECK LP expr RP",
 /*  28 */ "ccons ::= REFERENCES nm eidlist_opt refargs",
 /*  29 */ "ccons ::= defer_subclause",
 /*  30 */ "ccons ::= COLLATE ID|INDEXED",
 /*  31 */ "autoinc ::=",
 /*  32 */ "autoinc ::= AUTOINCR",
 /*  33 */ "refargs ::=",
 /*  34 */ "refargs ::= refargs refarg",
 /*  35 */ "refarg ::= MATCH matcharg",
 /*  36 */ "refarg ::= ON INSERT refact",
 /*  37 */ "refarg ::= ON DELETE refact",
 /*  38 */ "refarg ::= ON UPDATE refact",
 /*  39 */ "matcharg ::= SIMPLE",
 /*  40 */ "matcharg ::= PARTIAL",
 /*  41 */ "matcharg ::= FULL",
 /*  42 */ "refact ::= SET NULL",
 /*  43 */ "refact ::= SET DEFAULT",
 /*  44 */ "refact ::= CASCADE",
 /*  45 */ "refact ::= RESTRICT",
 /*  46 */ "refact ::= NO ACTION",
 /*  47 */ "defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt",
 /*  48 */ "defer_subclause ::= DEFERRABLE init_deferred_pred_opt",
 /*  49 */ "init_deferred_pred_opt ::=",
 /*  50 */ "init_deferred_pred_opt ::= INITIALLY DEFERRED",
 /*  51 */ "init_deferred_pred_opt ::= INITIALLY IMMEDIATE",
 /*  52 */ "tconscomma ::= COMMA",
 /*  53 */ "tcons ::= CONSTRAINT nm",
 /*  54 */ "tcons ::= PRIMARY KEY LP sortlist autoinc RP",
 /*  55 */ "tcons ::= UNIQUE LP sortlist RP",
 /*  56 */ "tcons ::= CHECK LP expr RP onconf",
 /*  57 */ "tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt refargs defer_subclause_opt",
 /*  58 */ "defer_subclause_opt ::=",
 /*  59 */ "onconf ::=",
 /*  60 */ "onconf ::= ON CONFLICT resolvetype",
 /*  61 */ "orconf ::=",
 /*  62 */ "orconf ::= OR resolvetype",
 /*  63 */ "resolvetype ::= IGNORE",
 /*  64 */ "resolvetype ::= REPLACE",
 /*  65 */ "cmd ::= DROP TABLE ifexists fullname",
 /*  66 */ "ifexists ::= IF EXISTS",
 /*  67 */ "ifexists ::=",
 /*  68 */ "cmd ::= createkw VIEW ifnotexists nm eidlist_opt AS select",
 /*  69 */ "cmd ::= DROP VIEW ifexists fullname",
 /*  70 */ "cmd ::= select",
 /*  71 */ "select ::= with selectnowith",
 /*  72 */ "selectnowith ::= selectnowith multiselect_op oneselect",
 /*  73 */ "multiselect_op ::= UNION",
 /*  74 */ "multiselect_op ::= UNION ALL",
 /*  75 */ "multiselect_op ::= EXCEPT|INTERSECT",
 /*  76 */ "oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt orderby_opt limit_opt",
 /*  77 */ "values ::= VALUES LP nexprlist RP",
 /*  78 */ "values ::= values COMMA LP exprlist RP",
 /*  79 */ "distinct ::= DISTINCT",
 /*  80 */ "distinct ::= ALL",
 /*  81 */ "distinct ::=",
 /*  82 */ "sclp ::=",
 /*  83 */ "selcollist ::= sclp expr as",
 /*  84 */ "selcollist ::= sclp STAR",
 /*  85 */ "selcollist ::= sclp nm DOT STAR",
 /*  86 */ "as ::= AS nm",
 /*  87 */ "as ::=",
 /*  88 */ "from ::=",
 /*  89 */ "from ::= FROM seltablist",
 /*  90 */ "stl_prefix ::= seltablist joinop",
 /*  91 */ "stl_prefix ::=",
 /*  92 */ "seltablist ::= stl_prefix nm as indexed_opt on_opt using_opt",
 /*  93 */ "seltablist ::= stl_prefix nm LP exprlist RP as on_opt using_opt",
 /*  94 */ "seltablist ::= stl_prefix LP select RP as on_opt using_opt",
 /*  95 */ "seltablist ::= stl_prefix LP seltablist RP as on_opt using_opt",
 /*  96 */ "fullname ::= nm",
 /*  97 */ "joinop ::= COMMA|JOIN",
 /*  98 */ "joinop ::= JOIN_KW JOIN",
 /*  99 */ "joinop ::= JOIN_KW join_nm JOIN",
 /* 100 */ "joinop ::= JOIN_KW join_nm join_nm JOIN",
 /* 101 */ "on_opt ::= ON expr",
 /* 102 */ "on_opt ::=",
 /* 103 */ "indexed_opt ::=",
 /* 104 */ "indexed_opt ::= INDEXED BY nm",
 /* 105 */ "indexed_opt ::= NOT INDEXED",
 /* 106 */ "using_opt ::= USING LP idlist RP",
 /* 107 */ "using_opt ::=",
 /* 108 */ "orderby_opt ::=",
 /* 109 */ "orderby_opt ::= ORDER BY sortlist",
 /* 110 */ "sortlist ::= sortlist COMMA expr sortorder",
 /* 111 */ "sortlist ::= expr sortorder",
 /* 112 */ "sortorder ::= ASC",
 /* 113 */ "sortorder ::= DESC",
 /* 114 */ "sortorder ::=",
 /* 115 */ "groupby_opt ::=",
 /* 116 */ "groupby_opt ::= GROUP BY nexprlist",
 /* 117 */ "having_opt ::=",
 /* 118 */ "having_opt ::= HAVING expr",
 /* 119 */ "limit_opt ::=",
 /* 120 */ "limit_opt ::= LIMIT expr",
 /* 121 */ "limit_opt ::= LIMIT expr OFFSET expr",
 /* 122 */ "limit_opt ::= LIMIT expr COMMA expr",
 /* 123 */ "cmd ::= with DELETE FROM fullname indexed_opt where_opt",
 /* 124 */ "cmd ::= TRUNCATE TABLE fullname",
 /* 125 */ "where_opt ::=",
 /* 126 */ "where_opt ::= WHERE expr",
 /* 127 */ "cmd ::= with UPDATE orconf fullname indexed_opt SET setlist where_opt",
 /* 128 */ "setlist ::= setlist COMMA nm EQ expr",
 /* 129 */ "setlist ::= setlist COMMA LP idlist RP EQ expr",
 /* 130 */ "setlist ::= nm EQ expr",
 /* 131 */ "setlist ::= LP idlist RP EQ expr",
 /* 132 */ "cmd ::= with insert_cmd INTO fullname idlist_opt select",
 /* 133 */ "cmd ::= with insert_cmd INTO fullname idlist_opt DEFAULT VALUES",
 /* 134 */ "insert_cmd ::= INSERT orconf",
 /* 135 */ "insert_cmd ::= REPLACE",
 /* 136 */ "idlist_opt ::=",
 /* 137 */ "idlist_opt ::= LP idlist RP",
 /* 138 */ "idlist ::= idlist COMMA nm",
 /* 139 */ "idlist ::= nm",
 /* 140 */ "expr ::= LP expr RP",
 /* 141 */ "term ::= NULL",
 /* 142 */ "expr ::= ID|INDEXED",
 /* 143 */ "expr ::= JOIN_KW",
 /* 144 */ "expr ::= nm DOT nm",
 /* 145 */ "term ::= FLOAT|BLOB",
 /* 146 */ "term ::= STRING",
 /* 147 */ "term ::= INTEGER",
 /* 148 */ "expr ::= VARIABLE",
 /* 149 */ "expr ::= expr COLLATE ID|INDEXED",
 /* 150 */ "expr ::= CAST LP expr AS typedef RP",
 /* 151 */ "expr ::= ID|INDEXED LP distinct exprlist RP",
 /* 152 */ "expr ::= type_func LP distinct exprlist RP",
 /* 153 */ "expr ::= ID|INDEXED LP STAR RP",
 /* 154 */ "term ::= CTIME_KW",
 /* 155 */ "expr ::= LP nexprlist COMMA expr RP",
 /* 156 */ "expr ::= expr AND expr",
 /* 157 */ "expr ::= expr OR expr",
 /* 158 */ "expr ::= expr LT|GT|GE|LE expr",
 /* 159 */ "expr ::= expr EQ|NE expr",
 /* 160 */ "expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr",
 /* 161 */ "expr ::= expr PLUS|MINUS expr",
 /* 162 */ "expr ::= expr STAR|SLASH|REM expr",
 /* 163 */ "expr ::= expr CONCAT expr",
 /* 164 */ "likeop ::= LIKE_KW|MATCH",
 /* 165 */ "likeop ::= NOT LIKE_KW|MATCH",
 /* 166 */ "expr ::= expr likeop expr",
 /* 167 */ "expr ::= expr likeop expr ESCAPE expr",
 /* 168 */ "expr ::= expr IS NULL",
 /* 169 */ "expr ::= expr IS NOT NULL",
 /* 170 */ "expr ::= NOT expr",
 /* 171 */ "expr ::= BITNOT expr",
 /* 172 */ "expr ::= MINUS expr",
 /* 173 */ "expr ::= PLUS expr",
 /* 174 */ "between_op ::= BETWEEN",
 /* 175 */ "between_op ::= NOT BETWEEN",
 /* 176 */ "expr ::= expr between_op expr AND expr",
 /* 177 */ "in_op ::= IN",
 /* 178 */ "in_op ::= NOT IN",
 /* 179 */ "expr ::= expr in_op LP exprlist RP",
 /* 180 */ "expr ::= LP select RP",
 /* 181 */ "expr ::= expr in_op LP select RP",
 /* 182 */ "expr ::= expr in_op nm paren_exprlist",
 /* 183 */ "expr ::= EXISTS LP select RP",
 /* 184 */ "expr ::= CASE case_operand case_exprlist case_else END",
 /* 185 */ "case_exprlist ::= case_exprlist WHEN expr THEN expr",
 /* 186 */ "case_exprlist ::= WHEN expr THEN expr",
 /* 187 */ "case_else ::= ELSE expr",
 /* 188 */ "case_else ::=",
 /* 189 */ "case_operand ::= expr",
 /* 190 */ "case_operand ::=",
 /* 191 */ "exprlist ::=",
 /* 192 */ "nexprlist ::= nexprlist COMMA expr",
 /* 193 */ "nexprlist ::= expr",
 /* 194 */ "paren_exprlist ::=",
 /* 195 */ "paren_exprlist ::= LP exprlist RP",
 /* 196 */ "cmd ::= createkw uniqueflag INDEX ifnotexists nm ON nm LP sortlist RP",
 /* 197 */ "uniqueflag ::= UNIQUE",
 /* 198 */ "uniqueflag ::=",
 /* 199 */ "eidlist_opt ::=",
 /* 200 */ "eidlist_opt ::= LP eidlist RP",
 /* 201 */ "eidlist ::= eidlist COMMA nm collate sortorder",
 /* 202 */ "eidlist ::= nm collate sortorder",
 /* 203 */ "collate ::=",
 /* 204 */ "collate ::= COLLATE ID|INDEXED",
 /* 205 */ "cmd ::= DROP INDEX ifexists fullname ON nm",
 /* 206 */ "cmd ::= PRAGMA nm",
 /* 207 */ "cmd ::= PRAGMA nm EQ nmnum",
 /* 208 */ "cmd ::= PRAGMA nm LP nmnum RP",
 /* 209 */ "cmd ::= PRAGMA nm EQ minus_num",
 /* 210 */ "cmd ::= PRAGMA nm LP minus_num RP",
 /* 211 */ "cmd ::= PRAGMA nm EQ nm DOT nm",
 /* 212 */ "cmd ::= PRAGMA",
 /* 213 */ "plus_num ::= PLUS INTEGER|FLOAT",
 /* 214 */ "minus_num ::= MINUS INTEGER|FLOAT",
 /* 215 */ "cmd ::= createkw trigger_decl BEGIN trigger_cmd_list END",
 /* 216 */ "trigger_decl ::= TRIGGER ifnotexists nm trigger_time trigger_event ON fullname foreach_clause when_clause",
 /* 217 */ "trigger_time ::= BEFORE",
 /* 218 */ "trigger_time ::= AFTER",
 /* 219 */ "trigger_time ::= INSTEAD OF",
 /* 220 */ "trigger_time ::=",
 /* 221 */ "trigger_event ::= DELETE|INSERT",
 /* 222 */ "trigger_event ::= UPDATE",
 /* 223 */ "trigger_event ::= UPDATE OF idlist",
 /* 224 */ "when_clause ::=",
 /* 225 */ "when_clause ::= WHEN expr",
 /* 226 */ "trigger_cmd_list ::= trigger_cmd_list trigger_cmd SEMI",
 /* 227 */ "trigger_cmd_list ::= trigger_cmd SEMI",
 /* 228 */ "trnm ::= nm DOT nm",
 /* 229 */ "tridxby ::= INDEXED BY nm",
 /* 230 */ "tridxby ::= NOT INDEXED",
 /* 231 */ "trigger_cmd ::= UPDATE orconf trnm tridxby SET setlist where_opt",
 /* 232 */ "trigger_cmd ::= insert_cmd INTO trnm idlist_opt select",
 /* 233 */ "trigger_cmd ::= DELETE FROM trnm tridxby where_opt",
 /* 234 */ "trigger_cmd ::= select",
 /* 235 */ "expr ::= RAISE LP IGNORE RP",
 /* 236 */ "expr ::= RAISE LP raisetype COMMA STRING RP",
 /* 237 */ "raisetype ::= ROLLBACK",
 /* 238 */ "raisetype ::= ABORT",
 /* 239 */ "raisetype ::= FAIL",
 /* 240 */ "cmd ::= DROP TRIGGER ifexists fullname",
 /* 241 */ "cmd ::= ANALYZE",
 /* 242 */ "cmd ::= ANALYZE nm",
 /* 243 */ "cmd ::= ALTER TABLE fullname RENAME TO nm",
 /* 244 */ "cmd ::= ALTER TABLE fullname ADD CONSTRAINT nm FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt refargs defer_subclause_opt",
 /* 245 */ "cmd ::= ALTER TABLE fullname DROP CONSTRAINT nm",
 /* 246 */ "with ::=",
 /* 247 */ "with ::= WITH wqlist",
 /* 248 */ "with ::= WITH RECURSIVE wqlist",
 /* 249 */ "wqlist ::= nm eidlist_opt AS LP select RP",
 /* 250 */ "wqlist ::= wqlist COMMA nm eidlist_opt AS LP select RP",
 /* 251 */ "typedef ::= TEXT",
 /* 252 */ "typedef ::= BLOB",
 /* 253 */ "typedef ::= DATE",
 /* 254 */ "typedef ::= TIME",
 /* 255 */ "typedef ::= DATETIME",
 /* 256 */ "typedef ::= CHAR|VARCHAR char_len_typedef",
 /* 257 */ "char_len_typedef ::= LP INTEGER RP",
 /* 258 */ "number_typedef ::= FLOAT|REAL|DOUBLE",
 /* 259 */ "number_typedef ::= INT|INTEGER",
 /* 260 */ "number_typedef ::= DECIMAL|NUMERIC|NUM number_len_typedef",
 /* 261 */ "number_len_typedef ::=",
 /* 262 */ "number_len_typedef ::= LP INTEGER RP",
 /* 263 */ "number_len_typedef ::= LP INTEGER COMMA INTEGER RP",
 /* 264 */ "input ::= ecmd",
 /* 265 */ "explain ::=",
 /* 266 */ "cmdx ::= cmd",
 /* 267 */ "savepoint_opt ::= SAVEPOINT",
 /* 268 */ "savepoint_opt ::=",
 /* 269 */ "cmd ::= create_table create_table_args",
 /* 270 */ "columnlist ::= columnlist COMMA columnname carglist",
 /* 271 */ "columnlist ::= columnname carglist",
 /* 272 */ "carglist ::= carglist ccons",
 /* 273 */ "carglist ::=",
 /* 274 */ "conslist_opt ::=",
 /* 275 */ "conslist_opt ::= COMMA conslist",
 /* 276 */ "conslist ::= conslist tconscomma tcons",
 /* 277 */ "conslist ::= tcons",
 /* 278 */ "tconscomma ::=",
 /* 279 */ "defer_subclause_opt ::= defer_subclause",
 /* 280 */ "resolvetype ::= raisetype",
 /* 281 */ "selectnowith ::= oneselect",
 /* 282 */ "oneselect ::= values",
 /* 283 */ "sclp ::= selcollist COMMA",
 /* 284 */ "as ::= ID|STRING",
 /* 285 */ "join_nm ::= ID|INDEXED",
 /* 286 */ "join_nm ::= JOIN_KW",
 /* 287 */ "expr ::= term",
 /* 288 */ "type_func ::= DATE",
 /* 289 */ "type_func ::= DATETIME",
 /* 290 */ "type_func ::= CHAR",
 /* 291 */ "exprlist ::= nexprlist",
 /* 292 */ "nmnum ::= plus_num",
 /* 293 */ "nmnum ::= STRING",
 /* 294 */ "nmnum ::= nm",
 /* 295 */ "nmnum ::= ON",
 /* 296 */ "nmnum ::= DELETE",
 /* 297 */ "nmnum ::= DEFAULT",
 /* 298 */ "plus_num ::= INTEGER|FLOAT",
 /* 299 */ "foreach_clause ::=",
 /* 300 */ "foreach_clause ::= FOR EACH ROW",
 /* 301 */ "trnm ::= nm",
 /* 302 */ "tridxby ::=",
 /* 303 */ "typedef ::= number_typedef",
};
#endif /* NDEBUG */


#if YYSTACKDEPTH<=0
/*
** Try to increase the size of the parser stack.  Return the number
** of errors.  Return 0 on success.
*/
static int yyGrowStack(yyParser *p){
  int newSize;
  int idx;
  yyStackEntry *pNew;

  newSize = p->yystksz*2 + 100;
  idx = p->yytos ? (int)(p->yytos - p->yystack) : 0;
  if( p->yystack==&p->yystk0 ){
    pNew = malloc(newSize*sizeof(pNew[0]));
    if( pNew ) pNew[0] = p->yystk0;
  }else{
    pNew = realloc(p->yystack, newSize*sizeof(pNew[0]));
  }
  if( pNew ){
    p->yystack = pNew;
    p->yytos = &p->yystack[idx];
#ifndef NDEBUG
    if( yyTraceFILE ){
      fprintf(yyTraceFILE,"%sStack grows from %d to %d entries.\n",
              yyTracePrompt, p->yystksz, newSize);
    }
#endif
    p->yystksz = newSize;
  }
  return pNew==0; 
}
#endif

/* Datatype of the argument to the memory allocated passed as the
** second argument to sqlite3ParserAlloc() below.  This can be changed by
** putting an appropriate #define in the %include section of the input
** grammar.
*/
#ifndef YYMALLOCARGTYPE
# define YYMALLOCARGTYPE size_t
#endif

/* 
** This function allocates a new parser.
** The only argument is a pointer to a function which works like
** malloc.
**
** Inputs:
** A pointer to the function used to allocate memory.
**
** Outputs:
** A pointer to a parser.  This pointer is used in subsequent calls
** to sqlite3Parser and sqlite3ParserFree.
*/
void *sqlite3ParserAlloc(void *(*mallocProc)(YYMALLOCARGTYPE)){
  yyParser *pParser;
  pParser = (yyParser*)(*mallocProc)( (YYMALLOCARGTYPE)sizeof(yyParser) );
  if( pParser ){
#ifdef YYTRACKMAXSTACKDEPTH
    pParser->yyhwm = 0;
#endif
    pParser->is_fallback_failed = false;
#if YYSTACKDEPTH<=0
    pParser->yytos = NULL;
    pParser->yystack = NULL;
    pParser->yystksz = 0;
    if( yyGrowStack(pParser) ){
      pParser->yystack = &pParser->yystk0;
      pParser->yystksz = 1;
    }
#endif
#ifndef YYNOERRORRECOVERY
    pParser->yyerrcnt = -1;
#endif
    pParser->yytos = pParser->yystack;
    pParser->yystack[0].stateno = 0;
    pParser->yystack[0].major = 0;
  }
  return pParser;
}

/* The following function deletes the "minor type" or semantic value
** associated with a symbol.  The symbol can be either a terminal
** or nonterminal. "yymajor" is the symbol code, and "yypminor" is
** a pointer to the value to be deleted.  The code used to do the 
** deletions is derived from the %destructor and/or %token_destructor
** directives of the input grammar.
*/
static void yy_destructor(
  yyParser *yypParser,    /* The parser */
  YYCODETYPE yymajor,     /* Type code for object to destroy */
  YYMINORTYPE *yypminor   /* The object to be destroyed */
){
  sqlite3ParserARG_FETCH;
  switch( yymajor ){
    /* Here is inserted the actions which take place when a
    ** terminal or non-terminal is destroyed.  This can happen
    ** when the symbol is popped from the stack during a
    ** reduce or during error processing or when a parser is 
    ** being destroyed before it is finished parsing.
    **
    ** Note: during a reduce, the only symbols destroyed are those
    ** which appear on the RHS of the rule, but which are *not* used
    ** inside the C code.
    */
/********* Begin destructor definitions ***************************************/
    case 163: /* select */
    case 192: /* selectnowith */
    case 193: /* oneselect */
    case 204: /* values */
{
#line 378 "/home/kir/tarantool/src/box/sql/parse.y"
sql_select_delete(pParse->db, (yypminor->yy375));
#line 1492 "parse.c"
}
      break;
    case 168: /* term */
    case 169: /* expr */
{
#line 801 "/home/kir/tarantool/src/box/sql/parse.y"
sql_expr_delete(pParse->db, (yypminor->yy370).pExpr, false);
#line 1500 "parse.c"
}
      break;
    case 173: /* eidlist_opt */
    case 183: /* sortlist */
    case 184: /* eidlist */
    case 197: /* selcollist */
    case 200: /* groupby_opt */
    case 202: /* orderby_opt */
    case 205: /* nexprlist */
    case 206: /* exprlist */
    case 207: /* sclp */
    case 217: /* setlist */
    case 224: /* paren_exprlist */
    case 226: /* case_exprlist */
{
#line 1234 "/home/kir/tarantool/src/box/sql/parse.y"
sql_expr_list_delete(pParse->db, (yypminor->yy418));
#line 1518 "parse.c"
}
      break;
    case 191: /* fullname */
    case 198: /* from */
    case 209: /* seltablist */
    case 210: /* stl_prefix */
{
#line 605 "/home/kir/tarantool/src/box/sql/parse.y"
sqlite3SrcListDelete(pParse->db, (yypminor->yy151));
#line 1528 "parse.c"
}
      break;
    case 194: /* with */
    case 242: /* wqlist */
{
#line 1464 "/home/kir/tarantool/src/box/sql/parse.y"
sqlite3WithDelete(pParse->db, (yypminor->yy43));
#line 1536 "parse.c"
}
      break;
    case 199: /* where_opt */
    case 201: /* having_opt */
    case 213: /* on_opt */
    case 225: /* case_operand */
    case 227: /* case_else */
    case 238: /* when_clause */
{
#line 723 "/home/kir/tarantool/src/box/sql/parse.y"
sql_expr_delete(pParse->db, (yypminor->yy62), false);
#line 1548 "parse.c"
}
      break;
    case 214: /* using_opt */
    case 216: /* idlist */
    case 219: /* idlist_opt */
{
#line 642 "/home/kir/tarantool/src/box/sql/parse.y"
sqlite3IdListDelete(pParse->db, (yypminor->yy240));
#line 1557 "parse.c"
}
      break;
    case 234: /* trigger_cmd_list */
    case 239: /* trigger_cmd */
{
#line 1354 "/home/kir/tarantool/src/box/sql/parse.y"
sqlite3DeleteTriggerStep(pParse->db, (yypminor->yy360));
#line 1565 "parse.c"
}
      break;
    case 236: /* trigger_event */
{
#line 1340 "/home/kir/tarantool/src/box/sql/parse.y"
sqlite3IdListDelete(pParse->db, (yypminor->yy30).b);
#line 1572 "parse.c"
}
      break;
/********* End destructor definitions *****************************************/
    default:  break;   /* If no destructor action specified: do nothing */
  }
}

/*
** Pop the parser's stack once.
**
** If there is a destructor routine associated with the token which
** is popped from the stack, then call it.
*/
static void yy_pop_parser_stack(yyParser *pParser){
  yyStackEntry *yytos;
  assert( pParser->yytos!=0 );
  assert( pParser->yytos > pParser->yystack );
  yytos = pParser->yytos--;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sPopping %s\n",
      yyTracePrompt,
      yyTokenName[yytos->major]);
  }
#endif
  yy_destructor(pParser, yytos->major, &yytos->minor);
}

/* 
** Deallocate and destroy a parser.  Destructors are called for
** all stack elements before shutting the parser down.
**
** If the YYPARSEFREENEVERNULL macro exists (for example because it
** is defined in a %include section of the input grammar) then it is
** assumed that the input pointer is never NULL.
*/
void sqlite3ParserFree(
  void *p,                    /* The parser to be deleted */
  void (*freeProc)(void*)     /* Function used to reclaim memory */
){
  yyParser *pParser = (yyParser*)p;
#ifndef YYPARSEFREENEVERNULL
  if( pParser==0 ) return;
#endif
  while( pParser->yytos>pParser->yystack ) yy_pop_parser_stack(pParser);
#if YYSTACKDEPTH<=0
  if( pParser->yystack!=&pParser->yystk0 ) free(pParser->yystack);
#endif
  (*freeProc)((void*)pParser);
}

/*
** Return the peak depth of the stack for a parser.
*/
#ifdef YYTRACKMAXSTACKDEPTH
int sqlite3ParserStackPeak(void *p){
  yyParser *pParser = (yyParser*)p;
  return pParser->yyhwm;
}
#endif

/*
** Find the appropriate action for a parser given the terminal
** look-ahead token iLookAhead.
*/
static unsigned int yy_find_shift_action(
  yyParser *pParser,        /* The parser */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
  int stateno = pParser->yytos->stateno;
 
  if( stateno>=YY_MIN_REDUCE ) return stateno;
  assert( stateno <= YY_SHIFT_COUNT );
  do{
    i = yy_shift_ofst[stateno];
    assert( iLookAhead!=YYNOCODE );
    i += iLookAhead;
    if( i<0 || i>=YY_ACTTAB_COUNT || yy_lookahead[i]!=iLookAhead ){
#ifdef YYFALLBACK
      YYCODETYPE iFallback = -1;            /* Fallback token */
      if( iLookAhead<sizeof(yyFallback)/sizeof(yyFallback[0])
             && (iFallback = yyFallback[iLookAhead])!=0 ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE, "%sFALLBACK %s => %s\n",
             yyTracePrompt, yyTokenName[iLookAhead], yyTokenName[iFallback]);
        }
#endif
        assert( yyFallback[iFallback]==0 ); /* Fallback loop must terminate */
        iLookAhead = iFallback;
        continue;
      } else if ( iFallback==0 ) {
        pParser->is_fallback_failed = true;
      }
#endif
#ifdef YYWILDCARD
      {
        int j = i - iLookAhead + YYWILDCARD;
        if( 
#if YY_SHIFT_MIN+YYWILDCARD<0
          j>=0 &&
#endif
#if YY_SHIFT_MAX+YYWILDCARD>=YY_ACTTAB_COUNT
          j<YY_ACTTAB_COUNT &&
#endif
          yy_lookahead[j]==YYWILDCARD && iLookAhead>0
        ){
#ifndef NDEBUG
          if( yyTraceFILE ){
            fprintf(yyTraceFILE, "%sWILDCARD %s => %s\n",
               yyTracePrompt, yyTokenName[iLookAhead],
               yyTokenName[YYWILDCARD]);
          }
#endif /* NDEBUG */
          return yy_action[j];
        }
      }
#endif /* YYWILDCARD */
      return yy_default[stateno];
    }else{
      return yy_action[i];
    }
  }while(1);
}

/*
** Find the appropriate action for a parser given the non-terminal
** look-ahead token iLookAhead.
*/
static int yy_find_reduce_action(
  int stateno,              /* Current state number */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
#ifdef YYERRORSYMBOL
  if( stateno>YY_REDUCE_COUNT ){
    return yy_default[stateno];
  }
#else
  assert( stateno<=YY_REDUCE_COUNT );
#endif
  i = yy_reduce_ofst[stateno];
  assert( i!=YY_REDUCE_USE_DFLT );
  assert( iLookAhead!=YYNOCODE );
  i += iLookAhead;
#ifdef YYERRORSYMBOL
  if( i<0 || i>=YY_ACTTAB_COUNT || yy_lookahead[i]!=iLookAhead ){
    return yy_default[stateno];
  }
#else
  assert( i>=0 && i<YY_ACTTAB_COUNT );
  assert( yy_lookahead[i]==iLookAhead );
#endif
  return yy_action[i];
}

/*
** The following routine is called if the stack overflows.
*/
static void yyStackOverflow(yyParser *yypParser){
   sqlite3ParserARG_FETCH;
#ifndef NDEBUG
   if( yyTraceFILE ){
     fprintf(yyTraceFILE,"%sStack Overflow!\n",yyTracePrompt);
   }
#endif
   while( yypParser->yytos>yypParser->yystack ) yy_pop_parser_stack(yypParser);
   /* Here code is inserted which will execute if the parser
   ** stack every overflows */
/******** Begin %stack_overflow code ******************************************/
#line 41 "/home/kir/tarantool/src/box/sql/parse.y"

  sqlite3ErrorMsg(pParse, "parser stack overflow");
#line 1747 "parse.c"
/******** End %stack_overflow code ********************************************/
   sqlite3ParserARG_STORE; /* Suppress warning about unused %extra_argument var */
}

/*
** Print tracing information for a SHIFT action
*/
#ifndef NDEBUG
static void yyTraceShift(yyParser *yypParser, int yyNewState){
  if( yyTraceFILE ){
    if( yyNewState<YYNSTATE ){
      fprintf(yyTraceFILE,"%sShift '%s', go to state %d\n",
         yyTracePrompt,yyTokenName[yypParser->yytos->major],
         yyNewState);
    }else{
      fprintf(yyTraceFILE,"%sShift '%s'\n",
         yyTracePrompt,yyTokenName[yypParser->yytos->major]);
    }
  }
}
#else
# define yyTraceShift(X,Y)
#endif

/*
** Perform a shift action.
*/
static void yy_shift(
  yyParser *yypParser,          /* The parser to be shifted */
  int yyNewState,               /* The new state to shift in */
  int yyMajor,                  /* The major token to shift in */
  sqlite3ParserTOKENTYPE yyMinor        /* The minor token to shift in */
){
  yyStackEntry *yytos;
  yypParser->yytos++;
#ifdef YYTRACKMAXSTACKDEPTH
  if( (int)(yypParser->yytos - yypParser->yystack)>yypParser->yyhwm ){
    yypParser->yyhwm++;
    assert( yypParser->yyhwm == (int)(yypParser->yytos - yypParser->yystack) );
  }
#endif
#if YYSTACKDEPTH>0 
  if( yypParser->yytos>=&yypParser->yystack[YYSTACKDEPTH] ){
    yypParser->yytos--;
    yyStackOverflow(yypParser);
    return;
  }
#else
  if( yypParser->yytos>=&yypParser->yystack[yypParser->yystksz] ){
    if( yyGrowStack(yypParser) ){
      yypParser->yytos--;
      yyStackOverflow(yypParser);
      return;
    }
  }
#endif
  if( yyNewState > YY_MAX_SHIFT ){
    yyNewState += YY_MIN_REDUCE - YY_MIN_SHIFTREDUCE;
  }
  yytos = yypParser->yytos;
  yytos->stateno = (YYACTIONTYPE)yyNewState;
  yytos->major = (YYCODETYPE)yyMajor;
  yytos->minor.yy0 = yyMinor;
  yyTraceShift(yypParser, yyNewState);
}

/* The following table contains information about every rule that
** is used during the reduce.
*/
static const struct {
  YYCODETYPE lhs;         /* Symbol on the left-hand side of the rule */
  unsigned char nrhs;     /* Number of right-hand side symbols in the rule */
} yyRuleInfo[] = {
  { 151, 3 },
  { 151, 1 },
  { 152, 1 },
  { 152, 3 },
  { 154, 2 },
  { 154, 1 },
  { 154, 1 },
  { 154, 2 },
  { 154, 3 },
  { 154, 4 },
  { 157, 4 },
  { 159, 1 },
  { 160, 0 },
  { 160, 3 },
  { 158, 4 },
  { 158, 2 },
  { 164, 2 },
  { 156, 1 },
  { 167, 2 },
  { 167, 2 },
  { 167, 4 },
  { 167, 3 },
  { 167, 3 },
  { 167, 2 },
  { 167, 3 },
  { 167, 4 },
  { 167, 1 },
  { 167, 4 },
  { 167, 4 },
  { 167, 1 },
  { 167, 2 },
  { 172, 0 },
  { 172, 1 },
  { 174, 0 },
  { 174, 2 },
  { 176, 2 },
  { 176, 3 },
  { 176, 3 },
  { 176, 3 },
  { 177, 1 },
  { 177, 1 },
  { 177, 1 },
  { 178, 2 },
  { 178, 2 },
  { 178, 1 },
  { 178, 1 },
  { 178, 2 },
  { 175, 3 },
  { 175, 2 },
  { 179, 0 },
  { 179, 2 },
  { 179, 2 },
  { 181, 1 },
  { 182, 2 },
  { 182, 6 },
  { 182, 4 },
  { 182, 5 },
  { 182, 10 },
  { 185, 0 },
  { 170, 0 },
  { 170, 3 },
  { 187, 0 },
  { 187, 2 },
  { 188, 1 },
  { 188, 1 },
  { 154, 4 },
  { 190, 2 },
  { 190, 0 },
  { 154, 7 },
  { 154, 4 },
  { 154, 1 },
  { 163, 2 },
  { 192, 3 },
  { 195, 1 },
  { 195, 2 },
  { 195, 1 },
  { 193, 9 },
  { 204, 4 },
  { 204, 5 },
  { 196, 1 },
  { 196, 1 },
  { 196, 0 },
  { 207, 0 },
  { 197, 3 },
  { 197, 2 },
  { 197, 4 },
  { 208, 2 },
  { 208, 0 },
  { 198, 0 },
  { 198, 2 },
  { 210, 2 },
  { 210, 0 },
  { 209, 6 },
  { 209, 8 },
  { 209, 7 },
  { 209, 7 },
  { 191, 1 },
  { 211, 1 },
  { 211, 2 },
  { 211, 3 },
  { 211, 4 },
  { 213, 2 },
  { 213, 0 },
  { 212, 0 },
  { 212, 3 },
  { 212, 2 },
  { 214, 4 },
  { 214, 0 },
  { 202, 0 },
  { 202, 3 },
  { 183, 4 },
  { 183, 2 },
  { 171, 1 },
  { 171, 1 },
  { 171, 0 },
  { 200, 0 },
  { 200, 3 },
  { 201, 0 },
  { 201, 2 },
  { 203, 0 },
  { 203, 2 },
  { 203, 4 },
  { 203, 4 },
  { 154, 6 },
  { 154, 3 },
  { 199, 0 },
  { 199, 2 },
  { 154, 8 },
  { 217, 5 },
  { 217, 7 },
  { 217, 3 },
  { 217, 5 },
  { 154, 6 },
  { 154, 7 },
  { 218, 2 },
  { 218, 1 },
  { 219, 0 },
  { 219, 3 },
  { 216, 3 },
  { 216, 1 },
  { 169, 3 },
  { 168, 1 },
  { 169, 1 },
  { 169, 1 },
  { 169, 3 },
  { 168, 1 },
  { 168, 1 },
  { 168, 1 },
  { 169, 1 },
  { 169, 3 },
  { 169, 6 },
  { 169, 5 },
  { 169, 5 },
  { 169, 4 },
  { 168, 1 },
  { 169, 5 },
  { 169, 3 },
  { 169, 3 },
  { 169, 3 },
  { 169, 3 },
  { 169, 3 },
  { 169, 3 },
  { 169, 3 },
  { 169, 3 },
  { 221, 1 },
  { 221, 2 },
  { 169, 3 },
  { 169, 5 },
  { 169, 3 },
  { 169, 4 },
  { 169, 2 },
  { 169, 2 },
  { 169, 2 },
  { 169, 2 },
  { 222, 1 },
  { 222, 2 },
  { 169, 5 },
  { 223, 1 },
  { 223, 2 },
  { 169, 5 },
  { 169, 3 },
  { 169, 5 },
  { 169, 4 },
  { 169, 4 },
  { 169, 5 },
  { 226, 5 },
  { 226, 4 },
  { 227, 2 },
  { 227, 0 },
  { 225, 1 },
  { 225, 0 },
  { 206, 0 },
  { 205, 3 },
  { 205, 1 },
  { 224, 0 },
  { 224, 3 },
  { 154, 10 },
  { 228, 1 },
  { 228, 0 },
  { 173, 0 },
  { 173, 3 },
  { 184, 5 },
  { 184, 3 },
  { 229, 0 },
  { 229, 2 },
  { 154, 6 },
  { 154, 2 },
  { 154, 4 },
  { 154, 5 },
  { 154, 4 },
  { 154, 5 },
  { 154, 6 },
  { 154, 1 },
  { 232, 2 },
  { 231, 2 },
  { 154, 5 },
  { 233, 9 },
  { 235, 1 },
  { 235, 1 },
  { 235, 2 },
  { 235, 0 },
  { 236, 1 },
  { 236, 1 },
  { 236, 3 },
  { 238, 0 },
  { 238, 2 },
  { 234, 3 },
  { 234, 2 },
  { 240, 3 },
  { 241, 3 },
  { 241, 2 },
  { 239, 7 },
  { 239, 5 },
  { 239, 5 },
  { 239, 1 },
  { 169, 4 },
  { 169, 6 },
  { 189, 1 },
  { 189, 1 },
  { 189, 1 },
  { 154, 4 },
  { 154, 1 },
  { 154, 2 },
  { 154, 6 },
  { 154, 16 },
  { 154, 6 },
  { 194, 0 },
  { 194, 2 },
  { 194, 3 },
  { 242, 6 },
  { 242, 8 },
  { 166, 1 },
  { 166, 1 },
  { 166, 1 },
  { 166, 1 },
  { 166, 1 },
  { 166, 2 },
  { 243, 3 },
  { 244, 1 },
  { 244, 1 },
  { 244, 2 },
  { 245, 0 },
  { 245, 3 },
  { 245, 5 },
  { 150, 1 },
  { 152, 0 },
  { 153, 1 },
  { 155, 1 },
  { 155, 0 },
  { 154, 2 },
  { 161, 4 },
  { 161, 2 },
  { 165, 2 },
  { 165, 0 },
  { 162, 0 },
  { 162, 2 },
  { 180, 3 },
  { 180, 1 },
  { 181, 0 },
  { 185, 1 },
  { 188, 1 },
  { 192, 1 },
  { 193, 1 },
  { 207, 2 },
  { 208, 1 },
  { 215, 1 },
  { 215, 1 },
  { 169, 1 },
  { 220, 1 },
  { 220, 1 },
  { 220, 1 },
  { 206, 1 },
  { 230, 1 },
  { 230, 1 },
  { 230, 1 },
  { 230, 1 },
  { 230, 1 },
  { 230, 1 },
  { 232, 1 },
  { 237, 0 },
  { 237, 3 },
  { 240, 1 },
  { 241, 0 },
  { 166, 1 },
};

static void yy_accept(yyParser*);  /* Forward Declaration */

/*
** Perform a reduce action and the shift that must immediately
** follow the reduce.
*/
static void yy_reduce(
  yyParser *yypParser,         /* The parser */
  unsigned int yyruleno        /* Number of the rule by which to reduce */
){
  int yygoto;                     /* The next state */
  int yyact;                      /* The next action */
  yyStackEntry *yymsp;            /* The top of the parser's stack */
  int yysize;                     /* Amount to pop the stack */
  sqlite3ParserARG_FETCH;
  yymsp = yypParser->yytos;
#ifndef NDEBUG
  if( yyTraceFILE && yyruleno<(int)(sizeof(yyRuleName)/sizeof(yyRuleName[0])) ){
    yysize = yyRuleInfo[yyruleno].nrhs;
    fprintf(yyTraceFILE, "%sReduce [%s], go to state %d.\n", yyTracePrompt,
      yyRuleName[yyruleno], yymsp[-yysize].stateno);
  }
#endif /* NDEBUG */

  /* Check that the stack is large enough to grow by a single entry
  ** if the RHS of the rule is empty.  This ensures that there is room
  ** enough on the stack to push the LHS value */
  if( yyRuleInfo[yyruleno].nrhs==0 ){
#ifdef YYTRACKMAXSTACKDEPTH
    if( (int)(yypParser->yytos - yypParser->yystack)>yypParser->yyhwm ){
      yypParser->yyhwm++;
      assert( yypParser->yyhwm == (int)(yypParser->yytos - yypParser->yystack));
    }
#endif
#if YYSTACKDEPTH>0 
    if( yypParser->yytos>=&yypParser->yystack[YYSTACKDEPTH-1] ){
      yyStackOverflow(yypParser);
      return;
    }
#else
    if( yypParser->yytos>=&yypParser->yystack[yypParser->yystksz-1] ){
      if( yyGrowStack(yypParser) ){
        yyStackOverflow(yypParser);
        return;
      }
      yymsp = yypParser->yytos;
    }
#endif
  }

  switch( yyruleno ){
  /* Beginning here are the reduction cases.  A typical example
  ** follows:
  **   case 0:
  **  #line <lineno> <grammarfile>
  **     { ... }           // User supplied code
  **  #line <lineno> <thisfile>
  **     break;
  */
/********** Begin reduce actions **********************************************/
        YYMINORTYPE yylhsminor;
      case 0: /* ecmd ::= explain cmdx SEMI */
#line 112 "/home/kir/tarantool/src/box/sql/parse.y"
{
	if (!pParse->parse_only)
		sql_finish_coding(pParse);
}
#line 2194 "parse.c"
        break;
      case 1: /* ecmd ::= SEMI */
#line 116 "/home/kir/tarantool/src/box/sql/parse.y"
{
  sqlite3ErrorMsg(pParse, "syntax error: empty request");
}
#line 2201 "parse.c"
        break;
      case 2: /* explain ::= EXPLAIN */
#line 120 "/home/kir/tarantool/src/box/sql/parse.y"
{ pParse->explain = 1; }
#line 2206 "parse.c"
        break;
      case 3: /* explain ::= EXPLAIN QUERY PLAN */
#line 121 "/home/kir/tarantool/src/box/sql/parse.y"
{ pParse->explain = 2; }
#line 2211 "parse.c"
        break;
      case 4: /* cmd ::= START TRANSACTION */
#line 151 "/home/kir/tarantool/src/box/sql/parse.y"
{sql_transaction_begin(pParse);}
#line 2216 "parse.c"
        break;
      case 5: /* cmd ::= COMMIT */
#line 152 "/home/kir/tarantool/src/box/sql/parse.y"
{sql_transaction_commit(pParse);}
#line 2221 "parse.c"
        break;
      case 6: /* cmd ::= ROLLBACK */
#line 153 "/home/kir/tarantool/src/box/sql/parse.y"
{sql_transaction_rollback(pParse);}
#line 2226 "parse.c"
        break;
      case 7: /* cmd ::= SAVEPOINT nm */
#line 157 "/home/kir/tarantool/src/box/sql/parse.y"
{
  sqlite3Savepoint(pParse, SAVEPOINT_BEGIN, &yymsp[0].minor.yy0);
}
#line 2233 "parse.c"
        break;
      case 8: /* cmd ::= RELEASE savepoint_opt nm */
#line 160 "/home/kir/tarantool/src/box/sql/parse.y"
{
  sqlite3Savepoint(pParse, SAVEPOINT_RELEASE, &yymsp[0].minor.yy0);
}
#line 2240 "parse.c"
        break;
      case 9: /* cmd ::= ROLLBACK TO savepoint_opt nm */
#line 163 "/home/kir/tarantool/src/box/sql/parse.y"
{
  sqlite3Savepoint(pParse, SAVEPOINT_ROLLBACK, &yymsp[0].minor.yy0);
}
#line 2247 "parse.c"
        break;
      case 10: /* create_table ::= createkw TABLE ifnotexists nm */
#line 170 "/home/kir/tarantool/src/box/sql/parse.y"
{
   sqlite3StartTable(pParse,&yymsp[0].minor.yy0,yymsp[-1].minor.yy280);
}
#line 2254 "parse.c"
        break;
      case 11: /* createkw ::= CREATE */
#line 173 "/home/kir/tarantool/src/box/sql/parse.y"
{disableLookaside(pParse);}
#line 2259 "parse.c"
        break;
      case 12: /* ifnotexists ::= */
      case 31: /* autoinc ::= */ yytestcase(yyruleno==31);
      case 49: /* init_deferred_pred_opt ::= */ yytestcase(yyruleno==49);
      case 58: /* defer_subclause_opt ::= */ yytestcase(yyruleno==58);
      case 67: /* ifexists ::= */ yytestcase(yyruleno==67);
      case 81: /* distinct ::= */ yytestcase(yyruleno==81);
      case 203: /* collate ::= */ yytestcase(yyruleno==203);
#line 176 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[1].minor.yy280 = 0;}
#line 2270 "parse.c"
        break;
      case 13: /* ifnotexists ::= IF NOT EXISTS */
#line 177 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[-2].minor.yy280 = 1;}
#line 2275 "parse.c"
        break;
      case 14: /* create_table_args ::= LP columnlist conslist_opt RP */
#line 179 "/home/kir/tarantool/src/box/sql/parse.y"
{
  sqlite3EndTable(pParse,&yymsp[0].minor.yy0,0);
}
#line 2282 "parse.c"
        break;
      case 15: /* create_table_args ::= AS select */
#line 182 "/home/kir/tarantool/src/box/sql/parse.y"
{
  sqlite3EndTable(pParse,0,yymsp[0].minor.yy375);
  sql_select_delete(pParse->db, yymsp[0].minor.yy375);
}
#line 2290 "parse.c"
        break;
      case 16: /* columnname ::= nm typedef */
#line 188 "/home/kir/tarantool/src/box/sql/parse.y"
{sqlite3AddColumn(pParse,&yymsp[-1].minor.yy0,&yymsp[0].minor.yy66);}
#line 2295 "parse.c"
        break;
      case 17: /* nm ::= ID|INDEXED */
#line 226 "/home/kir/tarantool/src/box/sql/parse.y"
{
  if(yymsp[0].minor.yy0.isReserved) {
    sqlite3ErrorMsg(pParse, "keyword \"%T\" is reserved", &yymsp[0].minor.yy0);
  }
}
#line 2304 "parse.c"
        break;
      case 18: /* ccons ::= CONSTRAINT nm */
      case 53: /* tcons ::= CONSTRAINT nm */ yytestcase(yyruleno==53);
#line 237 "/home/kir/tarantool/src/box/sql/parse.y"
{pParse->constraintName = yymsp[0].minor.yy0;}
#line 2310 "parse.c"
        break;
      case 19: /* ccons ::= DEFAULT term */
      case 21: /* ccons ::= DEFAULT PLUS term */ yytestcase(yyruleno==21);
#line 238 "/home/kir/tarantool/src/box/sql/parse.y"
{sqlite3AddDefaultValue(pParse,&yymsp[0].minor.yy370);}
#line 2316 "parse.c"
        break;
      case 20: /* ccons ::= DEFAULT LP expr RP */
#line 239 "/home/kir/tarantool/src/box/sql/parse.y"
{sqlite3AddDefaultValue(pParse,&yymsp[-1].minor.yy370);}
#line 2321 "parse.c"
        break;
      case 22: /* ccons ::= DEFAULT MINUS term */
#line 241 "/home/kir/tarantool/src/box/sql/parse.y"
{
  ExprSpan v;
  v.pExpr = sqlite3PExpr(pParse, TK_UMINUS, yymsp[0].minor.yy370.pExpr, 0);
  v.zStart = yymsp[-1].minor.yy0.z;
  v.zEnd = yymsp[0].minor.yy370.zEnd;
  sqlite3AddDefaultValue(pParse,&v);
}
#line 2332 "parse.c"
        break;
      case 23: /* ccons ::= NULL onconf */
#line 252 "/home/kir/tarantool/src/box/sql/parse.y"
{
    sql_column_add_nullable_action(pParse, ON_CONFLICT_ACTION_NONE);
    /* Trigger nullability mismatch error if required. */
    if (yymsp[0].minor.yy280 != ON_CONFLICT_ACTION_ABORT)
        sql_column_add_nullable_action(pParse, yymsp[0].minor.yy280);
}
#line 2342 "parse.c"
        break;
      case 24: /* ccons ::= NOT NULL onconf */
#line 258 "/home/kir/tarantool/src/box/sql/parse.y"
{sql_column_add_nullable_action(pParse, yymsp[0].minor.yy280);}
#line 2347 "parse.c"
        break;
      case 25: /* ccons ::= PRIMARY KEY sortorder autoinc */
#line 260 "/home/kir/tarantool/src/box/sql/parse.y"
{sqlite3AddPrimaryKey(pParse,0,yymsp[0].minor.yy280,yymsp[-1].minor.yy280);}
#line 2352 "parse.c"
        break;
      case 26: /* ccons ::= UNIQUE */
#line 261 "/home/kir/tarantool/src/box/sql/parse.y"
{sql_create_index(pParse,0,0,0,0,
                                                   SORT_ORDER_ASC, false,
                                                   SQL_INDEX_TYPE_CONSTRAINT_UNIQUE);}
#line 2359 "parse.c"
        break;
      case 27: /* ccons ::= CHECK LP expr RP */
#line 264 "/home/kir/tarantool/src/box/sql/parse.y"
{sql_add_check_constraint(pParse,&yymsp[-1].minor.yy370);}
#line 2364 "parse.c"
        break;
      case 28: /* ccons ::= REFERENCES nm eidlist_opt refargs */
#line 266 "/home/kir/tarantool/src/box/sql/parse.y"
{sql_create_foreign_key(pParse, NULL, NULL, NULL, &yymsp[-2].minor.yy0, yymsp[-1].minor.yy418, false, yymsp[0].minor.yy280);}
#line 2369 "parse.c"
        break;
      case 29: /* ccons ::= defer_subclause */
#line 267 "/home/kir/tarantool/src/box/sql/parse.y"
{fkey_change_defer_mode(pParse, yymsp[0].minor.yy280);}
#line 2374 "parse.c"
        break;
      case 30: /* ccons ::= COLLATE ID|INDEXED */
#line 268 "/home/kir/tarantool/src/box/sql/parse.y"
{sqlite3AddCollateType(pParse, &yymsp[0].minor.yy0);}
#line 2379 "parse.c"
        break;
      case 32: /* autoinc ::= AUTOINCR */
#line 273 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[0].minor.yy280 = 1;}
#line 2384 "parse.c"
        break;
      case 33: /* refargs ::= */
#line 281 "/home/kir/tarantool/src/box/sql/parse.y"
{ yymsp[1].minor.yy280 = FKEY_NO_ACTION; }
#line 2389 "parse.c"
        break;
      case 34: /* refargs ::= refargs refarg */
#line 282 "/home/kir/tarantool/src/box/sql/parse.y"
{ yymsp[-1].minor.yy280 = (yymsp[-1].minor.yy280 & ~yymsp[0].minor.yy359.mask) | yymsp[0].minor.yy359.value; }
#line 2394 "parse.c"
        break;
      case 35: /* refarg ::= MATCH matcharg */
#line 284 "/home/kir/tarantool/src/box/sql/parse.y"
{ yymsp[-1].minor.yy359.value = yymsp[0].minor.yy280<<16; yymsp[-1].minor.yy359.mask = 0xff0000; }
#line 2399 "parse.c"
        break;
      case 36: /* refarg ::= ON INSERT refact */
#line 285 "/home/kir/tarantool/src/box/sql/parse.y"
{ yymsp[-2].minor.yy359.value = 0;     yymsp[-2].minor.yy359.mask = 0x000000; }
#line 2404 "parse.c"
        break;
      case 37: /* refarg ::= ON DELETE refact */
#line 286 "/home/kir/tarantool/src/box/sql/parse.y"
{ yymsp[-2].minor.yy359.value = yymsp[0].minor.yy280;     yymsp[-2].minor.yy359.mask = 0x0000ff; }
#line 2409 "parse.c"
        break;
      case 38: /* refarg ::= ON UPDATE refact */
#line 287 "/home/kir/tarantool/src/box/sql/parse.y"
{ yymsp[-2].minor.yy359.value = yymsp[0].minor.yy280<<8;  yymsp[-2].minor.yy359.mask = 0x00ff00; }
#line 2414 "parse.c"
        break;
      case 39: /* matcharg ::= SIMPLE */
#line 289 "/home/kir/tarantool/src/box/sql/parse.y"
{ yymsp[0].minor.yy280 = FKEY_MATCH_SIMPLE; }
#line 2419 "parse.c"
        break;
      case 40: /* matcharg ::= PARTIAL */
#line 290 "/home/kir/tarantool/src/box/sql/parse.y"
{ yymsp[0].minor.yy280 = FKEY_MATCH_PARTIAL; }
#line 2424 "parse.c"
        break;
      case 41: /* matcharg ::= FULL */
#line 291 "/home/kir/tarantool/src/box/sql/parse.y"
{ yymsp[0].minor.yy280 = FKEY_MATCH_FULL; }
#line 2429 "parse.c"
        break;
      case 42: /* refact ::= SET NULL */
#line 293 "/home/kir/tarantool/src/box/sql/parse.y"
{ yymsp[-1].minor.yy280 = FKEY_ACTION_SET_NULL; }
#line 2434 "parse.c"
        break;
      case 43: /* refact ::= SET DEFAULT */
#line 294 "/home/kir/tarantool/src/box/sql/parse.y"
{ yymsp[-1].minor.yy280 = FKEY_ACTION_SET_DEFAULT; }
#line 2439 "parse.c"
        break;
      case 44: /* refact ::= CASCADE */
#line 295 "/home/kir/tarantool/src/box/sql/parse.y"
{ yymsp[0].minor.yy280 = FKEY_ACTION_CASCADE; }
#line 2444 "parse.c"
        break;
      case 45: /* refact ::= RESTRICT */
#line 296 "/home/kir/tarantool/src/box/sql/parse.y"
{ yymsp[0].minor.yy280 = FKEY_ACTION_RESTRICT; }
#line 2449 "parse.c"
        break;
      case 46: /* refact ::= NO ACTION */
#line 297 "/home/kir/tarantool/src/box/sql/parse.y"
{ yymsp[-1].minor.yy280 = FKEY_NO_ACTION; }
#line 2454 "parse.c"
        break;
      case 47: /* defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt */
#line 299 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[-2].minor.yy280 = 0;}
#line 2459 "parse.c"
        break;
      case 48: /* defer_subclause ::= DEFERRABLE init_deferred_pred_opt */
      case 62: /* orconf ::= OR resolvetype */ yytestcase(yyruleno==62);
      case 134: /* insert_cmd ::= INSERT orconf */ yytestcase(yyruleno==134);
#line 300 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[-1].minor.yy280 = yymsp[0].minor.yy280;}
#line 2466 "parse.c"
        break;
      case 50: /* init_deferred_pred_opt ::= INITIALLY DEFERRED */
      case 66: /* ifexists ::= IF EXISTS */ yytestcase(yyruleno==66);
      case 175: /* between_op ::= NOT BETWEEN */ yytestcase(yyruleno==175);
      case 178: /* in_op ::= NOT IN */ yytestcase(yyruleno==178);
      case 204: /* collate ::= COLLATE ID|INDEXED */ yytestcase(yyruleno==204);
#line 303 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[-1].minor.yy280 = 1;}
#line 2475 "parse.c"
        break;
      case 51: /* init_deferred_pred_opt ::= INITIALLY IMMEDIATE */
#line 304 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[-1].minor.yy280 = 0;}
#line 2480 "parse.c"
        break;
      case 52: /* tconscomma ::= COMMA */
#line 310 "/home/kir/tarantool/src/box/sql/parse.y"
{pParse->constraintName.n = 0;}
#line 2485 "parse.c"
        break;
      case 54: /* tcons ::= PRIMARY KEY LP sortlist autoinc RP */
#line 314 "/home/kir/tarantool/src/box/sql/parse.y"
{sqlite3AddPrimaryKey(pParse,yymsp[-2].minor.yy418,yymsp[-1].minor.yy280,0);}
#line 2490 "parse.c"
        break;
      case 55: /* tcons ::= UNIQUE LP sortlist RP */
#line 316 "/home/kir/tarantool/src/box/sql/parse.y"
{sql_create_index(pParse,0,0,yymsp[-1].minor.yy418,0,
                                                   SORT_ORDER_ASC,false,
                                                   SQL_INDEX_TYPE_CONSTRAINT_UNIQUE);}
#line 2497 "parse.c"
        break;
      case 56: /* tcons ::= CHECK LP expr RP onconf */
#line 320 "/home/kir/tarantool/src/box/sql/parse.y"
{sql_add_check_constraint(pParse,&yymsp[-2].minor.yy370);}
#line 2502 "parse.c"
        break;
      case 57: /* tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt refargs defer_subclause_opt */
#line 322 "/home/kir/tarantool/src/box/sql/parse.y"
{
    sql_create_foreign_key(pParse, NULL, NULL, yymsp[-6].minor.yy418, &yymsp[-3].minor.yy0, yymsp[-2].minor.yy418, yymsp[0].minor.yy280, yymsp[-1].minor.yy280);
}
#line 2509 "parse.c"
        break;
      case 59: /* onconf ::= */
#line 336 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[1].minor.yy280 = ON_CONFLICT_ACTION_ABORT;}
#line 2514 "parse.c"
        break;
      case 60: /* onconf ::= ON CONFLICT resolvetype */
#line 337 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[-2].minor.yy280 = yymsp[0].minor.yy280;}
#line 2519 "parse.c"
        break;
      case 61: /* orconf ::= */
#line 338 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[1].minor.yy280 = ON_CONFLICT_ACTION_DEFAULT;}
#line 2524 "parse.c"
        break;
      case 63: /* resolvetype ::= IGNORE */
#line 341 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[0].minor.yy280 = ON_CONFLICT_ACTION_IGNORE;}
#line 2529 "parse.c"
        break;
      case 64: /* resolvetype ::= REPLACE */
      case 135: /* insert_cmd ::= REPLACE */ yytestcase(yyruleno==135);
#line 342 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[0].minor.yy280 = ON_CONFLICT_ACTION_REPLACE;}
#line 2535 "parse.c"
        break;
      case 65: /* cmd ::= DROP TABLE ifexists fullname */
#line 346 "/home/kir/tarantool/src/box/sql/parse.y"
{
  sql_drop_table(pParse, yymsp[0].minor.yy151, 0, yymsp[-1].minor.yy280);
}
#line 2542 "parse.c"
        break;
      case 68: /* cmd ::= createkw VIEW ifnotexists nm eidlist_opt AS select */
#line 356 "/home/kir/tarantool/src/box/sql/parse.y"
{
  if (!pParse->parse_only)
    sql_create_view(pParse, &yymsp[-6].minor.yy0, &yymsp[-3].minor.yy0, yymsp[-2].minor.yy418, yymsp[0].minor.yy375, yymsp[-4].minor.yy280);
  else
    sql_store_select(pParse, yymsp[0].minor.yy375);
}
#line 2552 "parse.c"
        break;
      case 69: /* cmd ::= DROP VIEW ifexists fullname */
#line 362 "/home/kir/tarantool/src/box/sql/parse.y"
{
  sql_drop_table(pParse, yymsp[0].minor.yy151, 1, yymsp[-1].minor.yy280);
}
#line 2559 "parse.c"
        break;
      case 70: /* cmd ::= select */
#line 368 "/home/kir/tarantool/src/box/sql/parse.y"
{
  SelectDest dest = {SRT_Output, 0, 0, 0, 0, 0, 0};
  if(!pParse->parse_only)
          sqlite3Select(pParse, yymsp[0].minor.yy375, &dest);
  else
          sql_expr_extract_select(pParse, yymsp[0].minor.yy375);
  sql_select_delete(pParse->db, yymsp[0].minor.yy375);
}
#line 2571 "parse.c"
        break;
      case 71: /* select ::= with selectnowith */
#line 410 "/home/kir/tarantool/src/box/sql/parse.y"
{
  Select *p = yymsp[0].minor.yy375;
  if( p ){
    p->pWith = yymsp[-1].minor.yy43;
    parserDoubleLinkSelect(pParse, p);
  }else{
    sqlite3WithDelete(pParse->db, yymsp[-1].minor.yy43);
  }
  yymsp[-1].minor.yy375 = p; /*A-overwrites-W*/
}
#line 2585 "parse.c"
        break;
      case 72: /* selectnowith ::= selectnowith multiselect_op oneselect */
#line 423 "/home/kir/tarantool/src/box/sql/parse.y"
{
  Select *pRhs = yymsp[0].minor.yy375;
  Select *pLhs = yymsp[-2].minor.yy375;
  if( pRhs && pRhs->pPrior ){
    SrcList *pFrom;
    Token x;
    x.n = 0;
    parserDoubleLinkSelect(pParse, pRhs);
    pFrom = sqlite3SrcListAppendFromTerm(pParse,0,0,&x,pRhs,0,0);
    pRhs = sqlite3SelectNew(pParse,0,pFrom,0,0,0,0,0,0,0);
  }
  if( pRhs ){
    pRhs->op = (u8)yymsp[-1].minor.yy280;
    pRhs->pPrior = pLhs;
    if( ALWAYS(pLhs) ) pLhs->selFlags &= ~SF_MultiValue;
    pRhs->selFlags &= ~SF_MultiValue;
    if( yymsp[-1].minor.yy280!=TK_ALL ) pParse->hasCompound = 1;
  }else{
    sql_select_delete(pParse->db, pLhs);
  }
  yymsp[-2].minor.yy375 = pRhs;
}
#line 2611 "parse.c"
        break;
      case 73: /* multiselect_op ::= UNION */
      case 75: /* multiselect_op ::= EXCEPT|INTERSECT */ yytestcase(yyruleno==75);
#line 446 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[0].minor.yy280 = yymsp[0].major; /*A-overwrites-OP*/}
#line 2617 "parse.c"
        break;
      case 74: /* multiselect_op ::= UNION ALL */
#line 447 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[-1].minor.yy280 = TK_ALL;}
#line 2622 "parse.c"
        break;
      case 76: /* oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt orderby_opt limit_opt */
#line 451 "/home/kir/tarantool/src/box/sql/parse.y"
{
#ifdef SELECTTRACE_ENABLED
  Token s = yymsp[-8].minor.yy0; /*A-overwrites-S*/
#endif
  yymsp[-8].minor.yy375 = sqlite3SelectNew(pParse,yymsp[-6].minor.yy418,yymsp[-5].minor.yy151,yymsp[-4].minor.yy62,yymsp[-3].minor.yy418,yymsp[-2].minor.yy62,yymsp[-1].minor.yy418,yymsp[-7].minor.yy280,yymsp[0].minor.yy220.pLimit,yymsp[0].minor.yy220.pOffset);
#ifdef SELECTTRACE_ENABLED
  /* Populate the Select.zSelName[] string that is used to help with
  ** query planner debugging, to differentiate between multiple Select
  ** objects in a complex query.
  **
  ** If the SELECT keyword is immediately followed by a C-style comment
  ** then extract the first few alphanumeric characters from within that
  ** comment to be the zSelName value.  Otherwise, the label is #N where
  ** is an integer that is incremented with each SELECT statement seen.
  */
  if( yymsp[-8].minor.yy375!=0 ){
    const char *z = s.z+6;
    int i;
    sqlite3_snprintf(sizeof(yymsp[-8].minor.yy375->zSelName), yymsp[-8].minor.yy375->zSelName, "#%d",
                     ++pParse->nSelect);
    while( z[0]==' ' ) z++;
    if( z[0]=='/' && z[1]=='*' ){
      z += 2;
      while( z[0]==' ' ) z++;
      for(i=0; sqlite3Isalnum(z[i]); i++){}
      sqlite3_snprintf(sizeof(yymsp[-8].minor.yy375->zSelName), yymsp[-8].minor.yy375->zSelName, "%.*s", i, z);
    }
  }
#endif /* SELECTRACE_ENABLED */
}
#line 2656 "parse.c"
        break;
      case 77: /* values ::= VALUES LP nexprlist RP */
#line 485 "/home/kir/tarantool/src/box/sql/parse.y"
{
  yymsp[-3].minor.yy375 = sqlite3SelectNew(pParse,yymsp[-1].minor.yy418,0,0,0,0,0,SF_Values,0,0);
}
#line 2663 "parse.c"
        break;
      case 78: /* values ::= values COMMA LP exprlist RP */
#line 488 "/home/kir/tarantool/src/box/sql/parse.y"
{
  Select *pRight, *pLeft = yymsp[-4].minor.yy375;
  pRight = sqlite3SelectNew(pParse,yymsp[-1].minor.yy418,0,0,0,0,0,SF_Values|SF_MultiValue,0,0);
  if( ALWAYS(pLeft) ) pLeft->selFlags &= ~SF_MultiValue;
  if( pRight ){
    pRight->op = TK_ALL;
    pRight->pPrior = pLeft;
    yymsp[-4].minor.yy375 = pRight;
  }else{
    yymsp[-4].minor.yy375 = pLeft;
  }
}
#line 2679 "parse.c"
        break;
      case 79: /* distinct ::= DISTINCT */
#line 505 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[0].minor.yy280 = SF_Distinct;}
#line 2684 "parse.c"
        break;
      case 80: /* distinct ::= ALL */
#line 506 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[0].minor.yy280 = SF_All;}
#line 2689 "parse.c"
        break;
      case 82: /* sclp ::= */
      case 108: /* orderby_opt ::= */ yytestcase(yyruleno==108);
      case 115: /* groupby_opt ::= */ yytestcase(yyruleno==115);
      case 191: /* exprlist ::= */ yytestcase(yyruleno==191);
      case 194: /* paren_exprlist ::= */ yytestcase(yyruleno==194);
      case 199: /* eidlist_opt ::= */ yytestcase(yyruleno==199);
#line 519 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[1].minor.yy418 = 0;}
#line 2699 "parse.c"
        break;
      case 83: /* selcollist ::= sclp expr as */
#line 520 "/home/kir/tarantool/src/box/sql/parse.y"
{
   yymsp[-2].minor.yy418 = sql_expr_list_append(pParse->db, yymsp[-2].minor.yy418, yymsp[-1].minor.yy370.pExpr);
   if( yymsp[0].minor.yy0.n>0 ) sqlite3ExprListSetName(pParse, yymsp[-2].minor.yy418, &yymsp[0].minor.yy0, 1);
   sqlite3ExprListSetSpan(pParse,yymsp[-2].minor.yy418,&yymsp[-1].minor.yy370);
}
#line 2708 "parse.c"
        break;
      case 84: /* selcollist ::= sclp STAR */
#line 525 "/home/kir/tarantool/src/box/sql/parse.y"
{
  Expr *p = sqlite3Expr(pParse->db, TK_ASTERISK, 0);
  yymsp[-1].minor.yy418 = sql_expr_list_append(pParse->db, yymsp[-1].minor.yy418, p);
}
#line 2716 "parse.c"
        break;
      case 85: /* selcollist ::= sclp nm DOT STAR */
#line 529 "/home/kir/tarantool/src/box/sql/parse.y"
{
  Expr *pRight = sqlite3PExpr(pParse, TK_ASTERISK, 0, 0);
  Expr *pLeft = sqlite3ExprAlloc(pParse->db, TK_ID, &yymsp[-2].minor.yy0, 1);
  Expr *pDot = sqlite3PExpr(pParse, TK_DOT, pLeft, pRight);
  yymsp[-3].minor.yy418 = sql_expr_list_append(pParse->db,yymsp[-3].minor.yy418, pDot);
}
#line 2726 "parse.c"
        break;
      case 86: /* as ::= AS nm */
      case 213: /* plus_num ::= PLUS INTEGER|FLOAT */ yytestcase(yyruleno==213);
      case 214: /* minus_num ::= MINUS INTEGER|FLOAT */ yytestcase(yyruleno==214);
#line 540 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[-1].minor.yy0 = yymsp[0].minor.yy0;}
#line 2733 "parse.c"
        break;
      case 87: /* as ::= */
#line 542 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[1].minor.yy0.n = 0; yymsp[1].minor.yy0.z = 0;}
#line 2738 "parse.c"
        break;
      case 88: /* from ::= */
#line 554 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[1].minor.yy151 = sqlite3DbMallocZero(pParse->db, sizeof(*yymsp[1].minor.yy151));}
#line 2743 "parse.c"
        break;
      case 89: /* from ::= FROM seltablist */
#line 555 "/home/kir/tarantool/src/box/sql/parse.y"
{
  yymsp[-1].minor.yy151 = yymsp[0].minor.yy151;
  sqlite3SrcListShiftJoinType(yymsp[-1].minor.yy151);
}
#line 2751 "parse.c"
        break;
      case 90: /* stl_prefix ::= seltablist joinop */
#line 563 "/home/kir/tarantool/src/box/sql/parse.y"
{
   if( ALWAYS(yymsp[-1].minor.yy151 && yymsp[-1].minor.yy151->nSrc>0) ) yymsp[-1].minor.yy151->a[yymsp[-1].minor.yy151->nSrc-1].fg.jointype = (u8)yymsp[0].minor.yy280;
}
#line 2758 "parse.c"
        break;
      case 91: /* stl_prefix ::= */
#line 566 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[1].minor.yy151 = 0;}
#line 2763 "parse.c"
        break;
      case 92: /* seltablist ::= stl_prefix nm as indexed_opt on_opt using_opt */
#line 568 "/home/kir/tarantool/src/box/sql/parse.y"
{
  yymsp[-5].minor.yy151 = sqlite3SrcListAppendFromTerm(pParse,yymsp[-5].minor.yy151,&yymsp[-4].minor.yy0,&yymsp[-3].minor.yy0,0,yymsp[-1].minor.yy62,yymsp[0].minor.yy240);
  sqlite3SrcListIndexedBy(pParse, yymsp[-5].minor.yy151, &yymsp[-2].minor.yy0);
}
#line 2771 "parse.c"
        break;
      case 93: /* seltablist ::= stl_prefix nm LP exprlist RP as on_opt using_opt */
#line 573 "/home/kir/tarantool/src/box/sql/parse.y"
{
  yymsp[-7].minor.yy151 = sqlite3SrcListAppendFromTerm(pParse,yymsp[-7].minor.yy151,&yymsp[-6].minor.yy0,&yymsp[-2].minor.yy0,0,yymsp[-1].minor.yy62,yymsp[0].minor.yy240);
  sqlite3SrcListFuncArgs(pParse, yymsp[-7].minor.yy151, yymsp[-4].minor.yy418);
}
#line 2779 "parse.c"
        break;
      case 94: /* seltablist ::= stl_prefix LP select RP as on_opt using_opt */
#line 578 "/home/kir/tarantool/src/box/sql/parse.y"
{
  yymsp[-6].minor.yy151 = sqlite3SrcListAppendFromTerm(pParse,yymsp[-6].minor.yy151,0,&yymsp[-2].minor.yy0,yymsp[-4].minor.yy375,yymsp[-1].minor.yy62,yymsp[0].minor.yy240);
}
#line 2786 "parse.c"
        break;
      case 95: /* seltablist ::= stl_prefix LP seltablist RP as on_opt using_opt */
#line 582 "/home/kir/tarantool/src/box/sql/parse.y"
{
  if( yymsp[-6].minor.yy151==0 && yymsp[-2].minor.yy0.n==0 && yymsp[-1].minor.yy62==0 && yymsp[0].minor.yy240==0 ){
    yymsp[-6].minor.yy151 = yymsp[-4].minor.yy151;
  }else if( yymsp[-4].minor.yy151->nSrc==1 ){
    yymsp[-6].minor.yy151 = sqlite3SrcListAppendFromTerm(pParse,yymsp[-6].minor.yy151,0,&yymsp[-2].minor.yy0,0,yymsp[-1].minor.yy62,yymsp[0].minor.yy240);
    if( yymsp[-6].minor.yy151 ){
      struct SrcList_item *pNew = &yymsp[-6].minor.yy151->a[yymsp[-6].minor.yy151->nSrc-1];
      struct SrcList_item *pOld = yymsp[-4].minor.yy151->a;
      pNew->zName = pOld->zName;
      pNew->pSelect = pOld->pSelect;
      pOld->zName =  0;
      pOld->pSelect = 0;
    }
    sqlite3SrcListDelete(pParse->db, yymsp[-4].minor.yy151);
  }else{
    Select *pSubquery;
    sqlite3SrcListShiftJoinType(yymsp[-4].minor.yy151);
    pSubquery = sqlite3SelectNew(pParse,0,yymsp[-4].minor.yy151,0,0,0,0,SF_NestedFrom,0,0);
    yymsp[-6].minor.yy151 = sqlite3SrcListAppendFromTerm(pParse,yymsp[-6].minor.yy151,0,&yymsp[-2].minor.yy0,pSubquery,yymsp[-1].minor.yy62,yymsp[0].minor.yy240);
  }
}
#line 2811 "parse.c"
        break;
      case 96: /* fullname ::= nm */
#line 607 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[0].minor.yy151 = sqlite3SrcListAppend(pParse->db,0,&yymsp[0].minor.yy0); /*A-overwrites-X*/}
#line 2816 "parse.c"
        break;
      case 97: /* joinop ::= COMMA|JOIN */
#line 613 "/home/kir/tarantool/src/box/sql/parse.y"
{ yymsp[0].minor.yy280 = JT_INNER; }
#line 2821 "parse.c"
        break;
      case 98: /* joinop ::= JOIN_KW JOIN */
#line 615 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[-1].minor.yy280 = sqlite3JoinType(pParse,&yymsp[-1].minor.yy0,0,0);  /*X-overwrites-A*/}
#line 2826 "parse.c"
        break;
      case 99: /* joinop ::= JOIN_KW join_nm JOIN */
#line 617 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[-2].minor.yy280 = sqlite3JoinType(pParse,&yymsp[-2].minor.yy0,&yymsp[-1].minor.yy0,0); /*X-overwrites-A*/}
#line 2831 "parse.c"
        break;
      case 100: /* joinop ::= JOIN_KW join_nm join_nm JOIN */
#line 619 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[-3].minor.yy280 = sqlite3JoinType(pParse,&yymsp[-3].minor.yy0,&yymsp[-2].minor.yy0,&yymsp[-1].minor.yy0);/*X-overwrites-A*/}
#line 2836 "parse.c"
        break;
      case 101: /* on_opt ::= ON expr */
      case 118: /* having_opt ::= HAVING expr */ yytestcase(yyruleno==118);
      case 126: /* where_opt ::= WHERE expr */ yytestcase(yyruleno==126);
      case 187: /* case_else ::= ELSE expr */ yytestcase(yyruleno==187);
#line 623 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[-1].minor.yy62 = yymsp[0].minor.yy370.pExpr;}
#line 2844 "parse.c"
        break;
      case 102: /* on_opt ::= */
      case 117: /* having_opt ::= */ yytestcase(yyruleno==117);
      case 125: /* where_opt ::= */ yytestcase(yyruleno==125);
      case 188: /* case_else ::= */ yytestcase(yyruleno==188);
      case 190: /* case_operand ::= */ yytestcase(yyruleno==190);
#line 624 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[1].minor.yy62 = 0;}
#line 2853 "parse.c"
        break;
      case 103: /* indexed_opt ::= */
#line 637 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[1].minor.yy0.z=0; yymsp[1].minor.yy0.n=0;}
#line 2858 "parse.c"
        break;
      case 104: /* indexed_opt ::= INDEXED BY nm */
#line 638 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[-2].minor.yy0 = yymsp[0].minor.yy0;}
#line 2863 "parse.c"
        break;
      case 105: /* indexed_opt ::= NOT INDEXED */
#line 639 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[-1].minor.yy0.z=0; yymsp[-1].minor.yy0.n=1;}
#line 2868 "parse.c"
        break;
      case 106: /* using_opt ::= USING LP idlist RP */
#line 643 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[-3].minor.yy240 = yymsp[-1].minor.yy240;}
#line 2873 "parse.c"
        break;
      case 107: /* using_opt ::= */
      case 136: /* idlist_opt ::= */ yytestcase(yyruleno==136);
#line 644 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[1].minor.yy240 = 0;}
#line 2879 "parse.c"
        break;
      case 109: /* orderby_opt ::= ORDER BY sortlist */
      case 116: /* groupby_opt ::= GROUP BY nexprlist */ yytestcase(yyruleno==116);
#line 658 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[-2].minor.yy418 = yymsp[0].minor.yy418;}
#line 2885 "parse.c"
        break;
      case 110: /* sortlist ::= sortlist COMMA expr sortorder */
#line 659 "/home/kir/tarantool/src/box/sql/parse.y"
{
  yymsp[-3].minor.yy418 = sql_expr_list_append(pParse->db,yymsp[-3].minor.yy418,yymsp[-1].minor.yy370.pExpr);
  sqlite3ExprListSetSortOrder(yymsp[-3].minor.yy418,yymsp[0].minor.yy280);
}
#line 2893 "parse.c"
        break;
      case 111: /* sortlist ::= expr sortorder */
#line 663 "/home/kir/tarantool/src/box/sql/parse.y"
{
  /* yylhsminor.yy418-overwrites-yymsp[-1].minor.yy370. */
  yylhsminor.yy418 = sql_expr_list_append(pParse->db,NULL,yymsp[-1].minor.yy370.pExpr);
  sqlite3ExprListSetSortOrder(yylhsminor.yy418,yymsp[0].minor.yy280);
}
#line 2902 "parse.c"
  yymsp[-1].minor.yy418 = yylhsminor.yy418;
        break;
      case 112: /* sortorder ::= ASC */
#line 671 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[0].minor.yy280 = SORT_ORDER_ASC;}
#line 2908 "parse.c"
        break;
      case 113: /* sortorder ::= DESC */
#line 672 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[0].minor.yy280 = SORT_ORDER_DESC;}
#line 2913 "parse.c"
        break;
      case 114: /* sortorder ::= */
#line 673 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[1].minor.yy280 = SORT_ORDER_UNDEF;}
#line 2918 "parse.c"
        break;
      case 119: /* limit_opt ::= */
#line 698 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[1].minor.yy220.pLimit = 0; yymsp[1].minor.yy220.pOffset = 0;}
#line 2923 "parse.c"
        break;
      case 120: /* limit_opt ::= LIMIT expr */
#line 699 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[-1].minor.yy220.pLimit = yymsp[0].minor.yy370.pExpr; yymsp[-1].minor.yy220.pOffset = 0;}
#line 2928 "parse.c"
        break;
      case 121: /* limit_opt ::= LIMIT expr OFFSET expr */
#line 701 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[-3].minor.yy220.pLimit = yymsp[-2].minor.yy370.pExpr; yymsp[-3].minor.yy220.pOffset = yymsp[0].minor.yy370.pExpr;}
#line 2933 "parse.c"
        break;
      case 122: /* limit_opt ::= LIMIT expr COMMA expr */
#line 703 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[-3].minor.yy220.pOffset = yymsp[-2].minor.yy370.pExpr; yymsp[-3].minor.yy220.pLimit = yymsp[0].minor.yy370.pExpr;}
#line 2938 "parse.c"
        break;
      case 123: /* cmd ::= with DELETE FROM fullname indexed_opt where_opt */
#line 707 "/home/kir/tarantool/src/box/sql/parse.y"
{
  sqlite3WithPush(pParse, yymsp[-5].minor.yy43, 1);
  sqlite3SrcListIndexedBy(pParse, yymsp[-2].minor.yy151, &yymsp[-1].minor.yy0);
  sqlSubProgramsRemaining = SQL_MAX_COMPILING_TRIGGERS;
  /* Instruct SQL to initate Tarantool's transaction.  */
  pParse->initiateTTrans = true;
  sql_table_delete_from(pParse,yymsp[-2].minor.yy151,yymsp[0].minor.yy62);
}
#line 2950 "parse.c"
        break;
      case 124: /* cmd ::= TRUNCATE TABLE fullname */
#line 718 "/home/kir/tarantool/src/box/sql/parse.y"
{
  sql_table_truncate(pParse, yymsp[0].minor.yy151);
}
#line 2957 "parse.c"
        break;
      case 127: /* cmd ::= with UPDATE orconf fullname indexed_opt SET setlist where_opt */
#line 732 "/home/kir/tarantool/src/box/sql/parse.y"
{
  sqlite3WithPush(pParse, yymsp[-7].minor.yy43, 1);
  sqlite3SrcListIndexedBy(pParse, yymsp[-4].minor.yy151, &yymsp[-3].minor.yy0);
  sqlite3ExprListCheckLength(pParse,yymsp[-1].minor.yy418,"set list"); 
  sqlSubProgramsRemaining = SQL_MAX_COMPILING_TRIGGERS;
  /* Instruct SQL to initate Tarantool's transaction.  */
  pParse->initiateTTrans = true;
  sqlite3Update(pParse,yymsp[-4].minor.yy151,yymsp[-1].minor.yy418,yymsp[0].minor.yy62,yymsp[-5].minor.yy280);
}
#line 2970 "parse.c"
        break;
      case 128: /* setlist ::= setlist COMMA nm EQ expr */
#line 746 "/home/kir/tarantool/src/box/sql/parse.y"
{
  yymsp[-4].minor.yy418 = sql_expr_list_append(pParse->db, yymsp[-4].minor.yy418, yymsp[0].minor.yy370.pExpr);
  sqlite3ExprListSetName(pParse, yymsp[-4].minor.yy418, &yymsp[-2].minor.yy0, 1);
}
#line 2978 "parse.c"
        break;
      case 129: /* setlist ::= setlist COMMA LP idlist RP EQ expr */
#line 750 "/home/kir/tarantool/src/box/sql/parse.y"
{
  yymsp[-6].minor.yy418 = sqlite3ExprListAppendVector(pParse, yymsp[-6].minor.yy418, yymsp[-3].minor.yy240, yymsp[0].minor.yy370.pExpr);
}
#line 2985 "parse.c"
        break;
      case 130: /* setlist ::= nm EQ expr */
#line 753 "/home/kir/tarantool/src/box/sql/parse.y"
{
  yylhsminor.yy418 = sql_expr_list_append(pParse->db, NULL, yymsp[0].minor.yy370.pExpr);
  sqlite3ExprListSetName(pParse, yylhsminor.yy418, &yymsp[-2].minor.yy0, 1);
}
#line 2993 "parse.c"
  yymsp[-2].minor.yy418 = yylhsminor.yy418;
        break;
      case 131: /* setlist ::= LP idlist RP EQ expr */
#line 757 "/home/kir/tarantool/src/box/sql/parse.y"
{
  yymsp[-4].minor.yy418 = sqlite3ExprListAppendVector(pParse, 0, yymsp[-3].minor.yy240, yymsp[0].minor.yy370.pExpr);
}
#line 3001 "parse.c"
        break;
      case 132: /* cmd ::= with insert_cmd INTO fullname idlist_opt select */
#line 763 "/home/kir/tarantool/src/box/sql/parse.y"
{
  sqlite3WithPush(pParse, yymsp[-5].minor.yy43, 1);
  sqlSubProgramsRemaining = SQL_MAX_COMPILING_TRIGGERS;
  /* Instruct SQL to initate Tarantool's transaction.  */
  pParse->initiateTTrans = true;
  sqlite3Insert(pParse, yymsp[-2].minor.yy151, yymsp[0].minor.yy375, yymsp[-1].minor.yy240, yymsp[-4].minor.yy280);
}
#line 3012 "parse.c"
        break;
      case 133: /* cmd ::= with insert_cmd INTO fullname idlist_opt DEFAULT VALUES */
#line 771 "/home/kir/tarantool/src/box/sql/parse.y"
{
  sqlite3WithPush(pParse, yymsp[-6].minor.yy43, 1);
  sqlSubProgramsRemaining = SQL_MAX_COMPILING_TRIGGERS;
  /* Instruct SQL to initate Tarantool's transaction.  */
  pParse->initiateTTrans = true;
  sqlite3Insert(pParse, yymsp[-3].minor.yy151, 0, yymsp[-2].minor.yy240, yymsp[-5].minor.yy280);
}
#line 3023 "parse.c"
        break;
      case 137: /* idlist_opt ::= LP idlist RP */
#line 789 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[-2].minor.yy240 = yymsp[-1].minor.yy240;}
#line 3028 "parse.c"
        break;
      case 138: /* idlist ::= idlist COMMA nm */
#line 791 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[-2].minor.yy240 = sqlite3IdListAppend(pParse->db,yymsp[-2].minor.yy240,&yymsp[0].minor.yy0);}
#line 3033 "parse.c"
        break;
      case 139: /* idlist ::= nm */
#line 793 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[0].minor.yy240 = sqlite3IdListAppend(pParse->db,0,&yymsp[0].minor.yy0); /*A-overwrites-Y*/}
#line 3038 "parse.c"
        break;
      case 140: /* expr ::= LP expr RP */
#line 856 "/home/kir/tarantool/src/box/sql/parse.y"
{spanSet(&yymsp[-2].minor.yy370,&yymsp[-2].minor.yy0,&yymsp[0].minor.yy0); /*A-overwrites-B*/  yymsp[-2].minor.yy370.pExpr = yymsp[-1].minor.yy370.pExpr;}
#line 3043 "parse.c"
        break;
      case 141: /* term ::= NULL */
      case 145: /* term ::= FLOAT|BLOB */ yytestcase(yyruleno==145);
      case 146: /* term ::= STRING */ yytestcase(yyruleno==146);
#line 857 "/home/kir/tarantool/src/box/sql/parse.y"
{spanExpr(&yymsp[0].minor.yy370,pParse,yymsp[0].major,yymsp[0].minor.yy0);/*A-overwrites-X*/}
#line 3050 "parse.c"
        break;
      case 142: /* expr ::= ID|INDEXED */
      case 143: /* expr ::= JOIN_KW */ yytestcase(yyruleno==143);
#line 858 "/home/kir/tarantool/src/box/sql/parse.y"
{spanExpr(&yymsp[0].minor.yy370,pParse,TK_ID,yymsp[0].minor.yy0); /*A-overwrites-X*/}
#line 3056 "parse.c"
        break;
      case 144: /* expr ::= nm DOT nm */
#line 860 "/home/kir/tarantool/src/box/sql/parse.y"
{
  Expr *temp1 = sqlite3ExprAlloc(pParse->db, TK_ID, &yymsp[-2].minor.yy0, 1);
  Expr *temp2 = sqlite3ExprAlloc(pParse->db, TK_ID, &yymsp[0].minor.yy0, 1);
  spanSet(&yymsp[-2].minor.yy370,&yymsp[-2].minor.yy0,&yymsp[0].minor.yy0); /*A-overwrites-X*/
  yymsp[-2].minor.yy370.pExpr = sqlite3PExpr(pParse, TK_DOT, temp1, temp2);
}
#line 3066 "parse.c"
        break;
      case 147: /* term ::= INTEGER */
#line 868 "/home/kir/tarantool/src/box/sql/parse.y"
{
  yylhsminor.yy370.pExpr = sqlite3ExprAlloc(pParse->db, TK_INTEGER, &yymsp[0].minor.yy0, 1);
  yylhsminor.yy370.pExpr->affinity = AFFINITY_INTEGER;
  yylhsminor.yy370.zStart = yymsp[0].minor.yy0.z;
  yylhsminor.yy370.zEnd = yymsp[0].minor.yy0.z + yymsp[0].minor.yy0.n;
  if( yylhsminor.yy370.pExpr ) yylhsminor.yy370.pExpr->flags |= EP_Leaf;
}
#line 3077 "parse.c"
  yymsp[0].minor.yy370 = yylhsminor.yy370;
        break;
      case 148: /* expr ::= VARIABLE */
#line 875 "/home/kir/tarantool/src/box/sql/parse.y"
{
  Token t = yymsp[0].minor.yy0;
  if (pParse->parse_only) {
    spanSet(&yylhsminor.yy370, &t, &t);
    sqlite3ErrorMsg(pParse, "bindings are not allowed in DDL");
    yylhsminor.yy370.pExpr = NULL;
  } else if (!(yymsp[0].minor.yy0.z[0]=='#' && sqlite3Isdigit(yymsp[0].minor.yy0.z[1]))) {
    u32 n = yymsp[0].minor.yy0.n;
    spanExpr(&yylhsminor.yy370, pParse, TK_VARIABLE, yymsp[0].minor.yy0);
    if (yylhsminor.yy370.pExpr->u.zToken[0] == '?' && n > 1)
        sqlite3ErrorMsg(pParse, "near \"%T\": syntax error", &t);
    else
        sqlite3ExprAssignVarNumber(pParse, yylhsminor.yy370.pExpr, n);
  }else{
    assert( t.n>=2 );
    spanSet(&yylhsminor.yy370, &t, &t);
    sqlite3ErrorMsg(pParse, "near \"%T\": syntax error", &t);
    yylhsminor.yy370.pExpr = NULL;
  }
}
#line 3102 "parse.c"
  yymsp[0].minor.yy370 = yylhsminor.yy370;
        break;
      case 149: /* expr ::= expr COLLATE ID|INDEXED */
#line 895 "/home/kir/tarantool/src/box/sql/parse.y"
{
  yymsp[-2].minor.yy370.pExpr = sqlite3ExprAddCollateToken(pParse, yymsp[-2].minor.yy370.pExpr, &yymsp[0].minor.yy0, 1);
  yymsp[-2].minor.yy370.zEnd = &yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n];
}
#line 3111 "parse.c"
        break;
      case 150: /* expr ::= CAST LP expr AS typedef RP */
#line 900 "/home/kir/tarantool/src/box/sql/parse.y"
{
  spanSet(&yymsp[-5].minor.yy370,&yymsp[-5].minor.yy0,&yymsp[0].minor.yy0); /*A-overwrites-X*/
  yymsp[-5].minor.yy370.pExpr = sqlite3ExprAlloc(pParse->db, TK_CAST, 0, 1);
  yymsp[-5].minor.yy370.pExpr->affinity = yymsp[-1].minor.yy66.type;
  sqlite3ExprAttachSubtrees(pParse->db, yymsp[-5].minor.yy370.pExpr, yymsp[-3].minor.yy370.pExpr, 0);
}
#line 3121 "parse.c"
        break;
      case 151: /* expr ::= ID|INDEXED LP distinct exprlist RP */
      case 152: /* expr ::= type_func LP distinct exprlist RP */ yytestcase(yyruleno==152);
#line 907 "/home/kir/tarantool/src/box/sql/parse.y"
{
  if( yymsp[-1].minor.yy418 && yymsp[-1].minor.yy418->nExpr>pParse->db->aLimit[SQLITE_LIMIT_FUNCTION_ARG] ){
    sqlite3ErrorMsg(pParse, "too many arguments on function %T", &yymsp[-4].minor.yy0);
  }
  yylhsminor.yy370.pExpr = sqlite3ExprFunction(pParse, yymsp[-1].minor.yy418, &yymsp[-4].minor.yy0);
  spanSet(&yylhsminor.yy370,&yymsp[-4].minor.yy0,&yymsp[0].minor.yy0);
  if( yymsp[-2].minor.yy280==SF_Distinct && yylhsminor.yy370.pExpr ){
    yylhsminor.yy370.pExpr->flags |= EP_Distinct;
  }
}
#line 3136 "parse.c"
  yymsp[-4].minor.yy370 = yylhsminor.yy370;
        break;
      case 153: /* expr ::= ID|INDEXED LP STAR RP */
#line 932 "/home/kir/tarantool/src/box/sql/parse.y"
{
  yylhsminor.yy370.pExpr = sqlite3ExprFunction(pParse, 0, &yymsp[-3].minor.yy0);
  spanSet(&yylhsminor.yy370,&yymsp[-3].minor.yy0,&yymsp[0].minor.yy0);
}
#line 3145 "parse.c"
  yymsp[-3].minor.yy370 = yylhsminor.yy370;
        break;
      case 154: /* term ::= CTIME_KW */
#line 936 "/home/kir/tarantool/src/box/sql/parse.y"
{
  yylhsminor.yy370.pExpr = sqlite3ExprFunction(pParse, 0, &yymsp[0].minor.yy0);
  spanSet(&yylhsminor.yy370, &yymsp[0].minor.yy0, &yymsp[0].minor.yy0);
}
#line 3154 "parse.c"
  yymsp[0].minor.yy370 = yylhsminor.yy370;
        break;
      case 155: /* expr ::= LP nexprlist COMMA expr RP */
#line 965 "/home/kir/tarantool/src/box/sql/parse.y"
{
  ExprList *pList = sql_expr_list_append(pParse->db, yymsp[-3].minor.yy418, yymsp[-1].minor.yy370.pExpr);
  yylhsminor.yy370.pExpr = sqlite3PExpr(pParse, TK_VECTOR, 0, 0);
  if( yylhsminor.yy370.pExpr ){
    yylhsminor.yy370.pExpr->x.pList = pList;
    spanSet(&yylhsminor.yy370, &yymsp[-4].minor.yy0, &yymsp[0].minor.yy0);
  }else{
    sql_expr_list_delete(pParse->db, pList);
  }
}
#line 3169 "parse.c"
  yymsp[-4].minor.yy370 = yylhsminor.yy370;
        break;
      case 156: /* expr ::= expr AND expr */
      case 157: /* expr ::= expr OR expr */ yytestcase(yyruleno==157);
      case 158: /* expr ::= expr LT|GT|GE|LE expr */ yytestcase(yyruleno==158);
      case 159: /* expr ::= expr EQ|NE expr */ yytestcase(yyruleno==159);
      case 160: /* expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr */ yytestcase(yyruleno==160);
      case 161: /* expr ::= expr PLUS|MINUS expr */ yytestcase(yyruleno==161);
      case 162: /* expr ::= expr STAR|SLASH|REM expr */ yytestcase(yyruleno==162);
      case 163: /* expr ::= expr CONCAT expr */ yytestcase(yyruleno==163);
#line 976 "/home/kir/tarantool/src/box/sql/parse.y"
{spanBinaryExpr(pParse,yymsp[-1].major,&yymsp[-2].minor.yy370,&yymsp[0].minor.yy370);}
#line 3182 "parse.c"
        break;
      case 164: /* likeop ::= LIKE_KW|MATCH */
#line 989 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[0].minor.yy0=yymsp[0].minor.yy0;/*A-overwrites-X*/}
#line 3187 "parse.c"
        break;
      case 165: /* likeop ::= NOT LIKE_KW|MATCH */
#line 990 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[-1].minor.yy0=yymsp[0].minor.yy0; yymsp[-1].minor.yy0.n|=0x80000000; /*yymsp[-1].minor.yy0-overwrite-yymsp[0].minor.yy0*/}
#line 3192 "parse.c"
        break;
      case 166: /* expr ::= expr likeop expr */
#line 991 "/home/kir/tarantool/src/box/sql/parse.y"
{
  ExprList *pList;
  int bNot = yymsp[-1].minor.yy0.n & 0x80000000;
  yymsp[-1].minor.yy0.n &= 0x7fffffff;
  pList = sql_expr_list_append(pParse->db,NULL, yymsp[0].minor.yy370.pExpr);
  pList = sql_expr_list_append(pParse->db,pList, yymsp[-2].minor.yy370.pExpr);
  yymsp[-2].minor.yy370.pExpr = sqlite3ExprFunction(pParse, pList, &yymsp[-1].minor.yy0);
  exprNot(pParse, bNot, &yymsp[-2].minor.yy370);
  yymsp[-2].minor.yy370.zEnd = yymsp[0].minor.yy370.zEnd;
  if( yymsp[-2].minor.yy370.pExpr ) yymsp[-2].minor.yy370.pExpr->flags |= EP_InfixFunc;
}
#line 3207 "parse.c"
        break;
      case 167: /* expr ::= expr likeop expr ESCAPE expr */
#line 1002 "/home/kir/tarantool/src/box/sql/parse.y"
{
  ExprList *pList;
  int bNot = yymsp[-3].minor.yy0.n & 0x80000000;
  yymsp[-3].minor.yy0.n &= 0x7fffffff;
  pList = sql_expr_list_append(pParse->db,NULL, yymsp[-2].minor.yy370.pExpr);
  pList = sql_expr_list_append(pParse->db,pList, yymsp[-4].minor.yy370.pExpr);
  pList = sql_expr_list_append(pParse->db,pList, yymsp[0].minor.yy370.pExpr);
  yymsp[-4].minor.yy370.pExpr = sqlite3ExprFunction(pParse, pList, &yymsp[-3].minor.yy0);
  exprNot(pParse, bNot, &yymsp[-4].minor.yy370);
  yymsp[-4].minor.yy370.zEnd = yymsp[0].minor.yy370.zEnd;
  if( yymsp[-4].minor.yy370.pExpr ) yymsp[-4].minor.yy370.pExpr->flags |= EP_InfixFunc;
}
#line 3223 "parse.c"
        break;
      case 168: /* expr ::= expr IS NULL */
#line 1032 "/home/kir/tarantool/src/box/sql/parse.y"
{spanUnaryPostfix(pParse,TK_ISNULL,&yymsp[-2].minor.yy370,&yymsp[0].minor.yy0);}
#line 3228 "parse.c"
        break;
      case 169: /* expr ::= expr IS NOT NULL */
#line 1033 "/home/kir/tarantool/src/box/sql/parse.y"
{spanUnaryPostfix(pParse,TK_NOTNULL,&yymsp[-3].minor.yy370,&yymsp[0].minor.yy0);}
#line 3233 "parse.c"
        break;
      case 170: /* expr ::= NOT expr */
      case 171: /* expr ::= BITNOT expr */ yytestcase(yyruleno==171);
#line 1055 "/home/kir/tarantool/src/box/sql/parse.y"
{spanUnaryPrefix(&yymsp[-1].minor.yy370,pParse,yymsp[-1].major,&yymsp[0].minor.yy370,&yymsp[-1].minor.yy0);/*A-overwrites-B*/}
#line 3239 "parse.c"
        break;
      case 172: /* expr ::= MINUS expr */
#line 1059 "/home/kir/tarantool/src/box/sql/parse.y"
{spanUnaryPrefix(&yymsp[-1].minor.yy370,pParse,TK_UMINUS,&yymsp[0].minor.yy370,&yymsp[-1].minor.yy0);/*A-overwrites-B*/}
#line 3244 "parse.c"
        break;
      case 173: /* expr ::= PLUS expr */
#line 1061 "/home/kir/tarantool/src/box/sql/parse.y"
{spanUnaryPrefix(&yymsp[-1].minor.yy370,pParse,TK_UPLUS,&yymsp[0].minor.yy370,&yymsp[-1].minor.yy0);/*A-overwrites-B*/}
#line 3249 "parse.c"
        break;
      case 174: /* between_op ::= BETWEEN */
      case 177: /* in_op ::= IN */ yytestcase(yyruleno==177);
#line 1064 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[0].minor.yy280 = 0;}
#line 3255 "parse.c"
        break;
      case 176: /* expr ::= expr between_op expr AND expr */
#line 1066 "/home/kir/tarantool/src/box/sql/parse.y"
{
  ExprList *pList = sql_expr_list_append(pParse->db,NULL, yymsp[-2].minor.yy370.pExpr);
  pList = sql_expr_list_append(pParse->db,pList, yymsp[0].minor.yy370.pExpr);
  yymsp[-4].minor.yy370.pExpr = sqlite3PExpr(pParse, TK_BETWEEN, yymsp[-4].minor.yy370.pExpr, 0);
  if( yymsp[-4].minor.yy370.pExpr ){
    yymsp[-4].minor.yy370.pExpr->x.pList = pList;
  }else{
    sql_expr_list_delete(pParse->db, pList);
  } 
  exprNot(pParse, yymsp[-3].minor.yy280, &yymsp[-4].minor.yy370);
  yymsp[-4].minor.yy370.zEnd = yymsp[0].minor.yy370.zEnd;
}
#line 3271 "parse.c"
        break;
      case 179: /* expr ::= expr in_op LP exprlist RP */
#line 1081 "/home/kir/tarantool/src/box/sql/parse.y"
{
  if( yymsp[-1].minor.yy418==0 ){
    /* Expressions of the form
    **
    **      expr1 IN ()
    **      expr1 NOT IN ()
    **
    ** simplify to constants 0 (false) and 1 (true), respectively,
    ** regardless of the value of expr1.
    */
    sql_expr_delete(pParse->db, yymsp[-4].minor.yy370.pExpr, false);
    yymsp[-4].minor.yy370.pExpr = sqlite3ExprAlloc(pParse->db, TK_INTEGER,&sqlite3IntTokens[yymsp[-3].minor.yy280],1);
  }else if( yymsp[-1].minor.yy418->nExpr==1 ){
    /* Expressions of the form:
    **
    **      expr1 IN (?1)
    **      expr1 NOT IN (?2)
    **
    ** with exactly one value on the RHS can be simplified to something
    ** like this:
    **
    **      expr1 == ?1
    **      expr1 <> ?2
    **
    ** But, the RHS of the == or <> is marked with the EP_Generic flag
    ** so that it may not contribute to the computation of comparison
    ** affinity or the collating sequence to use for comparison.  Otherwise,
    ** the semantics would be subtly different from IN or NOT IN.
    */
    Expr *pRHS = yymsp[-1].minor.yy418->a[0].pExpr;
    yymsp[-1].minor.yy418->a[0].pExpr = 0;
    sql_expr_list_delete(pParse->db, yymsp[-1].minor.yy418);
    /* pRHS cannot be NULL because a malloc error would have been detected
    ** before now and control would have never reached this point */
    if( ALWAYS(pRHS) ){
      pRHS->flags &= ~EP_Collate;
      pRHS->flags |= EP_Generic;
    }
    yymsp[-4].minor.yy370.pExpr = sqlite3PExpr(pParse, yymsp[-3].minor.yy280 ? TK_NE : TK_EQ, yymsp[-4].minor.yy370.pExpr, pRHS);
  }else{
    yymsp[-4].minor.yy370.pExpr = sqlite3PExpr(pParse, TK_IN, yymsp[-4].minor.yy370.pExpr, 0);
    if( yymsp[-4].minor.yy370.pExpr ){
      yymsp[-4].minor.yy370.pExpr->x.pList = yymsp[-1].minor.yy418;
      sqlite3ExprSetHeightAndFlags(pParse, yymsp[-4].minor.yy370.pExpr);
    }else{
      sql_expr_list_delete(pParse->db, yymsp[-1].minor.yy418);
    }
    exprNot(pParse, yymsp[-3].minor.yy280, &yymsp[-4].minor.yy370);
  }
  yymsp[-4].minor.yy370.zEnd = &yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n];
}
#line 3326 "parse.c"
        break;
      case 180: /* expr ::= LP select RP */
#line 1132 "/home/kir/tarantool/src/box/sql/parse.y"
{
  spanSet(&yymsp[-2].minor.yy370,&yymsp[-2].minor.yy0,&yymsp[0].minor.yy0); /*A-overwrites-B*/
  yymsp[-2].minor.yy370.pExpr = sqlite3PExpr(pParse, TK_SELECT, 0, 0);
  sqlite3PExprAddSelect(pParse, yymsp[-2].minor.yy370.pExpr, yymsp[-1].minor.yy375);
}
#line 3335 "parse.c"
        break;
      case 181: /* expr ::= expr in_op LP select RP */
#line 1137 "/home/kir/tarantool/src/box/sql/parse.y"
{
  yymsp[-4].minor.yy370.pExpr = sqlite3PExpr(pParse, TK_IN, yymsp[-4].minor.yy370.pExpr, 0);
  sqlite3PExprAddSelect(pParse, yymsp[-4].minor.yy370.pExpr, yymsp[-1].minor.yy375);
  exprNot(pParse, yymsp[-3].minor.yy280, &yymsp[-4].minor.yy370);
  yymsp[-4].minor.yy370.zEnd = &yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n];
}
#line 3345 "parse.c"
        break;
      case 182: /* expr ::= expr in_op nm paren_exprlist */
#line 1143 "/home/kir/tarantool/src/box/sql/parse.y"
{
  SrcList *pSrc = sqlite3SrcListAppend(pParse->db, 0,&yymsp[-1].minor.yy0);
  Select *pSelect = sqlite3SelectNew(pParse, 0,pSrc,0,0,0,0,0,0,0);
  if( yymsp[0].minor.yy418 )  sqlite3SrcListFuncArgs(pParse, pSelect ? pSrc : 0, yymsp[0].minor.yy418);
  yymsp[-3].minor.yy370.pExpr = sqlite3PExpr(pParse, TK_IN, yymsp[-3].minor.yy370.pExpr, 0);
  sqlite3PExprAddSelect(pParse, yymsp[-3].minor.yy370.pExpr, pSelect);
  exprNot(pParse, yymsp[-2].minor.yy280, &yymsp[-3].minor.yy370);
  yymsp[-3].minor.yy370.zEnd = &yymsp[-1].minor.yy0.z[yymsp[-1].minor.yy0.n];
}
#line 3358 "parse.c"
        break;
      case 183: /* expr ::= EXISTS LP select RP */
#line 1152 "/home/kir/tarantool/src/box/sql/parse.y"
{
  Expr *p;
  spanSet(&yymsp[-3].minor.yy370,&yymsp[-3].minor.yy0,&yymsp[0].minor.yy0); /*A-overwrites-B*/
  p = yymsp[-3].minor.yy370.pExpr = sqlite3PExpr(pParse, TK_EXISTS, 0, 0);
  sqlite3PExprAddSelect(pParse, p, yymsp[-1].minor.yy375);
}
#line 3368 "parse.c"
        break;
      case 184: /* expr ::= CASE case_operand case_exprlist case_else END */
#line 1160 "/home/kir/tarantool/src/box/sql/parse.y"
{
  spanSet(&yymsp[-4].minor.yy370,&yymsp[-4].minor.yy0,&yymsp[0].minor.yy0);  /*A-overwrites-C*/
  yymsp[-4].minor.yy370.pExpr = sqlite3PExpr(pParse, TK_CASE, yymsp[-3].minor.yy62, 0);
  if( yymsp[-4].minor.yy370.pExpr ){
    yymsp[-4].minor.yy370.pExpr->x.pList = yymsp[-1].minor.yy62 ? sql_expr_list_append(pParse->db,yymsp[-2].minor.yy418,yymsp[-1].minor.yy62) : yymsp[-2].minor.yy418;
    sqlite3ExprSetHeightAndFlags(pParse, yymsp[-4].minor.yy370.pExpr);
  }else{
    sql_expr_list_delete(pParse->db, yymsp[-2].minor.yy418);
    sql_expr_delete(pParse->db, yymsp[-1].minor.yy62, false);
  }
}
#line 3383 "parse.c"
        break;
      case 185: /* case_exprlist ::= case_exprlist WHEN expr THEN expr */
#line 1173 "/home/kir/tarantool/src/box/sql/parse.y"
{
  yymsp[-4].minor.yy418 = sql_expr_list_append(pParse->db,yymsp[-4].minor.yy418, yymsp[-2].minor.yy370.pExpr);
  yymsp[-4].minor.yy418 = sql_expr_list_append(pParse->db,yymsp[-4].minor.yy418, yymsp[0].minor.yy370.pExpr);
}
#line 3391 "parse.c"
        break;
      case 186: /* case_exprlist ::= WHEN expr THEN expr */
#line 1177 "/home/kir/tarantool/src/box/sql/parse.y"
{
  yymsp[-3].minor.yy418 = sql_expr_list_append(pParse->db,NULL, yymsp[-2].minor.yy370.pExpr);
  yymsp[-3].minor.yy418 = sql_expr_list_append(pParse->db,yymsp[-3].minor.yy418, yymsp[0].minor.yy370.pExpr);
}
#line 3399 "parse.c"
        break;
      case 189: /* case_operand ::= expr */
#line 1187 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[0].minor.yy62 = yymsp[0].minor.yy370.pExpr; /*A-overwrites-X*/}
#line 3404 "parse.c"
        break;
      case 192: /* nexprlist ::= nexprlist COMMA expr */
#line 1198 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[-2].minor.yy418 = sql_expr_list_append(pParse->db,yymsp[-2].minor.yy418,yymsp[0].minor.yy370.pExpr);}
#line 3409 "parse.c"
        break;
      case 193: /* nexprlist ::= expr */
#line 1200 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[0].minor.yy418 = sql_expr_list_append(pParse->db,NULL,yymsp[0].minor.yy370.pExpr); /*A-overwrites-Y*/}
#line 3414 "parse.c"
        break;
      case 195: /* paren_exprlist ::= LP exprlist RP */
      case 200: /* eidlist_opt ::= LP eidlist RP */ yytestcase(yyruleno==200);
#line 1207 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[-2].minor.yy418 = yymsp[-1].minor.yy418;}
#line 3420 "parse.c"
        break;
      case 196: /* cmd ::= createkw uniqueflag INDEX ifnotexists nm ON nm LP sortlist RP */
#line 1213 "/home/kir/tarantool/src/box/sql/parse.y"
{
  sql_create_index(pParse, &yymsp[-5].minor.yy0, sqlite3SrcListAppend(pParse->db,0,&yymsp[-3].minor.yy0), yymsp[-1].minor.yy418, &yymsp[-9].minor.yy0,
                   SORT_ORDER_ASC, yymsp[-6].minor.yy280, yymsp[-8].minor.yy280);
}
#line 3428 "parse.c"
        break;
      case 197: /* uniqueflag ::= UNIQUE */
#line 1219 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[0].minor.yy280 = SQL_INDEX_TYPE_UNIQUE;}
#line 3433 "parse.c"
        break;
      case 198: /* uniqueflag ::= */
#line 1220 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[1].minor.yy280 = SQL_INDEX_TYPE_NON_UNIQUE;}
#line 3438 "parse.c"
        break;
      case 201: /* eidlist ::= eidlist COMMA nm collate sortorder */
#line 1263 "/home/kir/tarantool/src/box/sql/parse.y"
{
  yymsp[-4].minor.yy418 = parserAddExprIdListTerm(pParse, yymsp[-4].minor.yy418, &yymsp[-2].minor.yy0, yymsp[-1].minor.yy280, yymsp[0].minor.yy280);
}
#line 3445 "parse.c"
        break;
      case 202: /* eidlist ::= nm collate sortorder */
#line 1266 "/home/kir/tarantool/src/box/sql/parse.y"
{
  yymsp[-2].minor.yy418 = parserAddExprIdListTerm(pParse, 0, &yymsp[-2].minor.yy0, yymsp[-1].minor.yy280, yymsp[0].minor.yy280); /*A-overwrites-Y*/
}
#line 3452 "parse.c"
        break;
      case 205: /* cmd ::= DROP INDEX ifexists fullname ON nm */
#line 1277 "/home/kir/tarantool/src/box/sql/parse.y"
{
    sql_drop_index(pParse, yymsp[-2].minor.yy151, &yymsp[0].minor.yy0, yymsp[-3].minor.yy280);
}
#line 3459 "parse.c"
        break;
      case 206: /* cmd ::= PRAGMA nm */
#line 1283 "/home/kir/tarantool/src/box/sql/parse.y"
{
    sqlite3Pragma(pParse,&yymsp[0].minor.yy0,0,0,0);
}
#line 3466 "parse.c"
        break;
      case 207: /* cmd ::= PRAGMA nm EQ nmnum */
#line 1286 "/home/kir/tarantool/src/box/sql/parse.y"
{
    sqlite3Pragma(pParse,&yymsp[-2].minor.yy0,&yymsp[0].minor.yy0,0,0);
}
#line 3473 "parse.c"
        break;
      case 208: /* cmd ::= PRAGMA nm LP nmnum RP */
#line 1289 "/home/kir/tarantool/src/box/sql/parse.y"
{
    sqlite3Pragma(pParse,&yymsp[-3].minor.yy0,&yymsp[-1].minor.yy0,0,0);
}
#line 3480 "parse.c"
        break;
      case 209: /* cmd ::= PRAGMA nm EQ minus_num */
#line 1292 "/home/kir/tarantool/src/box/sql/parse.y"
{
    sqlite3Pragma(pParse,&yymsp[-2].minor.yy0,&yymsp[0].minor.yy0,0,1);
}
#line 3487 "parse.c"
        break;
      case 210: /* cmd ::= PRAGMA nm LP minus_num RP */
#line 1295 "/home/kir/tarantool/src/box/sql/parse.y"
{
    sqlite3Pragma(pParse,&yymsp[-3].minor.yy0,&yymsp[-1].minor.yy0,0,1);
}
#line 3494 "parse.c"
        break;
      case 211: /* cmd ::= PRAGMA nm EQ nm DOT nm */
#line 1298 "/home/kir/tarantool/src/box/sql/parse.y"
{
    sqlite3Pragma(pParse,&yymsp[-4].minor.yy0,&yymsp[0].minor.yy0,&yymsp[-2].minor.yy0,0);
}
#line 3501 "parse.c"
        break;
      case 212: /* cmd ::= PRAGMA */
#line 1301 "/home/kir/tarantool/src/box/sql/parse.y"
{
    sqlite3Pragma(pParse, 0,0,0,0);
}
#line 3508 "parse.c"
        break;
      case 215: /* cmd ::= createkw trigger_decl BEGIN trigger_cmd_list END */
#line 1318 "/home/kir/tarantool/src/box/sql/parse.y"
{
  Token all;
  all.z = yymsp[-3].minor.yy0.z;
  all.n = (int)(yymsp[0].minor.yy0.z - yymsp[-3].minor.yy0.z) + yymsp[0].minor.yy0.n;
  pParse->initiateTTrans = false;
  sql_trigger_finish(pParse, yymsp[-1].minor.yy360, &all);
}
#line 3519 "parse.c"
        break;
      case 216: /* trigger_decl ::= TRIGGER ifnotexists nm trigger_time trigger_event ON fullname foreach_clause when_clause */
#line 1328 "/home/kir/tarantool/src/box/sql/parse.y"
{
  sql_trigger_begin(pParse, &yymsp[-6].minor.yy0, yymsp[-5].minor.yy280, yymsp[-4].minor.yy30.a, yymsp[-4].minor.yy30.b, yymsp[-2].minor.yy151, yymsp[0].minor.yy62, yymsp[-7].minor.yy280);
  yymsp[-8].minor.yy0 = yymsp[-6].minor.yy0; /*yymsp[-8].minor.yy0-overwrites-T*/
}
#line 3527 "parse.c"
        break;
      case 217: /* trigger_time ::= BEFORE */
#line 1334 "/home/kir/tarantool/src/box/sql/parse.y"
{ yymsp[0].minor.yy280 = TK_BEFORE; }
#line 3532 "parse.c"
        break;
      case 218: /* trigger_time ::= AFTER */
#line 1335 "/home/kir/tarantool/src/box/sql/parse.y"
{ yymsp[0].minor.yy280 = TK_AFTER;  }
#line 3537 "parse.c"
        break;
      case 219: /* trigger_time ::= INSTEAD OF */
#line 1336 "/home/kir/tarantool/src/box/sql/parse.y"
{ yymsp[-1].minor.yy280 = TK_INSTEAD;}
#line 3542 "parse.c"
        break;
      case 220: /* trigger_time ::= */
#line 1337 "/home/kir/tarantool/src/box/sql/parse.y"
{ yymsp[1].minor.yy280 = TK_BEFORE; }
#line 3547 "parse.c"
        break;
      case 221: /* trigger_event ::= DELETE|INSERT */
      case 222: /* trigger_event ::= UPDATE */ yytestcase(yyruleno==222);
#line 1341 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[0].minor.yy30.a = yymsp[0].major; /*A-overwrites-X*/ yymsp[0].minor.yy30.b = 0;}
#line 3553 "parse.c"
        break;
      case 223: /* trigger_event ::= UPDATE OF idlist */
#line 1343 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[-2].minor.yy30.a = TK_UPDATE; yymsp[-2].minor.yy30.b = yymsp[0].minor.yy240;}
#line 3558 "parse.c"
        break;
      case 224: /* when_clause ::= */
#line 1350 "/home/kir/tarantool/src/box/sql/parse.y"
{ yymsp[1].minor.yy62 = 0; }
#line 3563 "parse.c"
        break;
      case 225: /* when_clause ::= WHEN expr */
#line 1351 "/home/kir/tarantool/src/box/sql/parse.y"
{ yymsp[-1].minor.yy62 = yymsp[0].minor.yy370.pExpr; }
#line 3568 "parse.c"
        break;
      case 226: /* trigger_cmd_list ::= trigger_cmd_list trigger_cmd SEMI */
#line 1355 "/home/kir/tarantool/src/box/sql/parse.y"
{
  assert( yymsp[-2].minor.yy360!=0 );
  yymsp[-2].minor.yy360->pLast->pNext = yymsp[-1].minor.yy360;
  yymsp[-2].minor.yy360->pLast = yymsp[-1].minor.yy360;
}
#line 3577 "parse.c"
        break;
      case 227: /* trigger_cmd_list ::= trigger_cmd SEMI */
#line 1360 "/home/kir/tarantool/src/box/sql/parse.y"
{ 
  assert( yymsp[-1].minor.yy360!=0 );
  yymsp[-1].minor.yy360->pLast = yymsp[-1].minor.yy360;
}
#line 3585 "parse.c"
        break;
      case 228: /* trnm ::= nm DOT nm */
#line 1371 "/home/kir/tarantool/src/box/sql/parse.y"
{
  yymsp[-2].minor.yy0 = yymsp[0].minor.yy0;
  sqlite3ErrorMsg(pParse, 
        "qualified table names are not allowed on INSERT, UPDATE, and DELETE "
        "statements within triggers");
}
#line 3595 "parse.c"
        break;
      case 229: /* tridxby ::= INDEXED BY nm */
#line 1383 "/home/kir/tarantool/src/box/sql/parse.y"
{
  sqlite3ErrorMsg(pParse,
        "the INDEXED BY clause is not allowed on UPDATE or DELETE statements "
        "within triggers");
}
#line 3604 "parse.c"
        break;
      case 230: /* tridxby ::= NOT INDEXED */
#line 1388 "/home/kir/tarantool/src/box/sql/parse.y"
{
  sqlite3ErrorMsg(pParse,
        "the NOT INDEXED clause is not allowed on UPDATE or DELETE statements "
        "within triggers");
}
#line 3613 "parse.c"
        break;
      case 231: /* trigger_cmd ::= UPDATE orconf trnm tridxby SET setlist where_opt */
#line 1401 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[-6].minor.yy360 = sqlite3TriggerUpdateStep(pParse->db, &yymsp[-4].minor.yy0, yymsp[-1].minor.yy418, yymsp[0].minor.yy62, yymsp[-5].minor.yy280);}
#line 3618 "parse.c"
        break;
      case 232: /* trigger_cmd ::= insert_cmd INTO trnm idlist_opt select */
#line 1405 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[-4].minor.yy360 = sqlite3TriggerInsertStep(pParse->db, &yymsp[-2].minor.yy0, yymsp[-1].minor.yy240, yymsp[0].minor.yy375, yymsp[-4].minor.yy280);/*A-overwrites-R*/}
#line 3623 "parse.c"
        break;
      case 233: /* trigger_cmd ::= DELETE FROM trnm tridxby where_opt */
#line 1409 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[-4].minor.yy360 = sqlite3TriggerDeleteStep(pParse->db, &yymsp[-2].minor.yy0, yymsp[0].minor.yy62);}
#line 3628 "parse.c"
        break;
      case 234: /* trigger_cmd ::= select */
#line 1413 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[0].minor.yy360 = sqlite3TriggerSelectStep(pParse->db, yymsp[0].minor.yy375); /*A-overwrites-X*/}
#line 3633 "parse.c"
        break;
      case 235: /* expr ::= RAISE LP IGNORE RP */
#line 1416 "/home/kir/tarantool/src/box/sql/parse.y"
{
  spanSet(&yymsp[-3].minor.yy370,&yymsp[-3].minor.yy0,&yymsp[0].minor.yy0);  /*A-overwrites-X*/
  yymsp[-3].minor.yy370.pExpr = sqlite3PExpr(pParse, TK_RAISE, 0, 0); 
  if( yymsp[-3].minor.yy370.pExpr ){
    yymsp[-3].minor.yy370.pExpr->on_conflict_action = ON_CONFLICT_ACTION_IGNORE;
  }
}
#line 3644 "parse.c"
        break;
      case 236: /* expr ::= RAISE LP raisetype COMMA STRING RP */
#line 1423 "/home/kir/tarantool/src/box/sql/parse.y"
{
  spanSet(&yymsp[-5].minor.yy370,&yymsp[-5].minor.yy0,&yymsp[0].minor.yy0);  /*A-overwrites-X*/
  yymsp[-5].minor.yy370.pExpr = sqlite3ExprAlloc(pParse->db, TK_RAISE, &yymsp[-1].minor.yy0, 1);
  if( yymsp[-5].minor.yy370.pExpr ) {
    yymsp[-5].minor.yy370.pExpr->on_conflict_action = (enum on_conflict_action) yymsp[-3].minor.yy280;
  }
}
#line 3655 "parse.c"
        break;
      case 237: /* raisetype ::= ROLLBACK */
#line 1432 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[0].minor.yy280 = ON_CONFLICT_ACTION_ROLLBACK;}
#line 3660 "parse.c"
        break;
      case 238: /* raisetype ::= ABORT */
#line 1433 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[0].minor.yy280 = ON_CONFLICT_ACTION_ABORT;}
#line 3665 "parse.c"
        break;
      case 239: /* raisetype ::= FAIL */
#line 1434 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[0].minor.yy280 = ON_CONFLICT_ACTION_FAIL;}
#line 3670 "parse.c"
        break;
      case 240: /* cmd ::= DROP TRIGGER ifexists fullname */
#line 1438 "/home/kir/tarantool/src/box/sql/parse.y"
{
  sql_drop_trigger(pParse,yymsp[0].minor.yy151,yymsp[-1].minor.yy280);
}
#line 3677 "parse.c"
        break;
      case 241: /* cmd ::= ANALYZE */
#line 1443 "/home/kir/tarantool/src/box/sql/parse.y"
{sqlite3Analyze(pParse, 0);}
#line 3682 "parse.c"
        break;
      case 242: /* cmd ::= ANALYZE nm */
#line 1444 "/home/kir/tarantool/src/box/sql/parse.y"
{sqlite3Analyze(pParse, &yymsp[0].minor.yy0);}
#line 3687 "parse.c"
        break;
      case 243: /* cmd ::= ALTER TABLE fullname RENAME TO nm */
#line 1447 "/home/kir/tarantool/src/box/sql/parse.y"
{
  sql_alter_table_rename(pParse,yymsp[-3].minor.yy151,&yymsp[0].minor.yy0);
}
#line 3694 "parse.c"
        break;
      case 244: /* cmd ::= ALTER TABLE fullname ADD CONSTRAINT nm FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt refargs defer_subclause_opt */
#line 1453 "/home/kir/tarantool/src/box/sql/parse.y"
{
    sql_create_foreign_key(pParse, yymsp[-13].minor.yy151, &yymsp[-10].minor.yy0, yymsp[-6].minor.yy418, &yymsp[-3].minor.yy0, yymsp[-2].minor.yy418, yymsp[0].minor.yy280, yymsp[-1].minor.yy280);
}
#line 3701 "parse.c"
        break;
      case 245: /* cmd ::= ALTER TABLE fullname DROP CONSTRAINT nm */
#line 1457 "/home/kir/tarantool/src/box/sql/parse.y"
{
    sql_drop_foreign_key(pParse, yymsp[-3].minor.yy151, &yymsp[0].minor.yy0);
}
#line 3708 "parse.c"
        break;
      case 246: /* with ::= */
#line 1467 "/home/kir/tarantool/src/box/sql/parse.y"
{yymsp[1].minor.yy43 = 0;}
#line 3713 "parse.c"
        break;
      case 247: /* with ::= WITH wqlist */
#line 1469 "/home/kir/tarantool/src/box/sql/parse.y"
{ yymsp[-1].minor.yy43 = yymsp[0].minor.yy43; }
#line 3718 "parse.c"
        break;
      case 248: /* with ::= WITH RECURSIVE wqlist */
#line 1470 "/home/kir/tarantool/src/box/sql/parse.y"
{ yymsp[-2].minor.yy43 = yymsp[0].minor.yy43; }
#line 3723 "parse.c"
        break;
      case 249: /* wqlist ::= nm eidlist_opt AS LP select RP */
#line 1472 "/home/kir/tarantool/src/box/sql/parse.y"
{
  yymsp[-5].minor.yy43 = sqlite3WithAdd(pParse, 0, &yymsp[-5].minor.yy0, yymsp[-4].minor.yy418, yymsp[-1].minor.yy375); /*A-overwrites-X*/
}
#line 3730 "parse.c"
        break;
      case 250: /* wqlist ::= wqlist COMMA nm eidlist_opt AS LP select RP */
#line 1475 "/home/kir/tarantool/src/box/sql/parse.y"
{
  yymsp[-7].minor.yy43 = sqlite3WithAdd(pParse, yymsp[-7].minor.yy43, &yymsp[-5].minor.yy0, yymsp[-4].minor.yy418, yymsp[-1].minor.yy375);
}
#line 3737 "parse.c"
        break;
      case 251: /* typedef ::= TEXT */
#line 1482 "/home/kir/tarantool/src/box/sql/parse.y"
{ yymsp[0].minor.yy66.type = AFFINITY_TEXT; }
#line 3742 "parse.c"
        break;
      case 252: /* typedef ::= BLOB */
#line 1483 "/home/kir/tarantool/src/box/sql/parse.y"
{ yymsp[0].minor.yy66.type = AFFINITY_BLOB; }
#line 3747 "parse.c"
        break;
      case 253: /* typedef ::= DATE */
      case 254: /* typedef ::= TIME */ yytestcase(yyruleno==254);
      case 255: /* typedef ::= DATETIME */ yytestcase(yyruleno==255);
      case 258: /* number_typedef ::= FLOAT|REAL|DOUBLE */ yytestcase(yyruleno==258);
#line 1484 "/home/kir/tarantool/src/box/sql/parse.y"
{ yymsp[0].minor.yy66.type = AFFINITY_REAL; }
#line 3755 "parse.c"
        break;
      case 256: /* typedef ::= CHAR|VARCHAR char_len_typedef */
#line 1489 "/home/kir/tarantool/src/box/sql/parse.y"
{
  yymsp[-1].minor.yy66.type = AFFINITY_TEXT;
  (void) yymsp[0].minor.yy66;
}
#line 3763 "parse.c"
        break;
      case 257: /* char_len_typedef ::= LP INTEGER RP */
      case 262: /* number_len_typedef ::= LP INTEGER RP */ yytestcase(yyruleno==262);
#line 1494 "/home/kir/tarantool/src/box/sql/parse.y"
{
  (void) yymsp[-2].minor.yy66;
  (void) yymsp[-1].minor.yy0;
}
#line 3772 "parse.c"
        break;
      case 259: /* number_typedef ::= INT|INTEGER */
#line 1502 "/home/kir/tarantool/src/box/sql/parse.y"
{ yymsp[0].minor.yy66.type = AFFINITY_INTEGER; }
#line 3777 "parse.c"
        break;
      case 260: /* number_typedef ::= DECIMAL|NUMERIC|NUM number_len_typedef */
#line 1505 "/home/kir/tarantool/src/box/sql/parse.y"
{
  yymsp[-1].minor.yy66.type = AFFINITY_REAL;
  (void) yymsp[0].minor.yy66;
}
#line 3785 "parse.c"
        break;
      case 261: /* number_len_typedef ::= */
#line 1510 "/home/kir/tarantool/src/box/sql/parse.y"
{ (void) yymsp[1].minor.yy66; }
#line 3790 "parse.c"
        break;
      case 263: /* number_len_typedef ::= LP INTEGER COMMA INTEGER RP */
#line 1516 "/home/kir/tarantool/src/box/sql/parse.y"
{
  (void) yymsp[-4].minor.yy66;
  (void) yymsp[-3].minor.yy0;
  (void) yymsp[-1].minor.yy0;
}
#line 3799 "parse.c"
        break;
      default:
      /* (264) input ::= ecmd */ yytestcase(yyruleno==264);
      /* (265) explain ::= */ yytestcase(yyruleno==265);
      /* (266) cmdx ::= cmd (OPTIMIZED OUT) */ assert(yyruleno!=266);
      /* (267) savepoint_opt ::= SAVEPOINT */ yytestcase(yyruleno==267);
      /* (268) savepoint_opt ::= */ yytestcase(yyruleno==268);
      /* (269) cmd ::= create_table create_table_args */ yytestcase(yyruleno==269);
      /* (270) columnlist ::= columnlist COMMA columnname carglist */ yytestcase(yyruleno==270);
      /* (271) columnlist ::= columnname carglist */ yytestcase(yyruleno==271);
      /* (272) carglist ::= carglist ccons */ yytestcase(yyruleno==272);
      /* (273) carglist ::= */ yytestcase(yyruleno==273);
      /* (274) conslist_opt ::= */ yytestcase(yyruleno==274);
      /* (275) conslist_opt ::= COMMA conslist */ yytestcase(yyruleno==275);
      /* (276) conslist ::= conslist tconscomma tcons */ yytestcase(yyruleno==276);
      /* (277) conslist ::= tcons (OPTIMIZED OUT) */ assert(yyruleno!=277);
      /* (278) tconscomma ::= */ yytestcase(yyruleno==278);
      /* (279) defer_subclause_opt ::= defer_subclause (OPTIMIZED OUT) */ assert(yyruleno!=279);
      /* (280) resolvetype ::= raisetype (OPTIMIZED OUT) */ assert(yyruleno!=280);
      /* (281) selectnowith ::= oneselect (OPTIMIZED OUT) */ assert(yyruleno!=281);
      /* (282) oneselect ::= values */ yytestcase(yyruleno==282);
      /* (283) sclp ::= selcollist COMMA */ yytestcase(yyruleno==283);
      /* (284) as ::= ID|STRING */ yytestcase(yyruleno==284);
      /* (285) join_nm ::= ID|INDEXED */ yytestcase(yyruleno==285);
      /* (286) join_nm ::= JOIN_KW */ yytestcase(yyruleno==286);
      /* (287) expr ::= term (OPTIMIZED OUT) */ assert(yyruleno!=287);
      /* (288) type_func ::= DATE */ yytestcase(yyruleno==288);
      /* (289) type_func ::= DATETIME */ yytestcase(yyruleno==289);
      /* (290) type_func ::= CHAR */ yytestcase(yyruleno==290);
      /* (291) exprlist ::= nexprlist */ yytestcase(yyruleno==291);
      /* (292) nmnum ::= plus_num (OPTIMIZED OUT) */ assert(yyruleno!=292);
      /* (293) nmnum ::= STRING */ yytestcase(yyruleno==293);
      /* (294) nmnum ::= nm */ yytestcase(yyruleno==294);
      /* (295) nmnum ::= ON */ yytestcase(yyruleno==295);
      /* (296) nmnum ::= DELETE */ yytestcase(yyruleno==296);
      /* (297) nmnum ::= DEFAULT */ yytestcase(yyruleno==297);
      /* (298) plus_num ::= INTEGER|FLOAT */ yytestcase(yyruleno==298);
      /* (299) foreach_clause ::= */ yytestcase(yyruleno==299);
      /* (300) foreach_clause ::= FOR EACH ROW */ yytestcase(yyruleno==300);
      /* (301) trnm ::= nm */ yytestcase(yyruleno==301);
      /* (302) tridxby ::= */ yytestcase(yyruleno==302);
      /* (303) typedef ::= number_typedef (OPTIMIZED OUT) */ assert(yyruleno!=303);
        break;
/********** End reduce actions ************************************************/
  };
  assert( yyruleno<sizeof(yyRuleInfo)/sizeof(yyRuleInfo[0]) );
  yygoto = yyRuleInfo[yyruleno].lhs;
  yysize = yyRuleInfo[yyruleno].nrhs;
  yyact = yy_find_reduce_action(yymsp[-yysize].stateno,(YYCODETYPE)yygoto);
  if( yyact <= YY_MAX_SHIFTREDUCE ){
    if( yyact>YY_MAX_SHIFT ){
      yyact += YY_MIN_REDUCE - YY_MIN_SHIFTREDUCE;
    }
    yymsp -= yysize-1;
    yypParser->yytos = yymsp;
    yymsp->stateno = (YYACTIONTYPE)yyact;
    yymsp->major = (YYCODETYPE)yygoto;
    yyTraceShift(yypParser, yyact);
  }else{
    assert( yyact == YY_ACCEPT_ACTION );
    yypParser->yytos -= yysize;
    yy_accept(yypParser);
  }
}

/*
** The following code executes when the parse fails
*/
#ifndef YYNOERRORRECOVERY
static void yy_parse_failed(
  yyParser *yypParser           /* The parser */
){
  sqlite3ParserARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sFail!\n",yyTracePrompt);
  }
#endif
  while( yypParser->yytos>yypParser->yystack ) yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser fails */
/************ Begin %parse_failure code ***************************************/
/************ End %parse_failure code *****************************************/
  sqlite3ParserARG_STORE; /* Suppress warning about unused %extra_argument variable */
}
#endif /* YYNOERRORRECOVERY */

/*
** The following code executes when a syntax error first occurs.
*/
static void yy_syntax_error(
  yyParser *yypParser,           /* The parser */
  int yymajor,                   /* The major type of the error token */
  sqlite3ParserTOKENTYPE yyminor         /* The minor type of the error token */
){
  sqlite3ParserARG_FETCH;
#define TOKEN yyminor
/************ Begin %syntax_error code ****************************************/
#line 32 "/home/kir/tarantool/src/box/sql/parse.y"

  UNUSED_PARAMETER(yymajor);  /* Silence some compiler warnings */
  assert( TOKEN.z[0] );  /* The tokenizer always gives us a token */
  if (yypParser->is_fallback_failed && TOKEN.isReserved) {
    sqlite3ErrorMsg(pParse, "keyword \"%T\" is reserved", &TOKEN);
  } else {
    sqlite3ErrorMsg(pParse, "near \"%T\": syntax error", &TOKEN);
  }
#line 3907 "parse.c"
/************ End %syntax_error code ******************************************/
  sqlite3ParserARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/*
** The following is executed when the parser accepts
*/
static void yy_accept(
  yyParser *yypParser           /* The parser */
){
  sqlite3ParserARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sAccept!\n",yyTracePrompt);
  }
#endif
#ifndef YYNOERRORRECOVERY
  yypParser->yyerrcnt = -1;
#endif
  assert( yypParser->yytos==yypParser->yystack );
  /* Here code is inserted which will be executed whenever the
  ** parser accepts */
/*********** Begin %parse_accept code *****************************************/
/*********** End %parse_accept code *******************************************/
  sqlite3ParserARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/* The main parser program.
** The first argument is a pointer to a structure obtained from
** "sqlite3ParserAlloc" which describes the current state of the parser.
** The second argument is the major token number.  The third is
** the minor token.  The fourth optional argument is whatever the
** user wants (and specified in the grammar) and is available for
** use by the action routines.
**
** Inputs:
** <ul>
** <li> A pointer to the parser (an opaque structure.)
** <li> The major token number.
** <li> The minor token number.
** <li> An option argument of a grammar-specified type.
** </ul>
**
** Outputs:
** None.
*/
void sqlite3Parser(
  void *yyp,                   /* The parser */
  int yymajor,                 /* The major token code number */
  sqlite3ParserTOKENTYPE yyminor       /* The value for the token */
  sqlite3ParserARG_PDECL               /* Optional %extra_argument parameter */
){
  YYMINORTYPE yyminorunion;
  unsigned int yyact;   /* The parser action. */
#if !defined(YYERRORSYMBOL) && !defined(YYNOERRORRECOVERY)
  int yyendofinput;     /* True if we are at the end of input */
#endif
#ifdef YYERRORSYMBOL
  int yyerrorhit = 0;   /* True if yymajor has invoked an error */
#endif
  yyParser *yypParser;  /* The parser */

  yypParser = (yyParser*)yyp;
  assert( yypParser->yytos!=0 );
#if !defined(YYERRORSYMBOL) && !defined(YYNOERRORRECOVERY)
  yyendofinput = (yymajor==0);
#endif
  sqlite3ParserARG_STORE;

#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sInput '%s'\n",yyTracePrompt,yyTokenName[yymajor]);
  }
#endif

  do{
    yyact = yy_find_shift_action(yypParser,(YYCODETYPE)yymajor);
    if( yyact <= YY_MAX_SHIFTREDUCE ){
      yy_shift(yypParser,yyact,yymajor,yyminor);
#ifndef YYNOERRORRECOVERY
      yypParser->yyerrcnt--;
#endif
      yymajor = YYNOCODE;
    }else if( yyact <= YY_MAX_REDUCE ){
      yy_reduce(yypParser,yyact-YY_MIN_REDUCE);
    }else{
      assert( yyact == YY_ERROR_ACTION );
      yyminorunion.yy0 = yyminor;
#ifdef YYERRORSYMBOL
      int yymx;
#endif
#ifndef NDEBUG
      if( yyTraceFILE ){
        fprintf(yyTraceFILE,"%sSyntax Error!\n",yyTracePrompt);
      }
#endif
#ifdef YYERRORSYMBOL
      /* A syntax error has occurred.
      ** The response to an error depends upon whether or not the
      ** grammar defines an error token "ERROR".  
      **
      ** This is what we do if the grammar does define ERROR:
      **
      **  * Call the %syntax_error function.
      **
      **  * Begin popping the stack until we enter a state where
      **    it is legal to shift the error symbol, then shift
      **    the error symbol.
      **
      **  * Set the error count to three.
      **
      **  * Begin accepting and shifting new tokens.  No new error
      **    processing will occur until three tokens have been
      **    shifted successfully.
      **
      */
      if( yypParser->yyerrcnt<0 ){
        yy_syntax_error(yypParser,yymajor,yyminor);
      }
      yymx = yypParser->yytos->major;
      if( yymx==YYERRORSYMBOL || yyerrorhit ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE,"%sDiscard input token %s\n",
             yyTracePrompt,yyTokenName[yymajor]);
        }
#endif
        yy_destructor(yypParser, (YYCODETYPE)yymajor, &yyminorunion);
        yymajor = YYNOCODE;
      }else{
        while( yypParser->yytos >= yypParser->yystack
            && yymx != YYERRORSYMBOL
            && (yyact = yy_find_reduce_action(
                        yypParser->yytos->stateno,
                        YYERRORSYMBOL)) >= YY_MIN_REDUCE
        ){
          yy_pop_parser_stack(yypParser);
        }
        if( yypParser->yytos < yypParser->yystack || yymajor==0 ){
          yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
          yy_parse_failed(yypParser);
#ifndef YYNOERRORRECOVERY
          yypParser->yyerrcnt = -1;
#endif
          yymajor = YYNOCODE;
        }else if( yymx!=YYERRORSYMBOL ){
          yy_shift(yypParser,yyact,YYERRORSYMBOL,yyminor);
        }
      }
      yypParser->yyerrcnt = 3;
      yyerrorhit = 1;
#elif defined(YYNOERRORRECOVERY)
      /* If the YYNOERRORRECOVERY macro is defined, then do not attempt to
      ** do any kind of error recovery.  Instead, simply invoke the syntax
      ** error routine and continue going as if nothing had happened.
      **
      ** Applications can set this macro (for example inside %include) if
      ** they intend to abandon the parse upon the first syntax error seen.
      */
      yy_syntax_error(yypParser,yymajor, yyminor);
      yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
      yymajor = YYNOCODE;
      
#else  /* YYERRORSYMBOL is not defined */
      /* This is what we do if the grammar does not define ERROR:
      **
      **  * Report an error message, and throw away the input token.
      **
      **  * If the input token is $, then fail the parse.
      **
      ** As before, subsequent error messages are suppressed until
      ** three input tokens have been successfully shifted.
      */
      if( yypParser->yyerrcnt<=0 ){
        yy_syntax_error(yypParser,yymajor, yyminor);
      }
      yypParser->yyerrcnt = 3;
      yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
      if( yyendofinput ){
        yy_parse_failed(yypParser);
#ifndef YYNOERRORRECOVERY
        yypParser->yyerrcnt = -1;
#endif
      }
      yymajor = YYNOCODE;
#endif
    }
  }while( yymajor!=YYNOCODE && yypParser->yytos>yypParser->yystack );
#ifndef NDEBUG
  if( yyTraceFILE ){
    yyStackEntry *i;
    char cDiv = '[';
    fprintf(yyTraceFILE,"%sReturn. Stack=",yyTracePrompt);
    for(i=&yypParser->yystack[1]; i<=yypParser->yytos; i++){
      fprintf(yyTraceFILE,"%c%s", cDiv, yyTokenName[i->major]);
      cDiv = ' ';
    }
    fprintf(yyTraceFILE,"]\n");
  }
#endif
  return;
}
