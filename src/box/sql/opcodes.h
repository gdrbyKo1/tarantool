/* Automatically generated.  Do not edit */
/* See the tool/mkopcodeh.sh script for details */
#define OP_Savepoint       0
#define OP_SorterNext      1
#define OP_PrevIfOpen      2
#define OP_NextIfOpen      3
#define OP_Prev            4
#define OP_Or              5 /* same as TK_OR, synopsis: r[P3]=(r[P1] || r[P2]) */
#define OP_And             6 /* same as TK_AND, synopsis: r[P3]=(r[P1] && r[P2]) */
#define OP_Not             7 /* same as TK_NOT, synopsis: r[P2]= !r[P1]    */
#define OP_Next            8
#define OP_Goto            9
#define OP_Gosub          10
#define OP_InitCoroutine  11
#define OP_Yield          12
#define OP_Ne             13 /* same as TK_NE, synopsis: IF r[P3]!=r[P1]   */
#define OP_Eq             14 /* same as TK_EQ, synopsis: IF r[P3]==r[P1]   */
#define OP_Gt             15 /* same as TK_GT, synopsis: IF r[P3]>r[P1]    */
#define OP_Le             16 /* same as TK_LE, synopsis: IF r[P3]<=r[P1]   */
#define OP_Lt             17 /* same as TK_LT, synopsis: IF r[P3]<r[P1]    */
#define OP_Ge             18 /* same as TK_GE, synopsis: IF r[P3]>=r[P1]   */
#define OP_ElseNotEq      19 /* same as TK_ESCAPE                          */
#define OP_BitAnd         20 /* same as TK_BITAND, synopsis: r[P3]=r[P1]&r[P2] */
#define OP_BitOr          21 /* same as TK_BITOR, synopsis: r[P3]=r[P1]|r[P2] */
#define OP_ShiftLeft      22 /* same as TK_LSHIFT, synopsis: r[P3]=r[P2]<<r[P1] */
#define OP_ShiftRight     23 /* same as TK_RSHIFT, synopsis: r[P3]=r[P2]>>r[P1] */
#define OP_Add            24 /* same as TK_PLUS, synopsis: r[P3]=r[P1]+r[P2] */
#define OP_Subtract       25 /* same as TK_MINUS, synopsis: r[P3]=r[P2]-r[P1] */
#define OP_Multiply       26 /* same as TK_STAR, synopsis: r[P3]=r[P1]*r[P2] */
#define OP_Divide         27 /* same as TK_SLASH, synopsis: r[P3]=r[P2]/r[P1] */
#define OP_Remainder      28 /* same as TK_REM, synopsis: r[P3]=r[P2]%r[P1] */
#define OP_Concat         29 /* same as TK_CONCAT, synopsis: r[P3]=r[P2]+r[P1] */
#define OP_MustBeInt      30
#define OP_BitNot         31 /* same as TK_BITNOT, synopsis: r[P1]= ~r[P1] */
#define OP_Jump           32
#define OP_Once           33
#define OP_If             34
#define OP_IfNot          35
#define OP_SeekLT         36 /* synopsis: key=r[P3@P4]                     */
#define OP_SeekLE         37 /* synopsis: key=r[P3@P4]                     */
#define OP_SeekGE         38 /* synopsis: key=r[P3@P4]                     */
#define OP_SeekGT         39 /* synopsis: key=r[P3@P4]                     */
#define OP_NoConflict     40 /* synopsis: key=r[P3@P4]                     */
#define OP_NotFound       41 /* synopsis: key=r[P3@P4]                     */
#define OP_Found          42 /* synopsis: key=r[P3@P4]                     */
#define OP_Last           43
#define OP_SorterSort     44
#define OP_Sort           45
#define OP_Rewind         46
#define OP_IdxLE          47 /* synopsis: key=r[P3@P4]                     */
#define OP_IdxGT          48 /* synopsis: key=r[P3@P4]                     */
#define OP_IdxLT          49 /* synopsis: key=r[P3@P4]                     */
#define OP_IdxGE          50 /* synopsis: key=r[P3@P4]                     */
#define OP_Program        51
#define OP_FkIfZero       52 /* synopsis: if fkctr[P1]==0 goto P2          */
#define OP_IfPos          53 /* synopsis: if r[P1]>0 then r[P1]-=P3, goto P2 */
#define OP_IfNotZero      54 /* synopsis: if r[P1]!=0 then r[P1]--, goto P2 */
#define OP_DecrJumpZero   55 /* synopsis: if (--r[P1])==0 goto P2          */
#define OP_Init           56 /* synopsis: Start at P2                      */
#define OP_Return         57
#define OP_EndCoroutine   58
#define OP_HaltIfNull     59 /* synopsis: if r[P3]=null halt               */
#define OP_Halt           60
#define OP_Integer        61 /* synopsis: r[P2]=P1                         */
#define OP_Bool           62 /* synopsis: r[P2]=P1                         */
#define OP_Int64          63 /* synopsis: r[P2]=P4                         */
#define OP_String         64 /* synopsis: r[P2]='P4' (len=P1)              */
#define OP_NextAutoincValue  65 /* synopsis: r[P2] = next value from space sequence, which pageno is r[P1] */
#define OP_Null           66 /* synopsis: r[P2..P3]=NULL                   */
#define OP_SoftNull       67 /* synopsis: r[P1]=NULL                       */
#define OP_Blob           68 /* synopsis: r[P2]=P4 (len=P1, subtype=P3)    */
#define OP_Variable       69 /* synopsis: r[P2]=parameter(P1,P4)           */
#define OP_Move           70 /* synopsis: r[P2@P3]=r[P1@P3]                */
#define OP_Copy           71 /* synopsis: r[P2@P3+1]=r[P1@P3+1]            */
#define OP_String8        72 /* same as TK_STRING, synopsis: r[P2]='P4'    */
#define OP_SCopy          73 /* synopsis: r[P2]=r[P1]                      */
#define OP_IntCopy        74 /* synopsis: r[P2]=r[P1]                      */
#define OP_ResultRow      75 /* synopsis: output=r[P1@P2]                  */
#define OP_CollSeq        76
#define OP_Function0      77 /* synopsis: r[P3]=func(r[P2@P5])             */
#define OP_Function       78 /* synopsis: r[P3]=func(r[P2@P5])             */
#define OP_AddImm         79 /* synopsis: r[P1]=r[P1]+P2                   */
#define OP_RealAffinity   80
#define OP_Cast           81 /* synopsis: affinity(r[P1])                  */
#define OP_Permutation    82
#define OP_Compare        83 /* synopsis: r[P1@P3] <-> r[P2@P3]            */
#define OP_Column         84 /* synopsis: r[P3]=PX                         */
#define OP_Affinity       85 /* synopsis: affinity(r[P1@P2])               */
#define OP_MakeRecord     86 /* synopsis: r[P3]=mkrec(r[P1@P2])            */
#define OP_Count          87 /* synopsis: r[P2]=count()                    */
#define OP_CheckViewReferences  88 /* synopsis: r[P1] = space id                 */
#define OP_TransactionBegin  89
#define OP_TransactionCommit  90
#define OP_TransactionRollback  91
#define OP_TTransaction   92
#define OP_IteratorReopen  93 /* synopsis: index id = P2, space ptr = P4    */
#define OP_IteratorOpen   94 /* synopsis: index id = P2, space ptr = P4 or reg[P3] */
#define OP_OpenTEphemeral  95
#define OP_SorterOpen     96
#define OP_SequenceTest   97 /* synopsis: if (cursor[P1].ctr++) pc = P2    */
#define OP_OpenPseudo     98 /* synopsis: P3 columns in r[P2]              */
#define OP_Close          99
#define OP_ColumnsUsed   100
#define OP_Sequence      101 /* synopsis: r[P2]=cursor[P1].ctr++           */
#define OP_NextSequenceId 102 /* synopsis: r[P2]=get_max(_sequence)         */
#define OP_NextIdEphemeral 103 /* synopsis: r[P2]=get_next_rowid(space[P1])  */
#define OP_FCopy         104 /* synopsis: reg[P2@cur_frame]= reg[P1@root_frame(OPFLAG_SAME_FRAME)] */
#define OP_Delete        105
#define OP_ResetCount    106
#define OP_SorterCompare 107 /* synopsis: if key(P1)!=trim(r[P3],P4) goto P2 */
#define OP_SorterData    108 /* synopsis: r[P2]=data                       */
#define OP_RowData       109 /* synopsis: r[P2]=data                       */
#define OP_NullRow       110
#define OP_SorterInsert  111 /* synopsis: key=r[P2]                        */
#define OP_IdxReplace    112
#define OP_IdxInsert     113 /* synopsis: key=r[P1]                        */
#define OP_SInsert       114 /* synopsis: space id = P1, key = r[P3], on error goto P2 */
#define OP_SDelete       115 /* synopsis: space id = P1, key = r[P2]       */
#define OP_Real          116 /* same as TK_FLOAT, synopsis: r[P2]=P4       */
#define OP_IdxDelete     117 /* synopsis: key=r[P2@P3]                     */
#define OP_Clear         118 /* synopsis: space id = P1                    */
#define OP_ResetSorter   119
#define OP_RenameTable   120 /* synopsis: P1 = space_id, P4 = name         */
#define OP_LoadAnalysis  121
#define OP_Param         122
#define OP_FkCounter     123 /* synopsis: fkctr[P1]+=P2                    */
#define OP_OffsetLimit   124 /* synopsis: if r[P1]>0 then r[P2]=r[P1]+max(0,r[P3]) else r[P2]=(-1) */
#define OP_AggStep0      125 /* synopsis: accum=r[P3] step(r[P2@P5])       */
#define OP_AggStep       126 /* synopsis: accum=r[P3] step(r[P2@P5])       */
#define OP_AggFinal      127 /* synopsis: accum=r[P1] N=P2                 */
#define OP_Expire        128
#define OP_IncMaxid      129
#define OP_Noop          130
#define OP_Explain       131
#define OP_NotUsed_132   132
#define OP_NotUsed_133   133
#define OP_NotUsed_134   134
#define OP_NotUsed_135   135
#define OP_NotUsed_136   136
#define OP_NotUsed_137   137
#define OP_NotUsed_138   138
#define OP_NotUsed_139   139
#define OP_NotUsed_140   140
#define OP_NotUsed_141   141
#define OP_NotUsed_142   142
#define OP_NotUsed_143   143
#define OP_NotUsed_144   144
#define OP_NotUsed_145   145
#define OP_NotUsed_146   146
#define OP_NotUsed_147   147
#define OP_NotUsed_148   148
#define OP_NotUsed_149   149
#define OP_NotUsed_150   150
#define OP_IsNull        151 /* same as TK_ISNULL, synopsis: if r[P1]==NULL goto P2 */
#define OP_NotNull       152 /* same as TK_NOTNULL, synopsis: if r[P1]!=NULL goto P2 */

/* Properties such as "out2" or "jump" that are specified in
** comments following the "case" for each opcode in the vdbe.c
** are encoded into bitvectors as follows:
*/
#define OPFLG_JUMP        0x01  /* jump:  P2 holds jmp target */
#define OPFLG_IN1         0x02  /* in1:   P1 is an input */
#define OPFLG_IN2         0x04  /* in2:   P2 is an input */
#define OPFLG_IN3         0x08  /* in3:   P3 is an input */
#define OPFLG_OUT2        0x10  /* out2:  P2 is an output */
#define OPFLG_OUT3        0x20  /* out3:  P3 is an output */
#define OPFLG_INITIALIZER {\
/*   0 */ 0x00, 0x01, 0x01, 0x01, 0x01, 0x26, 0x26, 0x12,\
/*   8 */ 0x01, 0x01, 0x01, 0x01, 0x03, 0x0b, 0x0b, 0x0b,\
/*  16 */ 0x0b, 0x0b, 0x0b, 0x01, 0x26, 0x26, 0x26, 0x26,\
/*  24 */ 0x26, 0x26, 0x26, 0x26, 0x26, 0x26, 0x03, 0x12,\
/*  32 */ 0x01, 0x01, 0x03, 0x03, 0x09, 0x09, 0x09, 0x09,\
/*  40 */ 0x09, 0x09, 0x09, 0x01, 0x01, 0x01, 0x01, 0x01,\
/*  48 */ 0x01, 0x01, 0x01, 0x01, 0x01, 0x03, 0x03, 0x03,\
/*  56 */ 0x01, 0x02, 0x02, 0x08, 0x00, 0x10, 0x10, 0x10,\
/*  64 */ 0x10, 0x00, 0x10, 0x00, 0x10, 0x10, 0x00, 0x00,\
/*  72 */ 0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x02,\
/*  80 */ 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10,\
/*  88 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
/*  96 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,\
/* 104 */ 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04,\
/* 112 */ 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,\
/* 120 */ 0x00, 0x00, 0x10, 0x00, 0x1a, 0x00, 0x00, 0x00,\
/* 128 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
/* 136 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
/* 144 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03,\
/* 152 */ 0x03,}

/* The sqlite3P2Values() routine is able to run faster if it knows
** the value of the largest JUMP opcode.  The smaller the maximum
** JUMP opcode the better, so the mkopcodeh.sh script that
** generated this include file strives to group all JUMP opcodes
** together near the beginning of the list.
*/
#define SQLITE_MX_JUMP_OPCODE  152  /* Maximum JUMP opcode */
